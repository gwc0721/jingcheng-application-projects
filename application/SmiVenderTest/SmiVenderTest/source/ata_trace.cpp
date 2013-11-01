#include "stdafx.h"

#include <boost/spirit/include/qi.hpp>     
#include <boost/spirit/include/phoenix.hpp>     
#include <boost/bind.hpp>

#include "ata_trace.h"

#define TRACE_DATA_OUTPUT	(16)


LOCAL_LOGGER_ENABLE(_T("ata_trace"), LOGGER_LEVEL_WARNING);

#define __COL_CLASS_NAME CAtaTrace
BEGIN_COLUMN_TABLE()
	COLINFO_DEC( JCSIZE, 	CAtaTrace::COL_STORE,			m_id,			_T("store") )
	COLINFO_FLOAT( double, 	CAtaTrace::COL_TIME_STAMP,	m_start_time,	_T("time_stamp") )
	COLINFO_FLOAT( double, 	CAtaTrace::COL_BUSY_TIME,		m_busy_time,	_T("busy_time") )
	COLINFO_HEX( BYTE,		CAtaTrace::COL_CMD_ID,		m_cmd_code,		_T("cmd_code") )
	COLINFO_HEX( JCSIZE,	CAtaTrace::COL_LBA,			m_lba,			_T("lba") )
	COLINFO_DEC( WORD,		CAtaTrace::COL_SECTORS,		m_sectors,		_T("sectors") )
	( new CAtaTrace::TracePayload(CAtaTrace::COL_DATA) )

END_COLUMN_TABLE()
#undef __COL_CLASS_NAME

///////////////////////////////////////////////////////////////////////////////
// -- ata trace

void CAtaTrace::TracePayload::GetText(void * row, CJCStringT & str) const
{
	CAtaTrace * trace = reinterpret_cast<CAtaTrace*>(row);
	if (NULL == trace->m_data) return;
	BYTE * buf = trace->m_data->Lock();
	JCASSERT(buf);

	JCSIZE len = min(trace->m_data->GetSize(), TRACE_DATA_OUTPUT);
	JCSIZE str_len = len * 3 + 1;

	str.resize(len * 3 + 1);
	TCHAR * _str = const_cast<TCHAR*>(str.data());
	for (JCSIZE ii = 0; ii < len; ++ii)
	{
		JCSIZE written = stdext::jc_sprintf(_str, str_len, _T("%02X "), buf[ii]);
		_str += written;
		str_len -= written;
	}
	trace->m_data->Unlock();
}

void CAtaTrace::TracePayload::CreateValue(BYTE * src, jcparam::IValue * & val) const
{
	CJCStringT str;
	GetText(src, str);
	val = jcparam::CTypedValue<CJCStringT>::Create(str);
}


///////////////////////////////////////////////////////////////////////////////
// -- trace parser

LPCTSTR CPluginDefault::ParserTrace::_BASE::m_feature_name = _T("trace");

CParamDefTab CPluginDefault::ParserTrace::_BASE::m_param_def_tab(
	CParamDefTab::RULE()
	(new CTypedParamDef<CJCStringT>(_T("#filename"), 0, offsetof(CPluginDefault::ParserTrace, m_file_name) ) )
	);

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

static qi::symbols<char, CAtaTrace::COL_ID>	rule_header_type;
bool CPluginDefault::ParserTrace::s_init = false;

void CPluginDefault::ParserTrace::StaticInit(void)
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
		//("",	CAtaTrace::)
		//("",	CAtaTrace::)
		//("",	CAtaTrace::)
		//("",	CAtaTrace::)

	s_init = true;
}


CPluginDefault::ParserTrace::ParserTrace()
	: m_src_file(NULL)
	, m_line_buf(NULL)
	, m_last(NULL)
	, m_first(NULL)
	, m_output(NULL)
{
	m_col_num = 0;
	memset(m_col_list, 0, sizeof(m_col_list) );

	if (!s_init) StaticInit();
}

CPluginDefault::ParserTrace::~ParserTrace(void)
{
	if (m_src_file) fclose(m_src_file);
	if (m_output)	m_output->Release();
	delete [] m_line_buf;
}

bool CPluginDefault::ParserTrace::GetResult(jcparam::IValue * & val)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == val);

	// 返回当前的命令
	if (m_output)
	{
		val = m_output;
		val->AddRef();
	}
	return true;
}

bool CPluginDefault::ParserTrace::Invoke(void)
{
	Init();
	stdext::auto_interface<CAtaTraceTable>	tab;
	CAtaTraceTable::Create(100, tab);
	while (	InvokeOnce() )
	{
		CAtaTraceRow * row = dynamic_cast<CAtaTraceRow*>(m_output);
		if ( !row ) continue;
		tab->push_back(*row);
	}
	if (m_output) m_output->Release(), m_output = NULL;
	tab.detach(m_output);
	return true;
}

