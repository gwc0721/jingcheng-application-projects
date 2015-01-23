#include "stdafx.h"

#include "../include/application.h"
#include <iostream>
#include <conio.h>


using namespace jcapp;

// {D8AE8880-341F-4e8f-B93D-3A700B5E30AA}
const GUID jcapp::JCAPP_GUID =  
{ 0xd8ae8880, 0x341f, 0x4e8f, { 0xb9, 0x3d, 0x3a, 0x70, 0xb, 0x5e, 0x30, 0xaa } };


AppArguSupport::AppArguSupport(DWORD prop)
	: m_src_file(NULL), m_dst_file(NULL), m_prop(prop)
	//, m_show_help(false)
{
	
}

bool AppArguSupport::CmdLineParse(BYTE * base)
{
	// input output support
	if (m_prop & ARGU_SUPPORT_INFILE)
	{
		m_cmd_parser.AddParamDefine( new jcparam::CArguDescBase(ARGU_SRC_FN, 
			ABBR_SRC_FN, jcparam::VT_STRING, _T("input file, default stdin")) );
	}

	if (m_prop & ARGU_SUPPORT_OUTFILE)
	{
		m_cmd_parser.AddParamDefine( new jcparam::CArguDescBase(ARGU_DST_FN, 
			ABBR_DST_FN, jcparam::VT_STRING, _T("output file, default stdout")) );
	}

	if (m_prop & ARGU_SUPPORT_HELP)
	{
		m_cmd_parser.AddParamDefine( new jcparam::CArguDescBase(ARGU_HELP,
			ABBR_HELP, jcparam::VT_BOOL, _T("show this message")) );
	}

	bool br = m_cmd_parser.Parse(GetCommandLine(), base);
	m_cmd_parser.m_remain.GetValT(ARGU_SRC_FN, m_src_fn);
	m_cmd_parser.m_remain.GetValT(ARGU_DST_FN, m_dst_fn);
	if ( m_cmd_parser.m_remain.Exist(ARGU_HELP) ) throw std::exception("show help\n");

	return br;
}

FILE * AppArguSupport::OpenInputFile(void)
{
	if ( !m_src_fn.empty() )
	{
		stdext::jc_fopen(&m_src_file, m_src_fn.c_str(), _T("r"));
		if (NULL == m_src_file) THROW_ERROR(ERR_USER, _T("failure on openning file %s"), m_src_fn.c_str());
	}
	else m_src_file = stdin;
	return m_src_file;
}

FILE * AppArguSupport::OpenOutputFile(void)
{
	if ( !m_dst_fn.empty() )
	{
		stdext::jc_fopen(&m_dst_file, m_dst_fn.c_str(), _T("w+"));
		if (NULL == m_dst_file) THROW_ERROR(ERR_USER, _T("failure on openning file %s"), m_dst_fn.c_str());
	}
	else m_dst_file = stdout;

	return m_dst_file;
}

void AppArguSupport::ArguCleanUp(void)
{
	if ( (m_src_file != stdin) && (m_src_file != NULL) )	fclose(m_src_file);
	if ( (m_dst_file != stdout)	&& (m_dst_file != NULL) )	fclose(m_dst_file);
}

//void AppArguSupport::ShowHelpInfo(FILE *out);

///////////////////////////////////////////////////////////////////////////////
// --
CJCAppBase * CJCAppBase::m_instance = NULL;

CJCAppBase::CJCAppBase(void)
{
	memset(m_ver, 0, sizeof(UINT) * 4);
	LoadApplicationInfo();
}

void CJCAppBase::GetAppPath(CJCStringT &path)
{
	TCHAR str[FILENAME_MAX];
	GetModuleFileName(NULL, str, FILENAME_MAX-1);
	LPTSTR _path = _tcsrchr(str, _T('\\'));
	if (_path) _path[0] = 0;
	path = str;
}

void CJCAppBase::ShowVersionInfo(FILE * out)
{
	_ftprintf_s(out, _T("\t ver %d.%d.%d.%d\n"), m_ver[0], m_ver[1], m_ver[2], m_ver[3]);
}

void CJCAppBase::ShowAppInfo(FILE * out)
{
	_ftprintf_s(out, AppDescription());
	ShowVersionInfo(out);
}

CJCAppBase * CJCAppBase::GetApp(void)
{
	//static CJCAppBase * instance = NULL;
	if (m_instance == NULL)	
	{
#if APP_GLOBAL_SINGLE_TONE > 0

		CSingleToneEntry * entry = CSingleToneEntry::Instance();
		CSingleToneBase * ptr = NULL;
		entry->QueryStInstance(JCAPP_GUID, ptr);
		instance = dynamic_cast<CJCAppBase*>(ptr);
#endif
	}
	JCASSERT(m_instance);
	return m_instance;
}

bool CJCAppBase::LoadApplicationInfo(void)
{
#ifdef WIN32
	stdext::auto_array<TCHAR> _str_temp(MAX_PATH);
	TCHAR * str_temp = _str_temp;

	GetModuleFileName(NULL, str_temp, MAX_PATH);
	UINT ver_info_size = GetFileVersionInfoSize(str_temp, 0);

	stdext::auto_array<BYTE> ver_buf(ver_info_size);

	BOOL br = GetFileVersionInfo(str_temp, 0, ver_info_size, ver_buf);
	if ( 0 == br )	return false;

	// English (USA)
	static const TCHAR _KEY[] = _T("\\StringFileInfo\\040904B0\\%s");
	//TCHAR sub_key[128];
	LPVOID key_val = NULL;
	JCSIZE length = 0;

	stdext::jc_sprintf(str_temp, MAX_PATH, _KEY, _T("FileVersion"));
	br = VerQueryValue(ver_buf, str_temp, &key_val, &length);
	if ( 0==br) return false;
	//LOG_RELEASE(_T("File ver: %s"), reinterpret_cast<TCHAR *>(key_val) );
	_stscanf_s(reinterpret_cast<TCHAR *>(key_val), _T("%d, %d, %d, %d"), 
		m_ver+0, m_ver+1, m_ver+2, m_ver+3);

	stdext::jc_sprintf(str_temp, MAX_PATH, _KEY, _T("ProductName"));
	br = VerQueryValue(ver_buf, str_temp, &key_val, &length);
	m_product_name = reinterpret_cast<TCHAR *>(key_val);

#endif
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//-- main
int jcapp::local_main(int argc, _TCHAR* argv[])
{
	int ret_code = 0;
	CJCAppBase * app = CJCAppBase::GetApp();
	app->ShowAppInfo(stdout);
	bool show_help = false;

	JCASSERT(app);
	try
	{
		app->Initialize();
		ret_code = app->Run();
	}
	catch (stdext::CJCException & err)
	{
		stdext::jc_fprintf(stderr, _T("error: %s\n"), err.WhatT() );
		ret_code = err.GetErrorID();
		if ( ret_code == stdext::CJCException::ERR_ARGUMENT) show_help = true;
	}
    catch(const std::exception & ex)
    {
        std::cout << ex.what();
		ret_code = -1;
		show_help = true;
    }
	if (show_help ) app->ShowHelpInfo(stdout);
	app->CleanUp();

	//stdext::jc_printf(_T("Press any key to continue..."));
	//getc(stdin);
	return ret_code;
}