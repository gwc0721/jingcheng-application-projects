
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include "../include/jclogger_base.h"

///////////////////////////////////////////////////////////////////////////////
// FileAppender ---------------------------------------------------------------

#ifdef WIN32
FileAppender::FileAppender(LPCTSTR file_name, DWORD col, DWORD prop)
    : m_file(NULL)
{
	m_file = CreateFile(file_name, 
				GENERIC_READ|GENERIC_WRITE, 
				FILE_SHARE_READ | FILE_SHARE_WRITE, 
				NULL, 
				CREATE_ALWAYS, 
				FILE_ATTRIBUTE_NORMAL,		//如果使用NO_BUFFERING选项，文件操作必须sector对齐。
				NULL );
	if (m_file == INVALID_HANDLE_VALUE) throw 0; 
	if (prop & CJCLoggerAppender::PROP_APPEND)
	{	// 追加模式，移动文件指针到尾部。
		SetFilePointer(m_file, 0, 0, FILE_END);
	}

	CJCLogger::Instance()->SetAppender(this);
	CJCLogger::Instance()->SetColumnSelect(col);
}

FileAppender::~FileAppender(void)
{
    CloseHandle(m_file);
}

void FileAppender::WriteString(LPCTSTR str)
{
	DWORD written = 100;
	DWORD len = (DWORD)(_tcslen(str) * sizeof(TCHAR));
	BOOL br = WriteFile(m_file, (LPCVOID) str, len, &written, NULL); 
	if (! br)
	{
		DWORD err = GetLastError();
	}
	FlushFileBuffers(m_file);
}

void FileAppender::Flush()
{
	FlushFileBuffers(m_file);
}

#else

FileAppender::FileAppender(LPCTSTR file_name, DWORD col, DWORD prop)
    : m_file(NULL)
{
	stdext::jc_fopen(&m_file, file_name, _T("w+S"));
//	if (prop & CJCLoggerAppender::PROP_APPEND)
//	{	// 追加模式，移动文件指针到尾部。
//		SetFilePointer(m_file, 0, 0, FILE_END);
//	}
	CJCLogger::Instance()->SetAppender(this);
	CJCLogger::Instance()->SetColumnSelect(col);
}

FileAppender::~FileAppender(void)
{
    fclose(m_file);
}

void FileAppender::WriteString(LPCTSTR str)
{
    //_SASSERT(m_file);
    stdext::jc_fprintf(m_file, str);
//    stdext::jc_fprintf(m_file, _T("\n"));
}

void FileAppender::Flush()
{
}

#endif

///////////////////////////////////////////////////////////////////////////////
// ----------------------------------------------------------------------------

#ifdef WIN32
///////////////////////////////////////////////////////////////////////////////
// CDebugAppender ---------------------------------------------------------------

CDebugAppender::CDebugAppender(DWORD col, DWORD prop)
{
	CJCLogger::Instance()->SetAppender(this);
	CJCLogger::Instance()->SetColumnSelect(col);
}


CDebugAppender::~CDebugAppender(void)
{
}


void CDebugAppender::WriteString(LPCTSTR str)
{
	OutputDebugString(str);
}


void CDebugAppender::Flush()
{
}

#endif

