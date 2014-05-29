#pragma once

#include "comm_define.h"

#include <map>

#define LOGGER_MSG_BUF_SIZE     1024

#ifndef LOGGER_LEVEL
#define LOGGER_LEVEL 0
#endif

#define LOGGER_LEVEL_NONE           00
#define LOGGER_LEVEL_ALERT          10
#define LOGGER_LEVEL_CRITICAL       20
#define LOGGER_LEVEL_RELEASEINFO    30
#define LOGGER_LEVEL_ERROR          40
#define LOGGER_LEVEL_WARNING        50
#define LOGGER_LEVEL_NOTICE         60
#define LOGGER_LEVEL_TRACE          70
#define LOGGER_LEVEL_DEBUGINFO      80
#define LOGGER_LEVEL_ALL            90

class CJCLogger;

class CJCLoggerAppender
{
public:
	enum PROPERTY
	{
		PROP_APPEND = 0x00000001,
		PROP_ASYNC	= 0x00000002,
	};

	// len: legnth of str, in BYTE
	virtual ~CJCLoggerAppender() {};
    virtual void WriteString(LPCTSTR str, JCSIZE len) = 0;
    virtual void Flush() = 0;
};


class CJCLoggerNode
{
public:
    CJCLoggerNode(const CJCStringW & name, int level, CJCLogger * logger = NULL);
    virtual ~CJCLoggerNode(void);

public:
    static CJCLoggerNode * CreateLoggerNode(const CJCStringW & name, int level)
    {
        return new CJCLoggerNode(name, level);
    }

    void LogMessageFunc(LPCSTR function, LPCTSTR format, ...);
    void LogMessageFuncV(LPCSTR function, LPCTSTR format, va_list arg);

	void SetLevel(int level)	{ m_level = level; }
    int GetLevel()				{ return m_level;	}
    virtual void Delete()		{ delete this;	    }
    const CJCStringW & GetNodeName() const		{ return m_category;}

protected:
    CJCStringW m_category;
    int m_level;
    CJCLogger * m_logger;
	static double		m_ts_cycle;		// Cycle for time stamp
};

///////////////////////////////////////////////////////////////////////////////
//--
class JCStaticLoggerNode : public CJCLoggerNode
{
public:
    JCStaticLoggerNode(
        const CJCStringW & name, int level);
    virtual ~JCStaticLoggerNode();
    virtual void Delete() { };

private:
    void * operator new (size_t) 
    {
        return NULL;
    }
    void operator delete (void *) {}
};

///////////////////////////////////////////////////////////////////////////////
//--
class CJCLogger
{
public:
	enum COLUMN_SELECT
	{
		COL_THREAD_ID =		0x80000000,
		COL_TIME_STAMP =	0x40000000,
		COL_COMPNENT_NAME =	0x20000000,
		COL_FUNCTION_NAME = 0x10000000,
		COL_REAL_TIME =		0x08000000,
		COL_REAL_DATE =		0x04000000,
	};

	enum PROPERTY
	{
		PROP_APPEND = 0x00000001,
	};

public:
    typedef std::map<CJCStringW, CJCLoggerNode*> LoggerCategoryMap;
    CJCLogger(CJCLoggerAppender * appender);
    ~CJCLogger(void);
	void CleanUp(void);

    static CJCLogger * Instance(void);

	void CreateAppender(LPCTSTR app_type, LPCTSTR file_name, DWORD prop);

    bool RegisterLoggerNode(CJCLoggerNode * node);
    bool UnregisterLoggerNode(CJCLoggerNode * node);
    void WriteString(LPCTSTR str, JCSIZE len);

	DWORD GetColumnSelect(void) const	{return m_column_select;}
	void SetColumnSelect(DWORD sel)		{ m_column_select = sel; }
	void SetProperty(DWORD prop)		{ m_prop = prop; }

	double GetTimeStampCycle()			{return m_ts_cycle;}

	// Read config from text file
	//  format:
	//		set appender:	><target>,<prop hex>,<filename>...		/exp:	>FILE,2,
	//		set column:		COL|<column name>|<column name>...	/exp:
	//		set node:		<Node Name>,<Level>					/exp:	CParameter,DEBUGINFO
	bool Configurate(FILE * config);
	bool Configurate(LPCTSTR file_name = NULL);

    CJCLoggerNode * EnableCategory(const CJCStringT & name, int level);
    CJCLoggerNode * GetLogger(const CJCStringT & name);

protected:
	void ParseAppender(LPTSTR line);
	void ParseNode(LPTSTR line);


protected:
    LoggerCategoryMap m_logger_category;
    CJCLoggerAppender * m_appender;
	DWORD m_column_select;
	DWORD m_prop;
	double m_ts_cycle;
};

///////////////////////////////////////////////////////////////////////////////
//-- StackTrace


