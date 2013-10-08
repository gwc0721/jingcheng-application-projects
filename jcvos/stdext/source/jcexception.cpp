#define LOGGER_LEVEL LOGGER_LEVEL_ERROR

#include "../include/comm_define.h"
#include "../include/jchelptool.h"
#include "../include/jcexception.h"

LOCAL_LOGGER_ENABLE(CJCStringT(_T("ERROR")), LOGGER_LEVEL_ERROR);

stdext::CJCException::CJCException(
    LPCTSTR msg, stdext::CJCException::ERROR_LEVEL level, int id )
    : m_err_id(level | id)
    , m_err_msg(msg)
{
}

stdext::CJCException::CJCException(const stdext::CJCException & exp)
    : m_err_id(exp.m_err_id)
    , m_err_msg(exp.m_err_msg)
{
}

stdext::CJCException::~CJCException(void) throw()
{
}

const char * stdext::CJCException::what() const throw()
{
	JCSIZE src_len = m_err_msg.size();
	CJCStringA & tmp = const_cast<CJCStringA &>(m_msg_utf8);
	UnicodeToUtf8(tmp, m_err_msg.c_str(), src_len);
	return m_msg_utf8.c_str();
}

///////////////////////////////////////////////////////////////////////////////
//--
#ifdef WIN32
stdext::CWin32Exception::CWin32Exception(DWORD err_id, const CJCStringT& msg, stdext::CJCException::ERROR_LEVEL level)
    : CJCException(_T(""), level, (int)err_id)
{
    LPTSTR strSysMsg;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_id,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPTSTR) &strSysMsg, 
		0, NULL );
	TCHAR str_err_id[32];
	jc_sprintf(str_err_id, _T(" #%d: "), err_id);
	m_err_msg = msg + str_err_id + strSysMsg;
	LocalFree(strSysMsg);
}
#endif


///////////////////////////////////////////////////////////////////////////////
//-- Assertion

//#ifdef _DEBUG
void LogAssertion(LPCSTR source_name, int source_line, LPCTSTR str_exp)
{
#ifdef WIN32
    if (_local_logger && _local_logger->GetLevel()>= LOGGER_LEVEL_CRITICAL)
        _local_logger->LogMessageFunc(source_name, _T("<LINE=%d> [Assert] %ls"), source_line, str_exp);

	//LOG_CRITICAL(_T("[Assert] file=\"%S\", line=%d, exp=%ls"),
	//	source_name, source_line, str_exp);
#else
	LOG_CRITICAL(_T("Assert"), _T("file=\"%s\", line=%d, exp=%ls"),
		source_name, source_line, str_exp);
#endif
}

void LogException(LPCSTR function, int line, stdext::CJCException & err)
{
    if (_local_logger && _local_logger->GetLevel()>= LOGGER_LEVEL_CRITICAL)
        _local_logger->LogMessageFunc(function, _T("<LINE=%d> [Exception] %ls"), line, err.WhatT());
}

//#endif
