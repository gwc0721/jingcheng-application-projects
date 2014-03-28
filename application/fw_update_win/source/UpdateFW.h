// fw_update_win.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CUpdateFWApp:
// See fw_update_win.cpp for the implementation of this class
//

class CUpdateFWApp : public CWinApp
{
public:
	CUpdateFWApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()

public:
	enum PROCESS
	{
		PROC_NOTHING = 0,
		PROC_BACKUP_MBR =	0x80000000,
		PROC_EXPORT_FILE =	0x40000000,
		PROC_EXPORT_GRLDR_MBR = 0x20000000,
		PROC_WRITE_MBR =	0x10000000,
		PROC_SET_STARTUP =	0x08000000,
		PROC_USER_CONFIRM =	0x04000000,
		PROC_REBOOT	=		0x02000000,
		PROC_DELETE_TEMP =	0x00080000,
		PROC_RESTOR_MBR	=   0x00040000,
	};

	PROCESS ParseCommandLine(void);

};

extern CUpdateFWApp theApp;