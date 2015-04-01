#include "stdafx.h"

#include "../include/device_config.h"
#include "config.h"

#include <jcapp.h>

LOCAL_LOGGER_ENABLE(_T("device_config"), LOGGER_LEVEL_ERROR);

///////////////////////////////////////////////////////////////////////////////
// configuration
void CDeviceConfig::LoadFromFile(const CJCStringT & fn)
{
	jcapp::CJCAppBase * app = jcapp::CJCAppBase::GetApp(); JCASSERT(app);
	CJCStringT app_path;
	app->GetAppPath(app_path);
	m_config_path = app_path + _T("\\") + fn;
	LOG_DEBUG(_T("open config file: %s"), m_config_path.c_str() );
	
	// 取得配置文件的所在目录，以后使用相对目录
	TCHAR path[MAX_PATH];
	LPTSTR fnpart= NULL;
	GetFullPathName(m_config_path.c_str(), MAX_PATH, path, &fnpart);
	fnpart[0] = 0;
	CJCStringT config_path = path;

	bool br = true;
	
	CJCStringT str;
	br = LoadConfig(SEG_SYSTEM_INFO, FIELD_DATA_FOLDER,		str,				true);		// MANDATORY
	m_data_folder = config_path + str;
	LOG_DEBUG(_T("data folder: %s"), m_data_folder.c_str() );

	br = LoadConfig(SEG_SYSTEM_INFO, FIELD_MAX_ISP_LEN,		m_max_isp_len,		true);		// MANDATORY

	br = LoadConfig(SEG_STORAGE,	FIELD_STORAGE_FILE,		str,				true);		// MANDATORY
	m_storage_file = config_path + str;
	LOG_DEBUG(_T("storage file: %s"), m_storage_file.c_str() );

	br = LoadConfig(SEG_STORAGE,	FIELD_TOTAL_SEC,		m_total_sec,		true);		// MANDATORY

	br = LoadConfig(SEG_CARD_MODE,	FIELD_CHUNK_SIZE,		m_chunk_size,		true);		// MANDATORY
	br = LoadConfig(SEG_CARD_MODE,	FIELD_CHUNK_PER_PAGE,	m_chunk_per_page,	true);		// MANDATORY
	br = LoadConfig(SEG_CARD_MODE,	FIELD_PAGE_PER_BLOCK,	m_page_per_block,	false);		// MANDATORY
	br = LoadConfig(SEG_CARD_MODE,	FIELD_BLOCK_NUM,		m_block_num,		false);		// MANDATORY

	str.clear();
	br = LoadConfig(SEG_TESTER,		FIELD_DATA_FOLDER,		str,				false);
	if (!str.empty()) m_solo_tester = config_path + str;
	else	LOG_WARNING(_T("[INIT] solo tester data folder is not set"));

	// for debug
	m_dummy_write_start=(-1);
	m_dummy_write_size=0;
	br = LoadConfig(SEG_CARD_DEBUG,	FIELD_DUMWR_START,	m_dummy_write_start,	false);		// MANDATORY
	br = LoadConfig(SEG_CARD_DEBUG,	FILED_DUMWR_SIZE,	m_dummy_write_size,		false);		// MANDATORY
}

bool CDeviceConfig::LoadConfig(LPCTSTR sec, LPCTSTR key, int & val, bool mandatory)
{
	val = GetPrivateProfileInt(sec, key, 0, m_config_path.c_str() );
	//if ( (0 == val) && (mandatory) ) THROW_ERROR(ERR_PARAMETER, _T("failure on loading local config %s/%s"), sec, key);
	LOG_DEBUG(_T("local config %s/%s=%d"), sec, key, val);
	return true;
}

bool CDeviceConfig::LoadConfig(LPCTSTR sec, LPCTSTR key, ULONG64 & val, bool mandatory)
{
	TCHAR str[MAX_STRING_LEN];
	bool br = LoadConfig(sec, key, str, MAX_STRING_LEN, mandatory);
	if (!br) return false;
	val = _tstoi64(str);
	return true;
}

bool CDeviceConfig::LoadConfig(LPCTSTR sec, LPCTSTR key, CJCStringT & val, bool mandatory)
{
	TCHAR str[MAX_STRING_LEN];
	bool br = LoadConfig(sec, key, str, MAX_STRING_LEN, mandatory);
	if (!br) return false;
	val = str;
	LOG_DEBUG(_T("local config %s/%s=%s"), sec, key, val.c_str());
	return true;
}

bool CDeviceConfig::LoadConfig(LPCTSTR sec, LPCTSTR key, LPTSTR str, JCSIZE len, bool mandatory)
{
	str[0] = 0;
	DWORD ir = GetPrivateProfileString(sec, key, NULL, str, len, m_config_path.c_str());
	//if ( (ir == 0) && mandatory)	THROW_ERROR(ERR_USER, _T("missing %s:%s in config file %s"), sec, key, m_config_path.c_str());
	return ( ir != 0);
}

class jcparam::CArguDefList jcapp::AppArguSupport::m_cmd_parser;