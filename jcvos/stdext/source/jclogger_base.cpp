
#include "../include/jclogger_base.h"
#include "../include/jcexception.h"
#include <time.h>
#include "jclogger_appenders.h"
#include "../include/single_tone.h"

#define _LOG_(...)	{			\
	TCHAR str_log[64] = {0};	\
	_stprintf_s(str_log, __VA_ARGS__);	\
	OutputDebugString(str_log); }


// {6106ECE5-CFB9-4440-A369-5DF85522C070}
const GUID CJCLoggerLocal::m_guid = 
{ 0x6106ece5, 0xcfb9, 0x4440, { 0xa3, 0x69, 0x5d, 0xf8, 0x55, 0x22, 0xc0, 0x70 } };

CJCLoggerLocal::CJCLoggerLocal(CJCLoggerAppender * appender)
    : m_appender(appender)
	, m_ts_cycle(0)
	, m_column_select( 0
		| COL_SIGNATURE
		| COL_FUNCTION_NAME 
		)
{
#ifdef WIN32
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	m_ts_cycle = 1000.0 * 1000.0 / (double)(freq.QuadPart);	// unit: us
	InitializeCriticalSection(&m_critical);
#endif

	if (m_appender == NULL)	m_appender = static_cast<CJCLoggerAppender*>(new jclogger::CDebugAppender(0) );
	JCASSERT(m_appender);
}

CJCLoggerLocal::~CJCLoggerLocal(void)
{
	OutputFunctionDuration();

    LoggerCategoryMap::iterator it = m_logger_category.begin();
    LoggerCategoryMap::iterator last_it = m_logger_category.end();
    for (; it != last_it; it++)
    {
        CJCLoggerNode * node = (it->second);
        node->Delete();
    }
    m_logger_category.clear();	
	//CleanUp();
	delete m_appender;
#ifdef WIN32
	DeleteCriticalSection(&m_critical);
#endif
}

CJCLoggerNode * CJCLoggerLocal::EnableCategory(const CJCStringW & name, int level)
{
    LoggerCategoryMap::iterator it = m_logger_category.find(name);
    if (it == m_logger_category.end() )
    {
        // Create new logger
        CJCLoggerNode * logger = CJCLoggerNode::CreateLoggerNode(name, level);
		_LOG_(_T("add logger: 0x%08X, %s, this=0x%08X\n"), (UINT32)(logger), name.c_str(), (UINT32)this )
        std::pair<LoggerCategoryMap::iterator, bool> rc;
        rc = m_logger_category.insert(LoggerCategoryMap::value_type(name, logger) );
        it = rc.first;
    }
	else
	{
		_LOG_(_T("enable logger: 0x%08X, %s, this=0x%08X\n"), 
			(UINT32)(it->second), name.c_str(), (UINT32)this )
	}
    return it->second;
}

CJCLoggerNode * CJCLoggerLocal::GetLogger(const CJCStringW & name/*, int level*/)
{
    CJCLoggerNode * logger = NULL;
    LoggerCategoryMap::iterator it = m_logger_category.find(name);
    if ( it != m_logger_category.end() )
    {
        logger = it->second;
    }
    return logger;
}

bool CJCLoggerLocal::Configurate(LPCTSTR file_name)
{
	bool br = false;
	if (NULL == file_name)	file_name = _T("jclog.cfg");
	FILE * config_file = NULL;
	_tfopen_s(&config_file, file_name, _T("r"));
	if (config_file)
	{
		br = Configurate(config_file);
		fclose(config_file);
	}
	return br;
}

void CJCLoggerLocal::ParseColumn(LPTSTR line_buf)
{
	DWORD col = 0;
	if		( 0 == _tcscmp(line_buf + 1, _T("THREAD_ID")) )		col = COL_THREAD_ID;
	else if ( 0 == _tcscmp(line_buf + 1, _T("TIME_STAMP")) )	col = COL_TIME_STAMP;
	else if ( 0 == _tcscmp(line_buf + 1, _T("COMPNENT_NAME")) )	col = COL_COMPNENT_NAME;
	else if ( 0 == _tcscmp(line_buf + 1, _T("FUNCTION_NAME")) )	col = COL_FUNCTION_NAME;
	else if ( 0 == _tcscmp(line_buf + 1, _T("REAL_TIME")) )		col = COL_REAL_TIME;
	else if ( 0 == _tcscmp(line_buf + 1, _T("REAL_DATE")) )		col = COL_REAL_DATE;
	else if ( 0 == _tcscmp(line_buf + 1, _T("SIGNATURE")) )		col = COL_SIGNATURE;

	if (_T('+') == line_buf[0])		m_column_select |= col;
	else if (_T('-') == line_buf[0] ) m_column_select &= (~col);
}

