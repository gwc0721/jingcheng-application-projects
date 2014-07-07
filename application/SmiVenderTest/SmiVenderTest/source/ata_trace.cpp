#include "stdafx.h"

#include <boost/spirit/include/qi.hpp>     
#include <boost/spirit/include/phoenix.hpp>     
#include <boost/bind.hpp>

#include "ata_trace.h"

#define TRACE_DATA_OUTPUT	(16)

LOCAL_LOGGER_ENABLE(_T("ata_trace"), LOGGER_LEVEL_WARNING);

// Column ID 必须按顺序递增
#define __COL_CLASS_NAME CAtaTrace
BEGIN_COLUMN_TABLE()
	COLINFO_DEC( JCSIZE, 	0,		m_id,			_T("store") )
	COLINFO_FLOAT( double, 	1,		m_start_time,	_T("time_stamp") )
	COLINFO_FLOAT( double, 	2,		m_busy_time,	_T("busy_time") )
	COLINFO_HEX( BYTE,		3,		m_cmd_code,		_T("cmd_code") )
	COLINFO_HEX( JCSIZE,	4,		m_lba,			_T("lba") )
	COLINFO_DEC( WORD,		5,		m_sectors,		_T("sectors") )
	( new CTracePayload(6, offsetof(CAtaTrace, m_data), _T("data") ) )

END_COLUMN_TABLE()
#undef __COL_CLASS_NAME

///////////////////////////////////////////////////////////////////////////////
// -- ata trace
LOG_CLASS_SIZE(CAtaTrace)
LOG_CLASS_SIZE(CAtaTraceRow)

void CTracePayload::GetText(void * row, CJCStringT & str) const
{
	void *  ptr = ((BYTE*)(row) + m_offset);
	CBinaryBuffer * _buf = *((CBinaryBuffer**)(ptr));
	//CAtaTrace * trace = reinterpret_cast<CAtaTrace*>(row);
	if (NULL == _buf) return;
	BYTE * buf = _buf->Lock();
	JCASSERT(buf);

	JCSIZE len = min(_buf->GetSize(), TRACE_DATA_OUTPUT);
	JCSIZE str_len = len * 3 + 1;

	str.resize(len * 3 + 1);
	TCHAR * _str = const_cast<TCHAR*>(str.data());
	for (JCSIZE ii = 0; ii < len; ++ii)
	{
		JCSIZE written = stdext::jc_sprintf(_str, str_len, _T("%02X "), buf[ii]);
		_str += written;
		str_len -= written;
	}
	_buf->Unlock();
}

void CTracePayload::CreateValue(BYTE * src, jcparam::IValue * & val) const
{
	CJCStringT str;
	GetText(src, str);
	val = jcparam::CTypedValue<CJCStringT>::Create(str);
}


///////////////////////////////////////////////////////////////////////////////
// -- trace parser
LOG_CLASS_SIZE_T(CPluginTrace::ParserTrace, 0)

LPCTSTR CPluginTrace::ParserTrace::_BASE::m_feature_name = _T("trace");

CParamDefTab CPluginTrace::ParserTrace::_BASE::m_param_def_tab(
	CParamDefTab::RULE()
	(new CTypedParamDef<CJCStringT>(_T("#filename"), 0, offsetof(CPluginTrace::ParserTrace, m_file_name) ) )
	);

//const UINT CPluginTrace::ParserTrace::_BASE::m_property = jcscript::OPP_LOOP_SOURCE;

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

static qi::symbols<char, CAtaTrace::COL_ID>	rule_header_type;
bool CPluginTrace::ParserTrace::s_init = false;

void CPluginTrace::ParserTrace::StaticInit(void)
{
	LOG_CLASS_SIZE( CAtaTrace );
	LOG_CLASS_SIZE_T( jcparam::CTableRowBase<CAtaTrace>, 1 );
	LOG_CLASS_SIZE(CBinaryBuffer);

	rule_header_type.add
		("store",		CAtaTrace::COL_STORE)
		("time_stamp",	CAtaTrace::COL_TIME_STAMP)
		("busy_time",	CAtaTrace::COL_BUSY_TIME)
		("cmd_code",	CAtaTrace::COL_CMD_ID)
		("cmdx",		CAtaTrace::COL_CMDX)
		("lba",			CAtaTrace::COL_LBA)
		("sectors",		CAtaTrace::COL_SECTORS)
		("data",		CAtaTrace::COL_DATA)
		("status",		CAtaTrace::COL_STATUS);
	s_init = true;
}


CPluginTrace::ParserTrace::ParserTrace()
	: m_src_file(NULL)
	, m_line_buf(NULL)
	, m_last(NULL)
	, m_first(NULL)
	, m_init(false)
	//, m_running(true)
{
	m_col_num = 0;
	memset(m_col_list, 0, sizeof(m_col_list) );

	if (!s_init) StaticInit();
}

CPluginTrace::ParserTrace::~ParserTrace(void)
{
	if (m_src_file) fclose(m_src_file);
	delete [] m_line_buf;
}

bool CPluginTrace::ParserTrace::Invoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	if ( !m_init) 
	{
		Init();
		m_init = true;
	}
	return InvokeOnce(outport);
}

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

