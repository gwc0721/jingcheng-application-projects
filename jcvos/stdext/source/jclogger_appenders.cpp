
// end of configurations

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include "jclogger_appenders.h"

using namespace jclogger;

///////////////////////////////////////////////////////////////////////////////
// FileAppender ---------------------------------------------------------------

#ifdef WIN32
FileAppender::FileAppender(LPCTSTR file_name, DWORD prop)
    : m_file(NULL)
	, m_str_buf(NULL)
{
	DWORD flag = FILE_ATTRIBUTE_NORMAL;
	memset(&m_overlap, 0, sizeof(m_overlap));

	if (prop & CJCLoggerAppender::PROP_ASYNC)
	{
		// initialize overlap structure
		m_overlap.OffsetHigh = 0;
		m_overlap.Offset = 0;
		m_overlap.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
		if (NULL == m_overlap.hEvent) throw 0;
		m_str_buf = new TCHAR[LOGGER_MSG_BUF_SIZE];		
		flag |= FILE_FLAG_OVERLAPPED;
	}

	DWORD disp = (prop & CJCLoggerAppender::PROP_APPEND)? OPEN_ALWAYS: CREATE_ALWAYS;
	m_file = CreateFile(file_name, 
				GENERIC_READ|GENERIC_WRITE, 
				FILE_SHARE_READ | FILE_SHARE_WRITE, 
				NULL, 
				disp, 
				flag,					//如果使用NO_BUFFERING选项，文件操作必须sector对齐。
				NULL );

	if (m_file == INVALID_HANDLE_VALUE) throw 0; 
	if (prop & CJCLoggerAppender::PROP_APPEND)
	{	// 追加模式，移动文件指针到尾部。
		SetFilePointer(m_file, 0, 0, FILE_END);
	}
}

FileAppender::~FileAppender(void)
{
	if (NULL != m_overlap.hEvent)
	{
		DWORD ir = WaitForSingleObject(m_overlap.hEvent, 10000);
		CloseHandle(m_overlap.hEvent);
	}
    CloseHandle(m_file);
	delete [] m_str_buf;
}

void FileAppender::WriteString(LPCTSTR str, JCSIZE len)
{
	DWORD written = 0;
	BOOL br = FALSE;
	if (NULL != m_overlap.hEvent)
	{
		GetOverlappedResult(m_file, &m_overlap, &written, TRUE);
		m_overlap.Offset += written;
		_tcscpy_s(m_str_buf, LOGGER_MSG_BUF_SIZE, str);
		br = WriteFile(m_file, (LPCVOID) m_str_buf, len * sizeof(TCHAR), NULL, &m_overlap); 
	}
	else
	{
		br = WriteFile(m_file, (LPCVOID) str, len * sizeof(TCHAR), &written, NULL); 
		FlushFileBuffers(m_file);
	}
}

void FileAppender::Flush()
{
	FlushFileBuffers(m_file);
}

#else

FileAppender::FileAppender(LPCTSTR file_name,/* DWORD col,*/ DWORD prop)
    : m_file(NULL)
{
	stdext::jc_fopen(&m_file, file_name, _T("w+S"));
//	if (prop & CJCLoggerAppender::PROP_APPEND)
//	{	// 追加模式，移动文件指针到尾部。
//		SetFilePointer(m_file, 0, 0, FILE_END);
//	}
	//CJCLogger::Instance()->SetAppender(this);
	//CJCLogger::Instance()->SetColumnSelect(col);
}

FileAppender::~FileAppender(void)
{
    fclose(m_file);
}

void FileAppender::WriteString(LPCTSTR str)
{
    stdext::jc_fprintf(m_file, str);
}

void FileAppender::Flush()
{
}

#endif

///////////////////////////////////////////////////////////////////////////////
// stderr ----------------------------------------------------------------------
//void CStdErrApd::WriteString(LPCTSTR str, JCSIZE len)
//{
//	printf_s("%S\r", str);
//}
//
//void CStdErrApd::Flush()
//{
//}


#ifdef WIN32
///////////////////////////////////////////////////////////////////////////////
// CDebugAppender ---------------------------------------------------------------

CDebugAppender::CDebugAppender(DWORD prop)
{
#ifdef _DEBUG
	OutputDebugString(_T("create debug output\n"));
#endif
}


CDebugAppender::~CDebugAppender(void)
{
}


void CDebugAppender::WriteString(LPCTSTR str, JCSIZE)
{
	OutputDebugString(str);
}


void CDebugAppender::Flush()
{
}

#endif