void CJCLoggerLocal::ParseAppender(LPTSTR line_buf)
{
	wchar_t * sep = NULL;
	wchar_t * app_type = line_buf + 1;
	wchar_t * str_prop = NULL;
	wchar_t * str_fn = NULL;

	DWORD prop = 0;

	// find 1st ,
	sep = _tcschr(app_type, _T(','));
	if (sep)
	{
		*sep = 0, str_prop = sep + 1;
		// find 2nd ,
		sep = _tcschr(str_prop, _T(','));
		if (sep)	*sep = 0, str_fn = sep + 1;
	}

	if (str_prop) prop = stdext::jc_str2l(str_prop, 16);
	CreateAppender(app_type, str_fn, prop);
}

void CJCLoggerLocal::ParseNode(LPTSTR line_buf)
{
	wchar_t * sep = _tcschr(line_buf, _T(','));
	if (NULL == sep) return;
	*sep = 0;

	const wchar_t * str_level = sep + 1;
	int level = 0;
	if		( 0 == _tcscmp(_T("NONE"), str_level) )			level = LOGGER_LEVEL_NONE;
	else if ( 0 == _tcscmp(_T("ALERT"), str_level) )		level = LOGGER_LEVEL_ALERT;
	else if ( 0 == _tcscmp(_T("CRITICAL"), str_level) )		level = LOGGER_LEVEL_CRITICAL;
	else if ( 0 == _tcscmp(_T("RELEASEINFO"), str_level) )	level = LOGGER_LEVEL_RELEASEINFO;
	else if ( 0 == _tcscmp(_T("ERROR"), str_level) )		level = LOGGER_LEVEL_ERROR;
	else if ( 0 == _tcscmp(_T("WARNING"), str_level) )		level = LOGGER_LEVEL_WARNING;
	else if ( 0 == _tcscmp(_T("NOTICE"), str_level) )		level = LOGGER_LEVEL_NOTICE;
	else if ( 0 == _tcscmp(_T("TRACE"), str_level) )		level = LOGGER_LEVEL_TRACE;
	else if ( 0 == _tcscmp(_T("DEBUGINFO"), str_level) )	level = LOGGER_LEVEL_DEBUGINFO;
	else if ( 0 == _tcscmp(_T("ALL"), str_level) )			level = LOGGER_LEVEL_ALL;
	else if ( _T('0') == str_level[0])						level = stdext::jc_str2l(str_level);
		
	const wchar_t * name = line_buf;
	LoggerCategoryMap::iterator it = m_logger_category.find(name);
	if (it == m_logger_category.end() )
	{
		// Create new logger
		CJCLoggerNode * logger = CJCLoggerNode::CreateLoggerNode(name, level);
		std::pair<LoggerCategoryMap::iterator, bool> rc;
		rc = m_logger_category.insert(LoggerCategoryMap::value_type(name, logger) );
		it = rc.first;
	}
	it->second->SetLevel(level);
}

bool CJCLoggerLocal::Configurate(FILE * config)
{
	JCASSERT(config);
	static const JCSIZE MAX_LINE_BUF=128;
	wchar_t * line_buf = new wchar_t[MAX_LINE_BUF];

	while (1)
	{
		if ( !_fgetts(line_buf, MAX_LINE_BUF, config) ) break;

		// Remove EOL
		wchar_t * sep = _tcschr(line_buf, _T('\n'));
		if (sep) *sep = 0;

		if ( _T('#') == line_buf[0] ) continue;
		else if ( _T('>') == line_buf[0] )	ParseAppender(line_buf);
		else if ( (_T('+') == line_buf[0]) || (_T('-') == line_buf[0]) ) ParseColumn(line_buf);
		else ParseNode(line_buf);
	}
	delete [] line_buf;
	return true;
}

bool CJCLoggerLocal::RegisterLoggerNode(CJCLoggerNode * node)
{
    const CJCStringW &node_name = node->GetNodeName();
    bool rc = true;
    LoggerCategoryMap::iterator it = m_logger_category.find(node_name);
    if ( it != m_logger_category.end() )
    {
        // check if the exist node is equalt to node
        rc = ((it->second) == node);
    }
    else
    {
        m_logger_category.insert(LoggerCategoryMap::value_type(node_name, node) );
    }
    return rc;
}

bool CJCLoggerLocal::UnregisterLoggerNode(CJCLoggerNode * node)
{
    return m_logger_category.erase(node->GetNodeName()) > 0;
}

void CJCLoggerLocal::WriteString(LPCTSTR str, JCSIZE len)
{
	EnterCriticalSection(&m_critical);
	JCASSERT(m_appender);
	m_appender->WriteString(str, len);
	LeaveCriticalSection(&m_critical);
}