class CJCStackTrace
{
public:
    CJCStackTrace(CJCLoggerNode * log, const char * func_name, LPCTSTR msg);
    ~CJCStackTrace(void);

private:
    CJCLoggerNode * m_log;
	CJCStringA m_func_name;
};

///////////////////////////////////////////////////////////////////////////////
//-- Class Size
template <typename TYPE>
class CJCLogClassSize
{
public:
	CJCLogClassSize(const wchar_t * class_name, CJCLoggerNode * log)
	{
		if (log)		log->LogMessageFunc(("[ClassSize]"), _T("sizeof(%ls)\t\t= %d"), 
			class_name, (UINT)(sizeof(TYPE)) );
	}
};



///////////////////////////////////////////////////////////////////////////////
//bool CreateLogAppender(LPCTSTR app_type, LPCTSTR file_name, DWORD col_list, DWORD prop); 


// 以下方法实现的问题是：__logger 是一个局部对象，而不是CJCLogger中的__logger_object__对象
// 实际上appender并没有关联到全局__logger中。
// 以下是解决方法。但是这样的话，就无法实现为每个模块制定不同的logfile了。

#define LOGGER_INIT(app_type, file_name, prop)   \
	CJCLogger::Instance()->CreateAppender(app_type, file_name, prop);

#define LOGGER_CLEANUP	\
	CJCLogger::Instance()->CleanUp();

#define LOGGER_SELECT_COL(cols)	\
	CJCLogger::Instance()->SetColumnSelect(cols);

#define LOGGER_CONFIG(file_name) \
	CJCLogger::Instance()->Configurate(file_name);


#define LOCAL_LOGGER_ENABLE(name, level)  \
    static CJCLoggerNode * _local_logger = CJCLogger::Instance()->EnableCategory(name, level);

#define CLASS_LOGGER_ENABLE(classname, level)


#define DECLARE_CLASS_LOGGER(classname) \
    JCStaticLoggerNode __class_logger_##classname;

#define INIT_CLASS_LOGGER(classname, level) \
    __class_logger_##classname(_T(#classname), level)
	
#define CLASS_LOGGER(classname) \
    CJCLoggerNode * _local_logger = static_cast<CJCLoggerNode*>(  \
        &__class_logger_##classname);



#define _LOGGER_CRITICAL( ... )
#define _LOGGER_RELEASE( ... )
#define _LOGGER_ERROR( ... )
#define _LOGGER_WARNING( ... )
#define _LOGGER_NOTICE( ... )
#define _LOGGER_TRACE( ... )
#define _LOGGER_DEBUG( ... )	{}
#define _LOGGER_TRACE( ... )
#define LOG_STACK_TRACE( ... )
#define LOG_STACK_TRACE_P( ... )
#define LOG_STACK_TRACE_O( ... )
#define LOG_SIMPLE_TRACE( ... )


#define LOG_CLASS_SIZE( ... )
#define LOG_CLASS_SIZE_T( ... )
#define LOG_CLASS_SIZE_T1( ... )

#ifdef	LOG_OUT_CLASS_SIZE

#undef LOG_CLASS_SIZE
#define LOG_CLASS_SIZE(type)  CJCLogClassSize<type> _size_of_##type(_T(#type), _local_logger);

#undef LOG_CLASS_SIZE_T
#define LOG_CLASS_SIZE_T(type, sn)	CJCLogClassSize<type> _size_of_##sn(_T(#type), _local_logger);

#undef LOG_CLASS_SIZE_T1
#define LOG_CLASS_SIZE_T1(class_name, type1)	\
	CJCLogClassSize< class_name<type1> >		\
	_size_of_##class_name_##type1(_T(#class_name)_T("<")_T(#type1)_T(">"), _local_logger);

#endif


#if LOGGER_LEVEL >= LOGGER_LEVEL_CRITICAL

#undef _LOGGER_CRITICAL
#define _LOGGER_CRITICAL( _logger, ...) { \
    if (_logger && _logger->GetLevel()>= LOGGER_LEVEL_CRITICAL)    \
        _logger->LogMessageFunc(__FUNCTION__, __VA_ARGS__);   \
    }

#if LOGGER_LEVEL >= LOGGER_LEVEL_RELEASEINFO

#undef _LOGGER_RELEASE
#define _LOGGER_RELEASE( _logger, ...) { \
    if (_logger && _logger->GetLevel()>= LOGGER_LEVEL_RELEASEINFO)    \
        _logger->LogMessageFunc(__FUNCTION__, __VA_ARGS__);   \
    }

#if LOGGER_LEVEL >= LOGGER_LEVEL_ERROR

#undef _LOGGER_ERROR
#define _LOGGER_ERROR( _logger, ...) { \
    if (_logger && _logger->GetLevel()>= LOGGER_LEVEL_ERROR)    \
        _logger->LogMessageFunc(__FUNCTION__, __VA_ARGS__);   \
    }

