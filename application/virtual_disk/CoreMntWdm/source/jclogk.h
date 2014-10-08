// log for windows kernel
// 与user mode的jclogger相比，省去以下功能。
//	(1) 没有log node的功能，所有log只有一个node，
//  (2) 省去输出定向的功能(appender)，所有log都输出到 kdprint
#pragma once

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

#define MAX_FUNC_NAME				32		


///////////////////////////////////////////////////////////////////////////////
// -- StackTrace
// 用于跟踪函数运行，在函数进入和返回点，自动输出LOG并且计算执行时间
class CJCKStackTrace
{
public:
    CJCKStackTrace(const char * func_name, const char * msg)
	{
		KdPrint( ("[TRACE IN] %s, %s\n", func_name, msg ));
		ANSI_STRING		str_func_name;

		RtlInitAnsiString(&str_func_name, func_name);
		m_str_func.Buffer = m_func_name;
		m_str_func.MaximumLength = MAX_FUNC_NAME-1;
		RtlCopyString(&m_str_func, &str_func_name);
		m_func_name[m_str_func.Length] = 0;
	}

    ~CJCKStackTrace(void)
	{
		//KdPrint( ("[TRACE OUT1] %Z\n", m_str_func) );  !!DO NOT WORK
		KdPrint( ("[TRACE OUT] %s\n", m_func_name) );
	}
	
	//SetExitMsg(const char * msg) {}

private:
	ANSI_STRING		m_str_func;
	char	m_func_name[MAX_FUNC_NAME];
	//LONGLONG	m_start_time;
};


#if LOGGER_LEVEL >= LOGGER_LEVEL_TRACE

#undef LOG_STACK_TRACE
#define LOG_STACK_TRACE(msg)   \
    CJCKStackTrace __stack_trace__(__FUNCTION__, msg );

#endif		//LOGGER_LEVEL_TRACE


//#define LOG_CRITICAL(...)				_LOGGER_CRITICAL(_local_logger, __VA_ARGS__);
//#define LOG_RELEASE(...)				_LOGGER_RELEASE(_local_logger, __VA_ARGS__);
//#define LOG_ERROR(...)					_LOGGER_ERROR(_local_logger, __VA_ARGS__);
//#define LOG_WARNING(...)				_LOGGER_WARNING(_local_logger, __VA_ARGS__);
//#define LOG_NOTICE(...)					_LOGGER_NOTICE(_local_logger, __VA_ARGS__);
//#define LOG_TRACE(...)					_LOGGER_TRACE(_local_logger, __VA_ARGS__);
//#define LOG_DEBUG(...)					_LOGGER_DEBUG(_local_logger, 0, __VA_ARGS__)
//#define LOG_DEBUG_(lv, ...)					_LOGGER_DEBUG(_local_logger,lv, __VA_ARGS__)
//#define CLSLOG_DEBUG(classname, ...)    _LOGGER_DEBUG(_m_logger, __VA_ARGS__);
