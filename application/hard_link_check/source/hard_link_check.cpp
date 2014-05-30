// hard_link_check.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <vld.h>

#include <stdext.h>
#include <jcparam.h>
#include <jcapp.h>

#include <windows.h>

LOCAL_LOGGER_ENABLE(_T("hlchk"), LOGGER_LEVEL_DEBUGINFO);


class CHardLinkCheckApp : public jcapp::CJCAppBase<jcapp::AppArguSupport>
{
public:
	CHardLinkCheckApp(void) : jcapp::CJCAppBase<jcapp::AppArguSupport>(ARGU_SUPPORT_HELP | ARGU_SUPPORT_OUTFILE) {};

public:
	bool Initialize(void);
	virtual int Run(void);
	void CleanUp(void);

protected:
	void FindAllFiles(LPCTSTR fn);
	void OnFileFound(/*const WIN32_FIND_DATA & find_data*/LPCTSTR);

public:
	CJCStringT	m_folder;
};

typedef jcapp::CJCApp<CHardLinkCheckApp>	CApplication;
static CApplication the_app;
#define _class_name_	CApplication

BEGIN_ARGU_DEF_TABLE()
	ARGU_DEF_ITEM(_T("folder"),		_T('f'), CJCStringT,	m_folder, _T("folder for check.") )
	//ARGU_DEF_ITEM(_T("length"),		_T('l'), int,		m_length, _T("set length.") )
END_ARGU_DEF_TABLE()


bool CHardLinkCheckApp::Initialize(void)
{
	OpenOutputFile();
	return true;
}

void CHardLinkCheckApp::OnFileFound(/*const WIN32_FIND_DATA & find_data*/LPCTSTR fn)
{
	LOG_DEBUG(_T("openning file %s"), fn);
	stdext::auto_handle<HANDLE> file (CreateFile(/*find_data.cFileName*/ fn, 
		FILE_READ_ATTRIBUTES | FILE_READ_EA,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL) );

	if ( NULL == (HANDLE)file || INVALID_HANDLE_VALUE == (HANDLE)file)
	{
		LOG_ERROR(_T("failed on openning file %s"), fn);
		return;
	}
		//THROW_WIN32_ERROR(_T(""), fn);

	BY_HANDLE_FILE_INFORMATION info;
	BOOL br = GetFileInformationByHandle(file,
		& info);

	if (!br) THROW_WIN32_ERROR(_T("failed on getting attribute from %s"), /*find_data.cFileName*/fn);

	if (info.nNumberOfLinks > 1)
	{
		TCHAR hdlink[MAX_PATH];
		_ftprintf(m_dst_file, _T("hard linked file 0x%08X%08X, link(%d)\n"), info.nFileIndexHigh, info.nFileIndexLow, info.nNumberOfLinks);
		DWORD len = MAX_PATH;
		HANDLE find = FindFirstFileNameW(/*find_data.cFileName*/fn, 0, &len, hdlink);
		if (!find) return;
		do
		{
			_ftprintf(m_dst_file, _T("\t-> %s\n"), hdlink);
			len = MAX_PATH;
			BOOL br = FindNextFileNameW(find, &len, hdlink);
			if (!br) break;
		} while (1);
	}
}

void CHardLinkCheckApp::FindAllFiles(LPCTSTR fn)
{
	LOG_DEBUG(_T("searching %s"), fn);
	CJCStringT dir = fn;
	if (dir.at(dir.length()-1) == _T('\\'))	dir += _T("*.*");
	WIN32_FIND_DATA		find_data;
	memset(&find_data, 0, sizeof(WIN32_FIND_DATA) );
	stdext::auto_handle<HANDLE, stdext::CCloseHandleFileFind>	find(FindFirstFile(dir.c_str(), &find_data));
	if ( (HANDLE)find == NULL || (HANDLE)find == INVALID_HANDLE_VALUE )
	{
		LOG_ERROR(_T("failed on finding %s"), fn);
		return;
		//THROW_WIN32_ERROR(_T("failed on finding %s"), fn);
	}
	do
	{
		// make full path
		CJCStringT str = fn;
		str += find_data.cFileName;

		// on file find
		if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (_tcscmp(find_data.cFileName, _T(".")) != 0 &&
				_tcscmp(find_data.cFileName, _T("..")) != 0 )
			{
				str += _T("\\");
				//str += _T("\\");
				FindAllFiles(str.c_str());
			}
		}
		else	OnFileFound(str.c_str());
		
		memset(&find_data, 0, sizeof(WIN32_FIND_DATA) );
		BOOL br = FindNextFile(find, &find_data);
		if (!br)
		{
			if (GetLastError() == ERROR_NO_MORE_FILES) break;
			else THROW_WIN32_ERROR(_T("finding file failed. "));
		}
	} while (1);
	//FindClose(find);
}

int CHardLinkCheckApp::Run(void)
{
	FindAllFiles(m_folder.c_str());
	return 0;
}

void CHardLinkCheckApp::CleanUp(void)
{
	__super::CleanUp();
}



int _tmain(int argc, _TCHAR* argv[])
{
	int ret_code = 0;
	try
	{
		the_app.Initialize();
		ret_code = the_app.Run();
	}
	catch (stdext::CJCException & err)
	{
		stdext::jc_fprintf(stderr, _T("error: %s\n"), err.WhatT() );
		ret_code = err.GetErrorID();
	}
	the_app.CleanUp();

	stdext::jc_printf(_T("Press any key to continue..."));
	getc(stdin);
	return ret_code;
}