#if LOGGER_LEVEL >= LOGGER_LEVEL_WARNING

#undef _LOGGER_WARNING
#define _LOGGER_WARNING( _logger, ...) { \
    if (_logger && _logger->GetLevel()>= LOGGER_LEVEL_WARNING)    \
        _logger->LogMessageFunc(__FUNCTION__, __VA_ARGS__);   \
    }


#if LOGGER_LEVEL >= LOGGER_LEVEL_NOTICE
#undef _LOGGER_NOTICE
#define _LOGGER_NOTICE( _logger, ...) { \
    if (_logger && _logger->GetLevel()>= LOGGER_LEVEL_NOTICE)    \
        _logger->LogMessageFunc(__FUNCTION__, __VA_ARGS__);   \
    }


#if LOGGER_LEVEL >= LOGGER_LEVEL_TRACE


#undef LOG_STACK_TRACE
#define LOG_STACK_TRACE()   \
    CJCStackTrace __stack_trace__(_local_logger, __FUNCTION__, _T("") );

#undef LOG_STACK_TRACE_P
#define LOG_STACK_TRACE_P(...)   \
	LPTSTR __temp_str = new TCHAR[512];		\
	stdext::jc_sprintf(__temp_str, 512, __VA_ARGS__);	\
    CJCStackTrace __stack_trace__(_local_logger, __FUNCTION__, __temp_str );	\
	delete [] __temp_str;

#undef LOG_STACK_TRACE_O
#define LOG_STACK_TRACE_O(...)   \
	LPTSTR __temp_str = new TCHAR[512];		\
	int __ir = stdext::jc_sprintf(__temp_str, 512, _T("pt=0x%08X"), (UINT)(this) );	\
	stdext::jc_sprintf(__temp_str+__ir, 512-__ir, __VA_ARGS__);	\
    CJCStackTrace __stack_trace__(_local_logger, __FUNCTION__, __temp_str );	\
	delete [] __temp_str;

#undef LOGGER_SIMPLE_TRACE
#define LOGGER_SIMPLE_TRACE( _logger, ... ) {  \
    if (_logger && _logger->GetLevel()>= LOGGER_LEVEL_TRACE)    \
		LPTSTR __temp_str = new TCHAR[512];		\
		int __ir = stdext::jc_sprintf(__temp_str, 512, _T("[TRACE]") );	\
		stdext::jc_sprintf(__temp_str+__ir, 512-__ir, __VA_ARGS__);	\
        _logger->LogMessageFunc((__FUNCTION__), __temp_str);   \
		delete [] __temp_str;	\
    }

#undef _LOGGER_TRACE
#define _LOGGER_TRACE( _logger, ... ) {  \
    if (_logger && _logger->GetLevel()>= LOGGER_LEVEL_TRACE)    \
        _logger->LogMessageFunc((__FUNCTION__), __VA_ARGS__);   \
    }

#if LOGGER_LEVEL >= LOGGER_LEVEL_DEBUGINFO

#undef  _LOGGER_DEBUG
#define _LOGGER_DEBUG( _logger, ... ) {  \
    if (_logger && _logger->GetLevel()>= LOGGER_LEVEL_DEBUGINFO)    \
        _logger->LogMessageFunc((__FUNCTION__), __VA_ARGS__);   \
    }

#endif      //LOGGER_LEVEL_DEBUGINFO
#endif		//LOGGER_LEVEL_TRACE
#endif		//LOGGER_LEVEL_NOTICE
#endif		//LOGGER_LEVEL_WARING
#endif      //LOGGER_LEVEL_ERROR
#endif      //LOGGER_LEVEL_RELEASEINFO
#endif		// LOGGER_LECEL_CRETICAL

#define FULL_LOG_DEBUG(name, ...)    {   \
    CJCLoggerNode * _logger = CJCLogger::Instance()->GetLogger(name); \
    _LOGGER_DEBUG(_logger, __VA_ARGS__)    \
    }

#define LOG_CRITICAL(...)				_LOGGER_CRITICAL(_local_logger, __VA_ARGS__);
#define LOG_RELEASE(...)				_LOGGER_RELEASE(_local_logger, __VA_ARGS__);
#define LOG_ERROR(...)					_LOGGER_ERROR(_local_logger, __VA_ARGS__);
#define LOG_WARNING(...)				_LOGGER_WARNING(_local_logger, __VA_ARGS__);
#define LOG_NOTICE(...)					_LOGGER_NOTICE(_local_logger, __VA_ARGS__);
#define LOG_TRACE(...)					_LOGGER_TRACE(_local_logger, __VA_ARGS__);
#define LOG_DEBUG(...)					_LOGGER_DEBUG(_local_logger, __VA_ARGS__)
#define CLSLOG_DEBUG(classname, ...)    _LOGGER_DEBUG(_m_logger, __VA_ARGS__);

//#define THROW_WIN32_ERROR
