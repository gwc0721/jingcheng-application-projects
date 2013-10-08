#pragma once

#include "comm_define.h"

class CJCLoggerAppender
{
public:
	enum PROPERTY
	{
		PROP_APPEND = 0x00000001,
	};

    virtual void WriteString(LPCTSTR str) = 0;
    virtual void Flush() = 0;
};

class FileAppender : public CJCLoggerAppender
{
public:
    FileAppender(LPCTSTR file_name, DWORD col, DWORD prop = 0);
    virtual ~FileAppender(void);

public:
    virtual void WriteString(LPCTSTR str);
    virtual void Flush();

#ifdef WIN32
protected:
    HANDLE m_file;
#else
protected:
    FILE * m_file;
#endif
	
};

#ifdef WIN32
class CDebugAppender : public CJCLoggerAppender
{
public:
    CDebugAppender(DWORD col, DWORD prop = 0);
    virtual ~CDebugAppender(void);

public:
    virtual void WriteString(LPCTSTR str);
    virtual void Flush();
};
#endif