void CPluginTrace::ParserTrace::FillFileBuffer(void)
{
	JCSIZE remain = m_last - m_first;
	if ( (remain < SRC_BACK_SIZE ) && !feof(m_src_file) )
	{
		memcpy_s(m_line_buf, m_first - m_line_buf, m_first, remain);
		m_first = m_line_buf;
		char * last = m_line_buf + remain;
		JCSIZE read_size = fread(last, 1, SRC_READ_SIZE, m_src_file);
		m_last = last + read_size;
	}
}

bool CPluginTrace::ParserTrace::ParsePayload(const char * &first, const char * last, CAtaTrace * trace)
{
	std::vector<BYTE> d;
	bool br = qi::phrase_parse(first, last, *qi::hex, ascii::space, d);
	if (!br) return false;
	if (0 == d.size()) return true;
	
	JCASSERT(NULL == trace->m_data);
	BYTE * buf = trace->CreateBuffer( d.size() );

	JCSIZE ii = 0;
	std::vector<BYTE>::iterator it, endit =d.end();
	for (it = d.begin(); it != endit; ++it, ++ii)	buf[ii] = *it;
	return true;
}

bool CPluginTrace::ParserTrace::Init(void)
{
	// 初始化文件缓存
	JCASSERT(NULL == m_src_file);
	if (!m_line_buf) m_line_buf = new char[MAX_LINE_BUF];

	_tfopen_s(&m_src_file, m_file_name.c_str(), _T("r"));
	if (!m_src_file) THROW_ERROR(ERR_USER, _T("failure on openning file %s"), m_file_name.c_str() );

	m_line = 0;

	// 解析文件头
	fgets(m_line_buf, MAX_LINE_BUF, m_src_file);	// Skip header
	m_first = m_line_buf;
	m_last = m_first + strlen(m_first);
	std::vector<CAtaTrace::COL_ID>	col_list;
	qi::phrase_parse(m_first, m_last, (rule_header_type % ','), ascii::space, col_list);
	std::vector<CAtaTrace::COL_ID>::iterator	it, endit = col_list.end();
	for (it = col_list.begin(), m_col_num = 0; it != endit; ++it, ++m_col_num)		m_col_list[m_col_num] = *it;
	// Next line
	for ( ; (*m_first != '\n' && m_first != m_last); ++m_first);
	m_line ++;
	return true;
}

using namespace boost::phoenix;

bool CPluginTrace::ParserTrace::InvokeOnce(jcscript::IOutPort * outport)
{
	//!! This function is thread UNSAFE!
	LOG_STACK_TRACE();
	JCASSERT(m_src_file);

	static int int_val;
	static qi::rule<const char*> rule_int_val( (qi::int_[ref(int_val) = qi::_1]) >> ',' );
	static double dbl_val;
	static qi::rule<const char*> rule_double_val ( (qi::double_[ref(dbl_val) = qi::_1] )>> ',');
	static UINT hex_val;
	static qi::rule<const char*> rule_hex_val ( (qi::hex[ref(hex_val) = qi::_1] )>> ',');

	// Skip header	
	if (! fgets(m_line_buf, MAX_LINE_BUF, m_src_file) ) return false;

	m_first = m_line_buf;
	m_last = m_first + strlen(m_first);

	// parse line
	CAtaTraceRow * trace = new CAtaTraceRow;
	
	for (JCSIZE ii = 0; ii < m_col_num; ++ii)
	{
		bool br = false;
		switch (m_col_list[ii])
		{
		case CAtaTrace::COL_STORE: 
			br = qi::phrase_parse(m_first, m_last, rule_int_val, ascii::space);
			trace->m_id = int_val;
			break;

		case CAtaTrace::COL_TIME_STAMP:
			br = qi::phrase_parse(m_first, m_last, rule_double_val, ascii::space);
			trace->m_start_time = dbl_val;
			break;

		case CAtaTrace::COL_BUSY_TIME:
			br = qi::phrase_parse(m_first, m_last, rule_double_val, ascii::space);
			trace->m_busy_time = dbl_val;
			break;

		case CAtaTrace::COL_CMD_ID:
			br = qi::phrase_parse(m_first, m_last, rule_hex_val, ascii::space);
			trace->m_cmd_code = (BYTE)hex_val;
			break;

		case CAtaTrace::COL_LBA:
			br = qi::phrase_parse(m_first, m_last, rule_hex_val, ascii::space);
			trace->m_lba = hex_val;
			break;

		case CAtaTrace::COL_SECTORS:
			br = qi::phrase_parse(m_first, m_last, rule_int_val, ascii::space);
			trace->m_sectors = int_val;
			break;

		case CAtaTrace::COL_DATA:
			br = ParsePayload(m_first, m_last, trace);
			break;
		}

		if (!br)
		{
			LOG_WARNING(_T("format error at line %d"), m_line);
			for ( ; (*m_first != '\n' && m_first != m_last); ++m_first);
			break;
		}
	}
	// Next line
	m_line ++;

	outport->PushResult(trace);
	trace->Release();

	if ( (m_first == m_last) && (feof(m_src_file)) )	return false;
	return true;
}










