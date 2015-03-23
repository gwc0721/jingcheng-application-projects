#pragma once
#include "../../CoreMntLib/include/mntImage.h"

#define STATUS_SUCCESS		0
#define STATUS_INVALID_PARAMETER         (0xC000000DL)    // winnt


///////////////////////////////////////////////////////////////////////////////
//-- factory

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

///////////////////////////////////////////////////////////////////////////////
//-- configuration
class CDeviceConfig
{
public:
	void LoadFromFile(const CJCStringT & fn);

// storage
	CJCStringT	m_storage_file;
	ULONG64		m_total_sec;

// system_info
	CJCStringT	m_data_folder;
	CJCStringT	m_solo_tester;		// data folder for solo tester
	int			m_max_isp_len;

// card_mode
	int		m_chunk_size;	// sector
	int		m_chunk_per_page;
	int		m_page_per_block;
	int		m_block_num;

// for debug
	ULONG64		m_dummy_write_start;
	int		m_dummy_write_size;
	
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
	bool LoadConfig(LPCTSTR sec, LPCTSTR key, int & val, bool mandatory = false);
	bool LoadConfig(LPCTSTR sec, LPCTSTR key, ULONG64 & val, bool mandatory = false);

	bool LoadConfig(LPCTSTR sec, LPCTSTR key, CJCStringT & val, bool mandatory = false);
	bool LoadConfig(LPCTSTR sec, LPCTSTR key, LPTSTR str, JCSIZE len, bool mandatory = false);

protected:
	CJCStringT	m_config_path;
};