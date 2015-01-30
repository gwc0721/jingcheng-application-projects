// sm224testB.cpp : Defines the class behaviors for the application.
//
#include "stdafx.h"
#include <vld.h>

#include "UpdateSnTool.h"
#include "UpsnTestModeDlg.h"
#include <stdext.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

LOCAL_LOGGER_ENABLE(_T("upsn"), LOGGER_LEVEL_DEBUGINFO);

/////////////////////////////////////////////////////////////////////////////
// CSm320testBApp

BEGIN_MESSAGE_MAP(CUpdateSnToolApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUpdateSnToolApp construction

CUpdateSnToolApp::CUpdateSnToolApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	//LOGGER_INIT(_T("FILE"), _T("update_sn_tool.log"), 0);
	LOGGER_SELECT_COL( 0
		| CJCLogger::COL_TIME_STAMP
		| CJCLogger::COL_FUNCTION_NAME
		| CJCLogger::COL_REAL_TIME
		);
	LOGGER_CONFIG(_T("upsn.cfg"));
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CUpdateSnToolApp object

CUpdateSnToolApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CUpdateSnToolApp initialization

BOOL CUpdateSnToolApp::InitInstance()
{
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	LOG_RELEASE(_T("Update Serial Number Tool"));
	ReadVersionInfo();
	LOG_RELEASE(_T("Ver. %s"), m_str_ver);
	LOG_RELEASE(_T("Copyright (c) 2014 SiliconMotion.  All rights reserved."));

	// Get run folder
	GetRunFolder(m_run_folder);
	CUpsnSelTestModeDlg	dlg;
	m_pMainWnd = &dlg;

	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	
	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

bool CUpdateSnToolApp::ReadVersionInfo(void)
{
	TCHAR app_path[MAX_PATH];
	CString str_prod_ver;

	GetModuleFileName(NULL, app_path, MAX_PATH);
	UINT ver_info_size = GetFileVersionInfoSize(app_path, 0);

	stdext::auto_array<BYTE> ver_buf(ver_info_size);

	BOOL br = GetFileVersionInfo(app_path, 0, ver_info_size, ver_buf);
	if ( 0 == br )	return false;
	
	// Get language
    struct VersionLanguage
    {
        WORD m_wLanguage;
        WORD m_wCcodePage;
    };
	VersionLanguage * p_ver_lang = NULL;
	UINT length = 0;
	
	br = VerQueryValue(ver_buf, _T("\\VarFileInfo\\Translation"), reinterpret_cast<LPVOID *>(&p_ver_lang), &length);
	if ( 0==br) return false;
    
	str_prod_ver.Format(_T("\\StringFileInfo\\%04x%04x\\ProductVersion"),
		p_ver_lang->m_wLanguage, p_ver_lang->m_wCcodePage);

	LPVOID p_prod_ver = NULL;
	length = 0;
	br = VerQueryValue(ver_buf, str_prod_ver.GetBuffer(), &p_prod_ver, &length);
	if ( 0==br) return false;
	
	LOG_RELEASE(_T("File Ver. %s"), reinterpret_cast<TCHAR *>(p_prod_ver) );

	UINT main_ver, sub_ver;
	_stscanf_s(reinterpret_cast<TCHAR *>(p_prod_ver), _T("%d, %d"), &main_ver, &sub_ver);
	m_str_ver.Format(_T("%d.%d"), main_ver, sub_ver);

	return true;
}


void CUpdateSnToolApp::GetRunFolder(CString & path)
{
	TCHAR cur_dir[MAX_STRING_LEN];
	GetCurrentDirectory(MAX_STRING_LEN-1, cur_dir);
	path = cur_dir;
	path += _T("\\");
}