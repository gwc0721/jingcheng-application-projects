#pragma once

#include "../include/sdk_interface.h"
#include "smi_disk.h"

#define FLASH_ID_OFFSET     0x40
#define FLASH_ID_SIZE       0x80
#define	MAX_ISP_LENGTH			(512 * 1024)

#define IDTAB_IN_ISP(id_offset, id_item)	(id_offset + id_item * 2)

typedef int (*SMI_API_TYPE_0)		(BOOL Win98, HANDLE  hRead, HANDLE  hWrite, HANDLE hDisk);
typedef int (*SMI_API_TYPE_1)       (BOOL Win98, HANDLE  hRead, HANDLE  hWrite, HANDLE hDisk, UCHAR*);
typedef int (*lpUpdate_ISP)         (BOOL Win98, HANDLE  hRead, HANDLE  hWrite, HANDLE hDisk, int MPISP_length, UCHAR* MPISP_buffer, int ISP_length, UCHAR* ISPBuf);
typedef int (*lpRead_ISP)           (BOOL Win98, HANDLE  hRead, HANDLE  hWrite, HANDLE hDisk, int ISP_length, UCHAR* ISP1, UCHAR* ISP2);

struct SMI_API_SET
{
	SMI_API_TYPE_0		mf_check_if_smi_chip;
	SMI_API_TYPE_1		mf_get_id_isp_ver;
	SMI_API_TYPE_1      mf_read_flash_id;
	lpUpdate_ISP        mf_update_isp;
	lpRead_ISP          mf_read_isp;
	SMI_API_TYPE_0		mf_get_initial_bad_block_count;
	SMI_API_TYPE_1      mf_get_identify;
	SMI_API_TYPE_1		mf_get_smart;
};


///////////////////////////////////////////////////////////////////////////////
//---- CFerriFactory

class CFerriFactory : public IFerriFactory
{
public:
	CFerriFactory(LPCTSTR ctrl_name);
	virtual ~CFerriFactory(void);
	IMPLEMENT_INTERFACE;

	// interface
public:
	virtual bool CreateDevice(HANDLE dev, IFerriDevice * & ferri);
	virtual bool ScanDevice(IFerriDevice * & ferri);
	virtual bool CreateDummyDevice(IFerriDevice * & ferri) {return false;};
	bool LoadDll(LPCTSTR path);

protected:
	SMI_API_SET		m_apis;
	HMODULE			m_dll;
	CJCStringT		m_ctrl_name;
};

///////////////////////////////////////////////////////////////////////////////
//---- CFerriComm

class CFerriComm : public IFerriDevice
{
protected:
	CFerriComm(SMI_API_SET * apis, TCHAR drive_letter, LPCTSTR drive_name, SMI_TESTER_TYPE tester, UINT port, HANDLE dev);
	virtual ~CFerriComm(void);
	IMPLEMENT_INTERFACE;

public:
	virtual bool CheckIsFerriChip(void);
	virtual bool GetIspVersion(UCHAR * isp_ver, JCSIZE buf_len)
	{
		CheckDevice(m_dev);
		JCASSERT(m_apis);
		if (buf_len < ISP_VER_LENGTH) THROW_ERROR(ERR_APP, _T("buffer is not enough."));
		int ir = m_apis->mf_get_id_isp_ver(FALSE, NULL, NULL, m_dev, isp_ver);
		return (ir!=0);
	}
	virtual bool ReadFlashId(LPVOID buf, JCSIZE buf_len)
	{
		JCASSERT(m_apis);
		CheckDevice(m_dev);
		if (buf_len < SECTOR_SIZE) THROW_ERROR(ERR_APP, _T("buffer is not enough."));
		int ir = m_apis->mf_read_flash_id(FALSE, NULL, NULL, m_dev, (UCHAR*)buf);
		return (ir!=0);
	}

