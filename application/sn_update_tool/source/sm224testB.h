// sm224testB.h : main header file for the SM224TESTB application
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

//#include "resource.h"		// main symbols
#define     Channel 4
#define     CE      8
/////////////////////////////////////////////////////////////////////////////
// CUpdateSnToolApp:
// See sm224testB.cpp for the implementation of this class
//

class CUpdateSnToolApp : public CWinApp
{
public:
	CUpdateSnToolApp();

	CHAR szPreloadPath[256];
	int nEntryState;
	CMenu* pMenu;

// Overrides
	// ClassWizard generated virtual function overrides
	public:
	virtual BOOL InitInstance();

// Implementation
	DECLARE_MESSAGE_MAP()
	
public:
	const CString & GetVer(void) const {return m_str_ver;};
	const CString & GetRunFolder(void) const {return m_run_folder;};

protected:
	bool ReadVersionInfo(void);
	void GetRunFolder(CString & str);

protected:
	CString m_str_ver;
	CString	m_run_folder;
};


/////////////////////////////////////////////////////////////////////////////