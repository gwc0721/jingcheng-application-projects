// fw_update_win.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "fw_update_win.h"

#include <vld.h>
#include <stdext.h>

#include "config.h"
#include "export_file.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

LOCAL_LOGGER_ENABLE(_T("update_fw"), LOGGER_LEVEL_DEBUGINFO);


LOGGER_TO_DEBUG(0, 0
		| CJCLogger::COL_TIME_STAMP
		| CJCLogger::COL_COMPNENT_NAME
		| CJCLogger::COL_FUNCTION_NAME 
		, 0);

// CUpdateFWApp

BEGIN_MESSAGE_MAP(CUpdateFWApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CUpdateFWApp construction

CUpdateFWApp::CUpdateFWApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CUpdateFWApp object

CUpdateFWApp theApp;


// CUpdateFWApp initialization

BOOL CUpdateFWApp::InitInstance()
{
	LOG_STACK_TRACE();

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	HANDLE disk = INVALID_HANDLE_VALUE;

	// process options
	PROCESS proc = ParseCommandLine();
	bool delete_temp = false;

	try
	{
		BYTE* ptr_grldr_mbr = NULL;
		JCSIZE size_grldr_mbr = 0;
		size_grldr_mbr = ReadResource(IDR_GRLDR_MBR, ptr_grldr_mbr);

		OpenDevice(disk, m_target_drive);
		stdext::auto_array<BYTE> cur_mbr(size_grldr_mbr);
		bool got_cur_mbr = false;

		CString file_name;
		if (proc & PROC_BACKUP_MBR)
		{
			got_cur_mbr = true;
			file_name.Format(_T("%s%s"), m_target_drive, FN_MBR_BACKUP);
			BackupMBR(disk, cur_mbr, size_grldr_mbr, file_name);
			// set partition table
			memcpy(ptr_grldr_mbr + PARTITION_TAB, cur_mbr + PARTITION_TAB, PARTITION_TAB_LEN);
		}

		if (proc & PROC_USER_CONFIRM)
		{
			int ir = AfxMessageBox(
				_T("This Application will reboot and update SSD firmware!")
				_T("Please BackUp your data!Continue please press Yes!")
				_T("Don't turn off computer until update was successed!!"),
				MB_OKCANCEL);

			if( ir==IDNO || ir==IDCANCEL || ir==IDABORT) THROW_ERROR(ERR_USER, _T("user canceled"));
		}

		if (proc & PROC_EXPORT_FILE)
		{
			file_name.Format(_T("%s%s"), m_target_drive, FN_DOS_IMAGE);
			ExportFile(IDR_DOS_IMG, file_name);

			file_name.Format(_T("%s%s"), m_target_drive, FN_GRLDR);
			ExportFile(IDR_GRLDR, file_name);

			file_name.Format(_T("%s%s"), m_target_drive, FN_MENU_LST);
			ExportFile(IDR_MENU_LST, file_name);
		}
		
		if (proc & PROC_EXPORT_GRLDR_MBR)
		{
			file_name.Format(_T("%s%s"), m_target_drive, FN_GRLDR_MBR);
			FILE * file = NULL;
			stdext::jc_fopen(&file, file_name, _T("w+b"));
			if (NULL == file) THROW_ERROR(ERR_USER, _T("failure on openning file %s"), file_name);
			fwrite(ptr_grldr_mbr, 1, size_grldr_mbr, file);
			fclose(file);
			LOG_DEBUG(_T("save resource %d to %s, size=%d"), IDR_GRLDR_MBR, file_name, size_grldr_mbr);
		}

		if (proc & PROC_WRITE_MBR)
		{
			JCASSERT(got_cur_mbr);
			if (! got_cur_mbr) LOG_ERROR(_T("have not read current mbr!"));
#ifdef _DEBUG
			JCASSERT(0);
#endif
			WriteGrldrMbr(disk, ptr_grldr_mbr, size_grldr_mbr);
		}

		if (proc & PROC_SET_STARTUP)
		{
#ifdef _DEBUG
			JCASSERT(0);
#endif			
			SetAutoRun();
		}

		if (proc & PROC_RESTOR_MBR)
		{
#ifdef _DEBUG
			JCASSERT(0);
#endif	
			file_name.Format(_T("%s%s"), m_target_drive, FN_MBR_BACKUP);
			RestoreMBR(disk, file_name);
		}

		//disk.close();

		if (proc & PROC_DELETE_TEMP)
		{
			delete_temp = true;
			CleanUp();
		}

		if (proc & PROC_REBOOT)
		{
			JCASSERT(false == delete_temp);
			if (delete_temp) LOG_ERROR(_T("temp files were deleted!"));
#ifdef _DEBUG
			JCASSERT(0);
#endif			
			Reboot();
		}
	}
	catch (std::exception & /*err*/)
	{
		if (!delete_temp) CleanUp();
	}
	if (disk && disk != INVALID_HANDLE_VALUE)	CloseHandle(disk);

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

bool CUpdateFWApp::CleanUp(void)
{
	CString file_name;

	file_name.Format(_T("%s%s"), m_target_drive, FN_MBR_BACKUP);
	DeleteFile(file_name);

	file_name.Format(_T("%s%s"), m_target_drive, FN_DOS_IMAGE);
	DeleteFile(file_name);

	file_name.Format(_T("%s%s"), m_target_drive, FN_GRLDR);
	DeleteFile(file_name);

	file_name.Format(_T("%s%s"), m_target_drive, FN_MENU_LST);
	DeleteFile(file_name);

	file_name.Format(_T("%s%s"), m_target_drive, FN_GRLDR_MBR);
	DeleteFile(file_name);

	RemoveAutoRun();
}

CUpdateFWApp::PROCESS CUpdateFWApp::ParseCommandLine(void)
{
	LOG_STACK_TRACE();

	int args = 0;
	LPCTSTR cmd_line = GetCommandLine();
	LPTSTR * cmd_args = CommandLineToArgvW(cmd_line, &args);

	PROCESS proc = PROC_NOTHING;

	m_target_drive = DEFAULT_TARGET_DRIVE;


	for (int ii = 0; ii< args; ++ii)
	{
		if (_tcscmp(cmd_args[ii], _T("/b")) == 0)
		{	// backup mbr
			LOG_DEBUG(_T("got option /b"));
			proc = PROC_BACKUP_MBR;
		}
		else if (_tcscmp(cmd_args[ii], _T("/e")) == 0)
		{	// backup mbr and export file
			LOG_DEBUG(_T("got option /e"));
			proc = (PROCESS)( proc
				| PROC_EXPORT_FILE
				| PROC_EXPORT_GRLDR_MBR);
		}
		else if (_tcscmp(cmd_args[ii], _T("/a")) == 0)
		{	// backup mbr and export file
			LOG_DEBUG(_T("got option /a"));
			proc = (PROCESS)(proc
				| PROC_SET_STARTUP);
		}

		else if (_tcscmp(cmd_args[ii], _T("/w")) == 0)
		{	// backup mbr, export file, export grldr
			LOG_DEBUG(_T("got option /w"));
			proc = (PROCESS)(PROC_BACKUP_MBR 
				| PROC_EXPORT_FILE 
				| PROC_WRITE_MBR);
		}
		else if (_tcscmp(cmd_args[ii], _T("/n")) == 0)
		{	// backup mbr, export file, export grldr
			LOG_DEBUG(_T("got option /n"));
			proc = (PROCESS)(PROC_BACKUP_MBR 
				| PROC_EXPORT_FILE 
				| PROC_WRITE_MBR 
				| PROC_SET_STARTUP);
		}
		else if (_tcscmp(cmd_args[ii], _T("/o")) == 0)
		{
			LOG_DEBUG(_T("got option /o"));
			proc = (PROCESS)(PROC_BACKUP_MBR 
				| PROC_EXPORT_FILE 
				| PROC_WRITE_MBR 
				| PROC_SET_STARTUP
				| PROC_USER_CONFIRM
				| PROC_REBOOT);
		}
		else if (_tcscmp(cmd_args[ii], _T("/r")) == 0)
		{
			LOG_DEBUG(_T("got option /r"));
			proc = (PROCESS)(PROC_BACKUP_MBR 
				| PROC_EXPORT_FILE 
				| PROC_WRITE_MBR 
				| PROC_SET_STARTUP
				| PROC_REBOOT);
		}
		else if (_tcscmp(cmd_args[ii], _T("/d")) == 0)
		{
			LOG_DEBUG(_T("got option /d"));
			proc = PROC_DELETE_TEMP;
		}	
		else if (_tcscmp(cmd_args[ii], _T("/s")) == 0)
		{
			LOG_DEBUG(_T("got option /s"));
			proc = (PROCESS)(PROC_RESTOR_MBR
				| PROC_DELETE_TEMP);
		}	
		else if (_tcsstr(cmd_args[ii], _T("/t")) == cmd_args[ii])
		{
			m_target_drive = cmd_args[ii]+2;
			LOG_DEBUG(_T("option: target drive= %s"), m_target_drive);
		}
	}
	if (PROC_NOTHING == proc)
	{
		LOG_DEBUG(_T("default option /n"));
		proc = (PROCESS)(PROC_BACKUP_MBR 
			| PROC_EXPORT_FILE 
			| PROC_EXPORT_GRLDR_MBR
			| PROC_WRITE_MBR 
			);
	}
	return proc;
}