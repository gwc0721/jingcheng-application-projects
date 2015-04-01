#pragma once

#include <stdext.h>

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
	
public:
	bool LoadConfig(LPCTSTR sec, LPCTSTR key, int & val, bool mandatory = false);
	bool LoadConfig(LPCTSTR sec, LPCTSTR key, ULONG64 & val, bool mandatory = false);

	bool LoadConfig(LPCTSTR sec, LPCTSTR key, CJCStringT & val, bool mandatory = false);
	bool LoadConfig(LPCTSTR sec, LPCTSTR key, LPTSTR str, JCSIZE len, bool mandatory = false);

protected:
	CJCStringT	m_config_path;
};