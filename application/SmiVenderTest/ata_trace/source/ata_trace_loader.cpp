
#define LOGGER_LEVEL LOGGER_LEVEL_DEBUGINFO
#include "../include/ata_trace_data.h"
#include "stdafx.h"

#include <boost/spirit/include/qi.hpp>     
#include <boost/spirit/include/phoenix.hpp>     
#include <boost/bind.hpp>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
using namespace boost::phoenix;

LOCAL_LOGGER_ENABLE(_T("ata_trace"), LOGGER_LEVEL_DEBUGINFO);

LOG_CLASS_SIZE(CAtaTrace)
LOG_CLASS_SIZE(CAtaTraceRow)

#define __COL_CLASS_NAME CAtaTrace
BEGIN_COLUMN_TABLE()
	COLINFO_DEC( JCSIZE, 	0,		m_id,			_T("store") )
	COLINFO_FLOAT( double, 	1,		m_start_time,	_T("time_stamp") )
	COLINFO_FLOAT( double, 	2,		m_busy_time,	_T("busy_time") )
	COLINFO_HEX( BYTE,		3,		m_cmd_code,		_T("cmd_code") )
	COLINFO_HEX( JCSIZE,	4,		m_lba,			_T("lba") )
	COLINFO_DEC( WORD,		5,		m_sectors,		_T("sectors") )
	//( new CTracePayload(6, offsetof(CAtaTrace, m_data), _T("data") ) )

END_COLUMN_TABLE()
#undef __COL_CLASS_NAME


///////////////////////////////////////////////////////////////////////////////
//----
static qi::symbols<char, CAtaTrace::COL_ID>	rule_header_type;
bool CAtaTraceLoader::m_init = false;


CAtaTraceLoader::CAtaTraceLoader(void)
: m_col_num(0)
, m_file_mapping(NULL)
{
	if (m_init == false)
	{
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
		m_init = true;
	}
}

CAtaTraceLoader::~CAtaTraceLoader(void)
{
	if (m_file_mapping)	m_file_mapping->Release();
}

bool CAtaTraceLoader::Initialize(const CJCStringT & data_file, const char * str)
{
	LOG_STACK_TRACE();
	// set binary file name
	//m_data_fn = fn + _T(".bin");
	CreateFileMappingObject(data_file, m_file_mapping);
	JCASSERT(m_file_mapping);

	const char * str_first = str;
	const char * str_last = str + strlen(str);

	std::vector<CAtaTrace::COL_ID>	col_list;
	qi::phrase_parse(str_first, str_last, (rule_header_type % ','), ascii::space, col_list);
	std::vector<CAtaTrace::COL_ID>::iterator	it, endit = col_list.end();
	for (it = col_list.begin(), m_col_num = 0; it != endit; ++it, ++m_col_num)		m_col_list[m_col_num] = *it;
	LOG_DEBUG(_T("parsed %d headers"), m_col_num);
	return true;
}

bool CAtaTraceLoader::Initialize(jcparam::IJCStream * stream)
{
	return true;
}

bool CAtaTraceLoader::ParseLine(const char * str, CAtaTraceRow * & _trace)
{
	//!! This function is thread UNSAFE!
	LOG_STACK_TRACE();
	JCASSERT(_trace == NULL);

	static int int_val;
	static qi::rule<const char*> rule_int_val( (qi::int_[ref(int_val) = qi::_1]) >> ',' );
	static double dbl_val;
	static qi::rule<const char*> rule_double_val ( (qi::double_[ref(dbl_val) = qi::_1] )>> ',');
	static UINT hex_val;
	static qi::rule<const char*> rule_hex_val ( (qi::hex[ref(hex_val) = qi::_1] )>> ',');

	// Skip header	
	const char * str_first = str;
	const char * str_last = str + strlen(str);

	// parse line
	stdext::auto_interface<CAtaTraceRow> trace(new CAtaTraceRow);
	//CAtaTraceRow * trace = new CAtaTraceRow;
	
	for (JCSIZE ii = 0; ii < m_col_num; ++ii)
	{
		bool br = false;
		switch (m_col_list[ii])
		{
		case CAtaTrace::COL_STORE: 
			br = qi::phrase_parse(str_first, str_last, rule_int_val, ascii::space);
			trace->m_id = int_val;
			break;

		case CAtaTrace::COL_TIME_STAMP:
			br = qi::phrase_parse(str_first, str_last, rule_double_val, ascii::space);
			trace->m_start_time = dbl_val;
			break;

		case CAtaTrace::COL_BUSY_TIME:
			br = qi::phrase_parse(str_first, str_last, rule_double_val, ascii::space);
			trace->m_busy_time = dbl_val;
			break;

		case CAtaTrace::COL_CMD_ID:
			br = qi::phrase_parse(str_first, str_last, rule_hex_val, ascii::space);
			trace->m_cmd_code = (BYTE)hex_val;
			break;

		case CAtaTrace::COL_LBA:
			br = qi::phrase_parse(str_first, str_last, rule_hex_val, ascii::space);
			trace->m_lba = hex_val;
			break;

		case CAtaTrace::COL_SECTORS:
			br = qi::phrase_parse(str_first, str_last, rule_int_val, ascii::space);
			trace->m_sectors = int_val;
			break;

		case CAtaTrace::COL_DATA:
			br = ParsePayload(str_first, str_last, static_cast<CAtaTrace*>(trace) );
			break;
		}

		if (!br)
		{
			LOG_WARNING(_T("format error at line "));
			return false;
			//for ( ; (*m_first != '\n' && m_first != m_last); ++m_first);
			//break;
		}
	}
	// Next line
	//m_line ++;
	trace.detach(_trace);

	//outport->PushResult(trace);
	//trace->Release();

	//if ( (m_first == m_last) && (feof(m_src_file)) )	return false;
	return true;
}

//bool ParseLine(jcparam::IJCStream * stream);

bool CAtaTraceLoader::ParsePayload(const char * &first, const char * last, CAtaTrace * trace)
{
	static int offset, secs;
	static qi::rule<const char*> rule_int_val(
		(qi::lit("<#")) >> (qi::hex[ref(offset) = qi::_1]) >> (qi::lit(";")) >> 
		(qi::int_[ref(secs) = qi::_1]) >> (qi::lit(">,")) );

	jcparam::IBinaryBuffer * buf = NULL;
	CreateFileMappingBuf(m_file_mapping, offset, secs, buf);
	trace->m_data = buf;
	return true;
}
