#include "stdafx.h"

#include "../include/application.h"

using namespace jcapp;

// {D8AE8880-341F-4e8f-B93D-3A700B5E30AA}
const GUID jcapp::JCAPP_GUID =  
{ 0xd8ae8880, 0x341f, 0x4e8f, { 0xb9, 0x3d, 0x3a, 0x70, 0xb, 0x5e, 0x30, 0xaa } };


AppArguSupport::AppArguSupport(DWORD prop)
	: m_src_file(NULL), m_dst_file(NULL), m_prop(prop)
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

	return true;
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


///////////////////////////////////////////////////////////////////////////////
// --
/*
bool CJCAppBase::CmdLineParse(BYTE *base)
{
	if (prop & ARGU_SUPPORT)
	{
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

		return true;
	}
}

void CJCAppBase::CleanUp(void)
{
	if ( (m_src_file != stdin) && (m_src_file != NULL) )	fclose(m_src_file);
	if ( (m_dst_file != stdout)	&& (m_dst_file != NULL) )	fclose(m_dst_file);
}

*/

void CJCAppBase::GetAppPath(CJCStringT &path)
{
	TCHAR str[FILENAME_MAX];
	GetModuleFileName(NULL, str, FILENAME_MAX-1);
	LPTSTR _path = _tcsrchr(str, _T('\\'));
	if (_path) _path[0] = 0;
	path = str;
}

CJCAppBase * CJCAppBase::GetApp(void)
{
	static CJCAppBase * instance = NULL;
	if (instance == NULL)	
	{
		CSingleToneEntry * entry = CSingleToneEntry::Instance();
		CSingleToneBase * ptr = NULL;
		entry->QueryStInstance(JCAPP_GUID, ptr);
		instance = dynamic_cast<CJCAppBase*>(ptr);
	}
	return instance;
}