void CPluginDefault::ParserTrace::GetProgress(JCSIZE &cur_prog, JCSIZE &total_prog) const
{
}


namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

//template <typename ITERATOR> 
//class trace_gm : public qi::grammar<ITERATOR, CAtaTrace>
//{
//public:
//	trace_gm() : trace_gm::base_type(trace_line)
//	{
//		using qi::lit;
//		using ascii::char_;
//		using qi::_1;
//		using qi::string;
//		using qi::int_;
//		using qi::double_;
//		using qi::float_;
//		using qi::alpha;
//		using qi::hex;
//
//		using boost::phoenix::ref;
//
//		trace_line = 
//			(int_ [ref(content.m_id) = _1]) >> lit(',') >>
//			(double_ [ref(content.m_start_time) = _1])  >> lit(',') >>
//			(double_ [ref(content.m_busy_time) = _1])  >> lit(',') >>
//			(hex [ref(content.m_cmd_code) = _1])  >> lit(',') >>
//			(hex [ref(content.m_lba) = _1])  >> lit(',') >>
//			(int_ [ref(content.m_sectors) = _1])  >> lit(',') >> lit('\n');
//	}
//	qi::rule<ITERATOR, CAtaTrace> trace_line;
//
//	CAtaTrace content;
//
//	void clear()
//	{
//		memset(&content, 0, sizeof(content));
//	}
//};
//
//static 	trace_gm<const char *> gm;

void CPluginDefault::ParserTrace::FillFileBuffer(void)
{
	JCSIZE remain = m_last - m_first;
	if ( (remain < SRC_BACK_SIZE ) && !feof(m_src_file) )
	{
		memcpy_s(m_line_buf, m_first - m_line_buf, m_first, remain);
		m_first = m_line_buf/*, m_last = m_line_buf + remain*/;
		char * last = m_line_buf + remain;
		JCSIZE read_size = fread(last, 1, SRC_READ_SIZE, m_src_file);
		m_last = last + read_size;
	}
}

bool CPluginDefault::ParserTrace::ParsePayload(const char * &first, const char * last, CAtaTrace * trace)
{
	std::vector<BYTE> d;
	//static qi::rule<const char *> rule_data ( (*qi::hex))/* >> ','*/;
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

void CPluginDefault::ParserTrace::Init(void)
{
	// 初始化文件缓存
	JCASSERT(NULL == m_src_file);
	//if (!m_line_buf) m_line_buf = new char[SRC_READ_SIZE + SRC_BACK_SIZE];
	if (!m_line_buf) m_line_buf = new char[MAX_LINE_BUF];
	//m_first = m_line_buf/*, m_last = m_line_buf*/;

	_tfopen_s(&m_src_file, m_file_name.c_str(), _T("r"));
	if (!m_src_file) THROW_ERROR(ERR_USER, _T("failure on openning file %s"), m_file_name.c_str() );

	m_line = 0;

	// 解析文件头

	//memset(m_line_buf, MAX
	fgets(m_line_buf, MAX_LINE_BUF, m_src_file);	// Skip header
	m_first = m_line_buf;
	m_last = m_first + strlen(m_first);
	//FillFileBuffer();
	std::vector<CAtaTrace::COL_ID>	col_list;
	qi::phrase_parse(m_first, m_last, (rule_header_type % ','), ascii::space, col_list);
	std::vector<CAtaTrace::COL_ID>::iterator	it, endit = col_list.end();
	for (it = col_list.begin(), m_col_num = 0; it != endit; ++it, ++m_col_num)		m_col_list[m_col_num] = *it;
	// Next line
	for ( ; (*m_first != '\n' && m_first != m_last); ++m_first);
	m_line ++;
	//m_last = m_line_buf + SRC_READ_SIZE + SRC_BACK_SIZE;
	//m_first = m_last;

}

using namespace boost::phoenix;

bool CPluginDefault::ParserTrace::InvokeOnce(void)
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

	if (m_output) m_output->Release(), m_output = NULL;

	if (! fgets(m_line_buf, MAX_LINE_BUF, m_src_file) ) return false;	// Skip header
	m_first = m_line_buf;
	m_last = m_first + strlen(m_first);
	//FillFileBuffer();

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


	//gm.clear();
	//bool br = false;
	//do
	//{	// error handling
	//	br = qi::parse<const char *, trace_gm<const char *> >(
	//		m_first, m_last, gm);
	//	if ( br || (m_first==m_last) ) break;
	//	m_first++;
	//} while (1);

	//CAtaTraceRow * trace = new CAtaTraceRow(gm.content);
	m_output = static_cast<jcparam::IValue*>(trace);

	if ( (m_first == m_last) && (feof(m_src_file)) ) return false;
	return true;
}