	//virtual bool DownloadIsp(LPVOID mpisp, JCSIZE mpisp_len, LPVOID isp, JCSIZE isp_len) = 0;
	virtual bool DownloadIsp(IIspBuffer * isp)
	{
		JCASSERT(m_apis);
		CheckDevice(m_dev);
		int ir = m_apis->mf_update_isp(
			FALSE, NULL, NULL, m_dev, m_mpisp_len, m_mpisp, 
			isp->GetSize(), (UCHAR*)(isp->GetBuffer()) );
		isp->ReleaseBuffer();
		return (ir != 0);
	}

	virtual bool ReadIsp(JCSIZE data_len, IIspBuffer * & isp1, IIspBuffer * & isp2);

	virtual UINT GetInitialiBadBlockCount(void)
	{
		CheckDevice(m_dev);
		UINT bad = (UINT)(m_apis->mf_get_initial_bad_block_count(FALSE, NULL, NULL, m_dev));
		return bad;
	}

	virtual bool ReadIdentify(LPVOID buf, JCSIZE buf_len)
	{
		CheckDevice(m_dev);
		if (buf_len < SECTOR_SIZE) THROW_ERROR(ERR_APP, _T("buffer is not enough."));
		int ir = m_apis->mf_get_identify(FALSE, NULL, NULL, m_dev, (UCHAR*)buf);
		return (ir!=0);
	}

	virtual bool ReadSmart(LPVOID buf, JCSIZE buf_len);

	virtual bool SetMpisp(LPVOID mpisp, JCSIZE mpisp_len)
	{
		if (m_mpisp) delete [] m_mpisp;
		m_mpisp = new BYTE[mpisp_len];
		memcpy_s(m_mpisp, mpisp_len, mpisp, mpisp_len);
		m_mpisp_len = mpisp_len;
		return true;
	}
	virtual void Disconnect(void)
	{
		CheckDevice(m_dev);
		HANDLE dev = m_dev;
		m_dev = NULL;
		TestClose(dev);
	}
	virtual bool Connect(void);
	//virtual bool ResetTester(void);
	// on_off == true: power on, on_off == false: power off
	virtual bool PowerOnOff(bool power_on);
	virtual TCHAR GetDriveLetter(void) {return m_drive_letter;}
	virtual void GetDriveName(CJCStringT & str) {str = m_drive_name;}
	virtual UINT GetTesterPort(void) {return m_tester_port;}

protected:
	void CheckDevice(HANDLE dev);

protected:
	SMI_API_SET		* m_apis;
	TCHAR			m_drive_letter;
	CJCStringT		m_drive_name;
	SMI_TESTER_TYPE		m_tester;
	HANDLE			m_dev;
	UINT			m_tester_port;

	BYTE *			m_mpisp;
	JCSIZE			m_mpisp_len;
};

///////////////////////////////////////////////////////////////////////////////
//---- CIspBufferComm
class CIspBufferComm : public IIspBuffer
{
public:
	CIspBufferComm(JCSIZE reserved);
	virtual ~CIspBufferComm(void);
	IMPLEMENT_INTERFACE;

public:
	//virtual void SetCapacity(UINT cap) = 0;
	//virtual void SetProperty(UINT prop_id, UINT val) = 0;
	//virtual void SetSerianNumber(LPCTSTR sn) = 0;
	//virtual void SetVendorSpecific(LPVOID buf, JCSIZE buf_len) = 0;
	//virtual void SetModelName(LPCTSTR model) = 0;

	virtual LPVOID GetBuffer(void) {return m_buf;}
	virtual void ReleaseBuffer(void) {};

	virtual bool LoadFromFile(LPCTSTR fn);
	virtual bool SaveToFile(LPCTSTR fn);
	virtual DWORD CheckSum(void);
	//virtual bool GetIspVersion(LPVOID buf, JCSIZE buf_len) = 0;
	virtual bool Compare(IIspBuffer * isp) const;
	virtual JCSIZE GetSize(void) const {return m_data_len;}

public:
	void FillConvertString(LPCTSTR src, BYTE * dst, DWORD len, char filler = 0x20);
	void SetDataLen(JCSIZE len) {m_data_len = len;}


protected:
	BYTE * m_buf;
	JCSIZE	m_buf_size;		// 缓存大小
	JCSIZE	m_data_len;		// 实际数据大小
};