void CJCLoggerLocal::CreateAppender(LPCTSTR app_type, LPCTSTR file_name, DWORD prop)
{
	// create appender
	//CleanUp();
	CJCLoggerAppender * temp = NULL;
	if (_tcscmp(_T("FILE"), app_type) == 0 )
	{
		temp = static_cast<CJCLoggerAppender*>(new jclogger::FileAppender(file_name, prop) );
	}
	else if (_tcscmp(_T("DEBUG"), app_type) == 0 )
	{
		temp = static_cast<CJCLoggerAppender*>(new jclogger::CDebugAppender(prop) );
	}
	else if (_tcscmp(_T("STDERR"), app_type) == 0)
	{
		temp = static_cast<CJCLoggerAppender*>(new jclogger::CStdErrApd );
	}
	else if (_tcscmp(_T("NONE"), app_type) == 0)
	{
		temp = static_cast<CJCLoggerAppender*>(new jclogger::CNoneApd );
	}
	if (temp)
	{
		TCHAR msg[256];
		JCSIZE len = _stprintf_s(msg, _T("[APPENDER] change to %s : %s\n"), app_type, file_name?file_name:_T(""));
		m_appender->WriteString(msg, len );
		delete m_appender;
		m_appender = temp;
	}
}

void CJCLoggerLocal::RegistFunction(const CJCStringT & func, LONGLONG duration)
{
	typedef std::pair<CJCStringT, CJCFunctionDuration>	PAIR;
	DURATION_MAP::iterator it = m_duration_map.find(func);
	if ( it == m_duration_map.end() )
	{
		CJCFunctionDuration dur(func);
		std::pair<DURATION_MAP::iterator, bool> rs = m_duration_map.insert(PAIR(func, dur));
		JCASSERT(rs.second);
		it = rs.first;
	}
	CJCFunctionDuration & dur = it->second;
	dur.m_duration += duration;
	dur.m_calls ++;
}

void CJCLoggerLocal::OutputFunctionDuration(void)
{
	TCHAR str[LOGGER_MSG_BUF_SIZE];

	DURATION_MAP::iterator it = m_duration_map.begin();
	DURATION_MAP::iterator endit = m_duration_map.end();
	for ( ; it!=endit; ++ it)
	{
		CJCFunctionDuration & dur = it->second;
		double total= dur.m_duration / GetTimeStampCycle();
		stdext::jc_sprintf(str, LOGGER_MSG_BUF_SIZE, _T("<FUN=%s> [Duration] calls=%d, total duration=%.1f ms, avg=%.1f us\n")
			, dur.m_func_name.c_str(), dur.m_calls, total / 1000.0, total / dur.m_calls);
		WriteString(str, LOGGER_MSG_BUF_SIZE);
	}
}


////////////////////////////////////////////////////////////////////////////////
// --CJCLoggerNode

double CJCLoggerNode::m_ts_cycle = CJCLogger::Instance()->GetTimeStampCycle();

CJCLoggerNode::CJCLoggerNode(const CJCStringW & name, int level, CJCLogger * logger)
    : m_category(name)
    , m_level(level)
    , m_logger(logger)
{
    if (NULL == m_logger) m_logger = CJCLogger::Instance();
    JCASSERT(m_logger);
	m_logger->RegisterLoggerNode(this);
}

CJCLoggerNode::~CJCLoggerNode(void)
{
}

void CJCLoggerNode::LogMessageFunc(LPCSTR function, LPCTSTR format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    LogMessageFuncV(function, format, argptr);
}

