
#include "../include/jclogger_base.h"
#include "../include/jcexception.h"
#include <time.h>

#include "jclogger_appenders.h"

static CJCLoggerAppender * gptr_log = NULL;

CJCLogger * CJCLogger::Instance(void)
{
#ifdef WIN32
	static jclogger::CDebugAppender		__dbg_log__;
	gptr_log = &__dbg_log__;
#endif
    static CJCLogger __logger_object__(gptr_log);
    return &__logger_object__;
}

CJCLogger::CJCLogger(CJCLoggerAppender * appender)
    : m_appender(appender)
	, m_ts_cycle(0)
	, m_column_select( 0
		//| COL_TIME_STAMP 
		//| COL_REAL_TIME 
		| COL_SIGNATURE
		| COL_FUNCTION_NAME 
		)
{
#ifdef WIN32
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	m_ts_cycle = 1000.0 * 1000.0 / (double)(freq.QuadPart);
#endif
	JCASSERT(m_appender);
}

CJCLogger::~CJCLogger(void)
{
    LoggerCategoryMap::iterator it = m_logger_category.begin();
    LoggerCategoryMap::iterator last_it = m_logger_category.end();
    for (; it != last_it; it++)
    {
        CJCLoggerNode * node = (it->second);
        node->Delete();
    }
    m_logger_category.clear();	
	CleanUp();
}

void CJCLogger::CleanUp(void)
{
	JCASSERT(m_appender);
	if ( (m_appender != gptr_log) )
	{
		delete m_appender;
		m_appender = gptr_log;
	}
}


CJCLoggerNode * CJCLogger::EnableCategory(const CJCStringW & name, int level)
{
    LoggerCategoryMap::iterator it = m_logger_category.find(name);
    if (it == m_logger_category.end() )
    {
        // Create new logger
        CJCLoggerNode * logger = CJCLoggerNode::CreateLoggerNode(name, level);
        std::pair<LoggerCategoryMap::iterator, bool> rc;
        rc = m_logger_category.insert(LoggerCategoryMap::value_type(name, logger) );
        it = rc.first;
    }
    return it->second;
}

CJCLoggerNode * CJCLogger::GetLogger(const CJCStringW & name/*, int level*/)
{
    CJCLoggerNode * logger = NULL;
    LoggerCategoryMap::iterator it = m_logger_category.find(name);
    if ( it != m_logger_category.end() )
    {
        logger = it->second;
    }
    return logger;
}

bool CJCLogger::Configurate(LPCTSTR file_name)
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

void CJCLogger::ParseColumn(LPTSTR line_buf)
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
	//else if ( 0 == _tcscmp(line_buf + 1, _T("")) )	col = 
	//else if ( 0 == _tcscmp(line_buf + 1, _T("")) )	col = 
}

void CJCLogger::ParseAppender(LPTSTR line_buf)
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

void CJCLogger::ParseNode(LPTSTR line_buf)
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

bool CJCLogger::Configurate(FILE * config)
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

bool CJCLogger::RegisterLoggerNode(CJCLoggerNode * node)
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

bool CJCLogger::UnregisterLoggerNode(CJCLoggerNode * node)
{
    return m_logger_category.erase(node->GetNodeName()) > 0;
}

void CJCLogger::WriteString(LPCTSTR str, JCSIZE len)
{
	JCASSERT(m_appender);
	m_appender->WriteString(str, len);
}

void CJCLogger::CreateAppender(LPCTSTR app_type, LPCTSTR file_name, DWORD prop)
{
	// create appender
	CleanUp();
	if (_tcscmp(_T("FILE"), app_type) == 0 )
	{
		m_appender = static_cast<CJCLoggerAppender*>(new jclogger::FileAppender(file_name, prop) );
	}
	else if (_tcscmp(_T("DEBUG"), app_type) == 0 )
	{
		m_appender = static_cast<CJCLoggerAppender*>(new jclogger::CDebugAppender(prop) );
	}
	else if (_tcscmp(_T("STDERR"), app_type) == 0)
	{
	}
	JCASSERT(m_appender)
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

	if (col_sel & CJCLogger::COL_SIGNATURE)
	{
		ir = stdext::jc_sprintf(str, remain, _T("<JC>"));
		if (ir >=0 )  str+=ir, remain-=ir;
	}

#if defined(WIN32)
	if (col_sel & CJCLogger::COL_THREAD_ID)
	{
		DWORD tid = GetCurrentThreadId();
		ir = stdext::jc_sprintf(str, remain, _T("<TID=%04d> "), tid);
		if (ir >=0 )  str+=ir, remain-=ir;
	}

	if (col_sel & CJCLogger::COL_TIME_STAMP)
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		double ts = now.QuadPart * m_ts_cycle;
		unsigned int inow = (unsigned int)(ts);
		ir = stdext::jc_sprintf(str, remain, _T("<TS=%u> "), inow);
		if (ir >=0 )  str+=ir, remain-=ir;
	}
#endif
	if (col_sel & CJCLogger::COL_REAL_TIME)
	{
		TCHAR strtime[32];
		time_t now = time(NULL);
		struct tm now_t;
		localtime_s(&now_t, &now);

		stdext::jc_strftime(strtime, 32, _T("%H:%M:%S"), &now_t);
		strtime[24] = 0;
		ir = stdext::jc_sprintf(str, remain, _T("<%ls> "), strtime);
		if (ir >=0 )  str+=ir, remain-=ir;
	}

	if (col_sel & CJCLogger::COL_COMPNENT_NAME)
	{
		ir = stdext::jc_sprintf(str, remain, _T("<COM=%ls> "), m_category.c_str());
		if (ir >=0 )  str+=ir, remain-=ir;
	}

	if (col_sel & CJCLogger::COL_FUNCTION_NAME)
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
	//stdext::jc_strcat(str, remain, _T("\n"));
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
}

CJCStackTrace::~CJCStackTrace(void)
{
	if (m_log && m_log->GetLevel() >= LOGGER_LEVEL_TRACE)
	{
		m_log->LogMessageFunc(m_func_name.c_str(), _T("[TRACE OUT]"));
	}
}

