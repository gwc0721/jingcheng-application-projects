#pragma once

#include "../../CoreMntLib/include/mntImage.h"


class CDriverFactory : public IDriverFactory
{
public:
	CDriverFactory(void);
	~CDriverFactory(void);

public:
	inline virtual void AddRef()	{};			// static object
	inline virtual void Release(void)	{};		// static object
	virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr){return false;};

	virtual bool	CreateDriver(const CJCStringT & driver_name, jcparam::IValue * param, IImage * & driver);
	virtual UINT	GetRevision(void) const;
};

class CDeviceConfig
{
public:

	void LoadFromFile(const CJCStringT & fn);

// storage
	CJCStringT	m_storage_file;
	ULONG64		m_total_sec;

// system_info
	CJCStringT	m_data_folder;
	JCSIZE		m_max_isp_len;
	//CJCStringT	

protected:
	//template<typename T>
	//bool LoadConfig(LPCTSTR sec, LPCTSTR key, T & val, bool mandatory = false);
	//{
	//	val = GetPrivateProfileInt(sec, key, 0, m_config_file);
	//	if ( (-1 == val) && (mandatory)) THROW_ERROR(ERR_PARAMETER, _T("failure on loading local config %s/%s"), sec, key);
	//	LOG_DEBUG(_T("local config %s/%s=%d"), sec, key, val);
	//	return true;
	//}

	//bool LoadConfigU64(LPCTSTR sec, LPCTSTR key, ULONG64 & val, const ULONG64 def_val);
	//bool LoadConfig(LPCTSTR sec, LPCTSTR key, int & val, bool mandatory = false);
	bool LoadConfig(LPCTSTR sec, LPCTSTR key, ULONG64 & val, bool mandatory = false);

	bool LoadConfig(LPCTSTR sec, LPCTSTR key, CJCStringT & val, bool mandatory = false);
	bool LoadConfig(LPCTSTR sec, LPCTSTR key, LPTSTR str, JCSIZE len, bool mandatory = false);

protected:
	CJCStringT	m_config_path;
};

class CDriverImageFile : public IImage
{
public:
	// create from configuration file
	CDriverImageFile(const CJCStringT & config);
	CDriverImageFile(const CJCStringT & file_name, ULONG64 secs);
	~CDriverImageFile(void);

	IMPLEMENT_INTERFACE;

public:
	// interface
	virtual ULONG32	Read(UCHAR * buf, ULONG64 lba, ULONG32 secs);
	virtual ULONG32	Write(const UCHAR * buf, ULONG64 lba, ULONG32 secs);
	virtual ULONG32	FlushCache(void);
	virtual ULONG32	DeviceControl(ULONG code, READ_WRITE read_write, UCHAR * buf, ULONG32 & data_size, ULONG32 buf_size);
	virtual ULONG64	GetSize(void) const {return m_file_secs;}

	//
	bool Initialize(void);

protected:
	HANDLE OpenStorageFile(const CJCStringT fn, ULONG64 total_sec);
	JCSIZE LoadBinFile(const CJCStringT & fn, UCHAR * buf, JCSIZE buf_len, bool mandatory = false);
	void SaveBinFile(const CJCStringT & fn, UCHAR * buf, JCSIZE buf_len);

	ULONG32 ScsiCommand(READ_WRITE rd_wr, UCHAR *buf, JCSIZE buf_len, UCHAR *cb, JCSIZE cb_length, UINT timeout);

	enum VENDOR_CMD_STATUS
	{	// standby
		VDS0 = 0,	VDS1 = 1,	VDS2 = 2,	VDS3 = 3,
		VDS4 = 4,	VDS_CMD = 5,VDS_DATA = 6,
	};

	// 返回true这数据有vendor command处理，不要读写实际数据
	bool VendorCmdStatus(READ_WRITE rdwr, ULONG64 lba, ULONG32 secs, UCHAR * buf);
	JCSIZE ProcessVendorCommand(const UCHAR * vcmd, JCSIZE vcmd_len, READ_WRITE rdwr, UCHAR * data, JCSIZE data_sec);

	// read_secs: vendor command 中的读取大小
	// secs：buffer大小
	void ReadFlashData(WORD block, WORD page, BYTE chunk, BYTE mode, BYTE read_secs, UCHAR * buf, JCSIZE secs);
	void WriteFlashData(WORD block, WORD page, BYTE read_secs, UCHAR * buf, JCSIZE secs);
	VENDOR_CMD_STATUS	m_vendorcmd_st;
	// 保存当前vendor command内容
	UCHAR m_vendor_cmd[SECTOR_SIZE];

protected:
	HANDLE		m_file;
	ULONG64		m_file_secs;

	// vendor command entry read 0x55AA需要返回的内容
	UCHAR m_buf_vendor[SECTOR_SIZE];
	UCHAR m_buf_fid[SECTOR_SIZE];
	UCHAR m_buf_sfr[SECTOR_SIZE];
	UCHAR m_buf_par[SECTOR_SIZE];
	UCHAR m_buf_idtable[SECTOR_SIZE];
	UCHAR m_buf_device_info[SECTOR_SIZE];

	UCHAR m_buf_isp[MAX_ISP_SEC * SECTOR_SIZE];
	UCHAR m_buf_info[MAX_ISP_SEC * SECTOR_SIZE];
	UCHAR m_buf_orgbad[MAX_ISP_SEC * SECTOR_SIZE];

	UCHAR m_buf_bootisp[BOOTISP_SIZE * SECTOR_SIZE];


	JCSIZE m_isp_len;
	JCSIZE m_info_len;
	JCSIZE m_orgbad_len;

	CDeviceConfig	m_config;

	enum RUN_TYPE
	{
		RUN_ROM_CODE = 0, RUN_MPISP = 1, RUN_ISP = 2,
	};
	RUN_TYPE	m_run_type;
};

extern CDriverFactory	g_factory;