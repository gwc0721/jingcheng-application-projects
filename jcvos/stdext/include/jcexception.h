#pragma once

#include "jclogger_base.h"
#include <string>

namespace stdext
{
    class CJCException : public std::exception
    {
    public:
        //typedef std::basic_string<char> StringT;
	    // 将错误分成5个等级，1：无错误，2：警告，3：用户错误，4：应用程序错误，5：系统错误。
	    //	错误等级越高，对系统危害越大，恢复越困难。
	    //	1：无错误 - 无错误，不需要处理
	    //	2：警告 - 对程序的运行没有影响，程序可以继续运行
	    //	3：用户错误 - 由用户的不正确操作或者输入引起的错误。停止当前部分作业，要求用户重新输入。
	    //	4：应用程序错误 - 通常由应用程序设计的BUG引起，或者发生预料之外的状态。通常停止当前操作或者停止应用程序。
	    //	5：系统错误 - 一般包括内存、文件、网络等错误。通常停止应用程序。有时可以只停止当前操作，但未必能够恢复。
	    enum ERROR_LEVEL
	    {
		    ERR_OK =		0,
		    ERR_WARNING	=	0x01000000,
		    ERR_PARAMETER = 0x02000000,
		    // 用户错误：不是由程序本引起的错误。由用户不当操作、或者不正确的输入引起的错误。
		    //	这类错误通常是可恢复的。在记录错误时，不记录错误发生的源代码位置。
		    ERR_USER =		0x03000000,		
		    ERR_APP =		0x04000000,
			ERR_UNSUPPORT = 0x04800000,
			ERR_DEVICE =	0x05000000,
		    ERR_CLIENT =	0x06000000,
		    ERR_SERVER =	0x07000000,
		    ERR_SYSTEM =	0x08000000,
	    };    

    public:
        // All the string of exception should use UTF8
        virtual const char * what() const throw();
		LPCTSTR WhatT() const
		{
			return m_err_msg.c_str();
		};

    public:
		CJCException(LPCTSTR msg, ERROR_LEVEL level = ERR_APP, int id = 0);
        CJCException(const CJCException & exp);
		int GetErrorID(void) { return m_err_id;}
        virtual ~CJCException(void) throw();

    public:
    protected:
        int             m_err_id;   // The heighest bit is autodelete
        CJCStringT      m_err_msg;
		CJCStringA		m_msg_utf8;
    };

////////////
// for win32 only
#ifdef WIN32
    class CWin32Exception : public CJCException
    {
    public:
        CWin32Exception(DWORD err_id, const CJCStringT& msg, ERROR_LEVEL level = ERR_APP);
    };
#endif

};

extern "C"
{
	void LogException(LPCSTR function, int line, stdext::CJCException & err);
}

#define THROW_ERROR(level, ...)   {					\
		LPTSTR __temp_str = new TCHAR[512];			\
		stdext::jc_sprintf(__temp_str, 512, __VA_ARGS__);	\
		stdext::CJCException err(__temp_str, stdext::CJCException::level); \
		delete [] __temp_str;						\
        LogException(__FUNCTION__, __LINE__, err);	\
        throw err; }

#ifdef WIN32
#define THROW_WIN32_ERROR(msg, ...)   {									\
		LPTSTR __temp_str = new TCHAR[512];								\
		stdext::jc_sprintf(__temp_str, 512, msg, __VA_ARGS__);					\
        stdext::CWin32Exception err(GetLastError(), __temp_str);		\
 		delete [] __temp_str;											\
        LogException(__FUNCTION__, __LINE__, err);					\
        throw err;  \
    }

#define THROW_WIN32_ERROR_(errid, msg, ...)   {							\
		LPTSTR __temp_str = new TCHAR[512];								\
		stdext::jc_sprintf(__temp_str, 512, msg, __VA_ARGS__);					\
        stdext::CWin32Exception err(errid, __temp_str);					\
 		delete [] __temp_str;											\
        LogException(__FUNCTION__, __LINE__, err);					\
        throw err;  \
    }
#endif
