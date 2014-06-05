// hard_link_check.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <vld.h>

#include <stdext.h>
#include <jcparam.h>
#include <jcapp.h>

#include <windows.h>
#include <set>

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

protected:
	typedef std::set<UINT64> FILE_ID_SET;
	FILE_ID_SET	m_file_id_set;

	FILESIZE	m_total_size;
	FILESIZE	m_saved_size;
	JCSIZE		m_total_groups;
	JCSIZE		m_total_files;

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

inline UINT64 MakeUint64(UINT lo, UINT hi)
{
	return 	((UINT64)hi) << 32 | (((UINT64)lo) & 0xFFFFFFFF);

}

void CHardLinkCheckApp::OnFileFound(LPCTSTR fn)
{
	LOG_DEBUG(_T("openning file %s"), fn);
	stdext::auto_handle<HANDLE> file (CreateFile(fn, 
		FILE_READ_ATTRIBUTES | FILE_READ_EA,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL) );

	if ( NULL == (HANDLE)file || INVALID_HANDLE_VALUE == (HANDLE)file)
	{
		LOG_ERROR(_T("failed on openning file %s"), fn);
		return;
	}

	BY_HANDLE_FILE_INFORMATION info;
	BOOL br = GetFileInformationByHandle(file,
		& info);

	if (!br) THROW_WIN32_ERROR(_T("failed on getting attribute from %s"), fn);

	if (info.nNumberOfLinks > 1)
	{
		UINT64 file_id = MakeUint64(info.nFileIndexLow, info.nFileIndexHigh);

		FILE_ID_SET::iterator it = m_file_id_set.find(file_id);
		// group has alread searched, skip
		if ( it != m_file_id_set.end() ) return;

		FILESIZE file_size = MakeUint64(info.nFileSizeLow, info.nFileSizeHigh);
		m_saved_size += file_size;
		m_total_size += file_size * info.nNumberOfLinks;
		m_total_groups ++;
		m_total_files += info.nNumberOfLinks;

		m_file_id_set.insert(file_id);

		TCHAR hdlink[MAX_PATH];
		_ftprintf(m_dst_file, _T("group %d: id=0x%08X%08X, size=%I64d, link(%d)\n"), 
			m_total_groups, info.nFileIndexHigh, info.nFileIndexLow, 
			file_size, info.nNumberOfLinks);

		DWORD len = MAX_PATH;
		HANDLE find = FindFirstFileNameW(fn, 0, &len, hdlink);
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
	//if (dir.at(dir.length()-1) == _T('\\'))	
	dir += _T("*.*");

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
	m_total_size = 0;
	m_saved_size = 0;
	m_total_groups = 0;
	m_total_files = 0;

	if ( m_folder.empty() )	THROW_ERROR(ERR_USER, _T("missing folder"));
	if (m_folder.at(m_folder.length()-1) != _T('\\') )	m_folder += _T("\\");
	FindAllFiles(m_folder.c_str());

	_ftprintf(m_dst_file, _T("summary: \n"));
	double total_size_mb = (double)m_total_size / (1024 * 1024);
	double saved_size_mb = (double)m_saved_size / (1024 * 1024);
	_ftprintf(m_dst_file, _T("\t total size: %.1fMiB,\t saved size: %.1fMiB,\t save %.1f %%\n"),
		total_size_mb, saved_size_mb, saved_size_mb / total_size_mb * 100);

	_ftprintf(m_dst_file, _T("\t files: %d,\t groups: %d,\t save %.1f %%\n"),
		m_total_files, m_total_groups, (double)m_total_groups / (double)m_total_files * 100);

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