void CJCLoggerNode::LogMessageFuncV(LPCSTR function, LPCTSTR format, va_list arg)
{
	DWORD col_sel = m_logger->GetColumnSelect();
    TCHAR str_msg[LOGGER_MSG_BUF_SIZE];
	LPTSTR str = str_msg;
	int ir = 0, remain = LOGGER_MSG_BUF_SIZE;

	if (col_sel & CJCLoggerLocal::COL_SIGNATURE)
	{
		ir = stdext::jc_sprintf(str, remain, _T("<JC>"));
		if (ir >=0 )  str+=ir, remain-=ir;
	}

#if defined(WIN32)
	if (col_sel & CJCLoggerLocal::COL_THREAD_ID)
	{
		DWORD tid = GetCurrentThreadId();
		ir = stdext::jc_sprintf(str, remain, _T("<TID=%04d> "), tid);
		if (ir >=0 )  str+=ir, remain-=ir;
	}

	if (col_sel & CJCLoggerLocal::COL_TIME_STAMP)
	{	// 单位: us
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		double ts = now.QuadPart * m_ts_cycle;
		unsigned int inow = (unsigned int)(ts);
		ir = stdext::jc_sprintf(str, remain, _T("<TS=%u> "), inow);
		if (ir >=0 )  str+=ir, remain-=ir;
	}
#endif
	if ( (col_sel & CJCLoggerLocal::COL_REAL_TIME) || (col_sel & CJCLoggerLocal::COL_REAL_DATE) )
	{
		TCHAR strtime[32];
		time_t now = time(NULL);
		struct tm now_t;
		localtime_s(&now_t, &now);

		if (col_sel & CJCLoggerLocal::COL_REAL_DATE)
		{
			JCSIZE ll = stdext::jc_strftime(strtime, 32, _T("%Y.%m.%d"), &now_t);
			strtime[ll] = 0;
			ir = stdext::jc_sprintf(str, remain, _T("<%ls> "), strtime);
			if (ir >=0 )  str+=ir, remain-=ir;
		}

		if (col_sel & CJCLoggerLocal::COL_REAL_TIME)
		{
			JCSIZE ll = stdext::jc_strftime(strtime, 32, _T("%H:%M:%S"), &now_t);
			strtime[ll] = 0;
			ir = stdext::jc_sprintf(str, remain, _T("<%ls> "), strtime);
			if (ir >=0 )  str+=ir, remain-=ir;
		}

	}


	if (col_sel & CJCLoggerLocal::COL_COMPNENT_NAME)
	{
		ir = stdext::jc_sprintf(str, remain, _T("<COM=%ls> "), m_category.c_str());
		if (ir >=0 )  str+=ir, remain-=ir;
	}

	if (col_sel & CJCLoggerLocal::COL_FUNCTION_NAME)
	{
#if defined(WIN32)
		ir = stdext::jc_sprintf(str, remain, _T("<FUN=%S> "), function);
#else
		ir = stdext::jc_sprintf(str, remain, _T("<FUN=%s> "), function);
#endif
		if (ir >=0 )  str+=ir, remain-=ir;
	}
    ir = stdext::jc_vsprintf(str, remain, format, arg);
	if (ir >= 0 )  str+=ir, remain-=ir;
	str[0] = _T('\n'), str[1] = 0;
	str++, remain--;
	JCSIZE len = str - str_msg;
    m_logger->WriteString(str_msg, len);
}


///////////////////////////////////////////////////////////////////////////////
//-- JCStaticLoggerNode
JCStaticLoggerNode::JCStaticLoggerNode(
        const CJCStringW & name, int level)
    : CJCLoggerNode(name, level)
{
    CJCLogger * logger = CJCLogger::Instance();
    JCASSERT(logger);
    bool rc = logger->RegisterLoggerNode(this);
    JCASSERT(rc);
}

JCStaticLoggerNode::~JCStaticLoggerNode()
{
    CJCLogger * logger = CJCLogger::Instance();
    JCASSERT(logger);
    bool rc = logger->UnregisterLoggerNode(this);
    JCASSERT(rc);
}
	
///////////////////////////////////////////////////////////////////////////////
//-- CJCStackTrace

CJCStackTrace::CJCStackTrace(CJCLoggerNode *log, const char *func_name, LPCTSTR msg)
    : m_log(log)
    , m_func_name(func_name)
{
	if (log && log->GetLevel() >= LOGGER_LEVEL_TRACE)
	{
		log->LogMessageFunc(func_name, _T("[TRACE IN] %s"), msg);
	}
	// 计算函数执行时间
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	m_start_time = now.QuadPart;
}

CJCStackTrace::~CJCStackTrace(void)
{
	// 计算函数执行时间
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	m_start_time = now.QuadPart - m_start_time;
	double runtime = (m_start_time / CJCLogger::Instance()->GetTimeStampCycle());

	if (m_log && m_log->GetLevel() >= LOGGER_LEVEL_TRACE)
	{
		m_log->LogMessageFunc(m_func_name.c_str(), _T("[TRACE OUT], duration = %.1f us"), runtime);
	}

}

///////////////////////////////////////////////////////////////////////////////
// -- StackPerformance
// 用于计算特定函数被执行的总次数和总时间
CJCStackPerformance::CJCStackPerformance(LPCTSTR func_name)
	: m_func_name(func_name)
{
	// 计算函数执行时间
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	m_start_time = now.QuadPart;
}

CJCStackPerformance::~CJCStackPerformance(void)
{
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	m_start_time = now.QuadPart - m_start_time;

	if ( !m_func_name.empty() )	CJCLogger::Instance()->RegistFunction(m_func_name, m_start_time);
}

double CJCStackPerformance::GetDeltaTime(void)
{
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	LONGLONG delta = now.QuadPart - m_start_time;
	return ( delta / CJCLogger::Instance()->GetTimeStampCycle() );
}


// for test
//#pragma data_seg("Shared")
//int g_test = 10;
//#pragma data_seg()
//#pragma comment(linker, "/SECTION:Shared,RWS")