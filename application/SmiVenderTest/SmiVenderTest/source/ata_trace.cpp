#include "stdafx.h"

#include <boost/spirit/include/qi.hpp>     
#include <boost/spirit/include/phoenix.hpp>     
#include <boost/bind.hpp>

#include "ata_trace.h"

LOCAL_LOGGER_ENABLE(_T("ata_trace"), LOGGER_LEVEL_DEBUGINFO);

#define __COL_CLASS_NAME CAtaTrace
BEGIN_COLUMN_TABLE()
	COLINFO_DEC( JCSIZE, 	0, m_id,			_T("store") )
	COLINFO_FLOAT( double, 	1, m_start_time,	_T("time_stamp") )
	COLINFO_FLOAT( double, 	2, m_busy_time,		_T("busy_time") )
	COLINFO_HEX( BYTE,		3, m_cmd_code,		_T("cmd_code") )
	COLINFO_HEX( JCSIZE,	4, m_lba,			_T("lba") )
	COLINFO_DEC( WORD,		5, m_sectors,		_T("sectors") )
	( new CAtaTrace::TracePayload(6) )

END_COLUMN_TABLE()
#undef __COL_CLASS_NAME

LOG_CLASS_SIZE_T( jcparam::CTableRowBase<CAtaTrace>, 1 );


LPCTSTR CPluginDefault::ParserTrace::_BASE::m_feature_name = _T("trace");

CParamDefTab CPluginDefault::ParserTrace::_BASE::m_param_def_tab(
	CParamDefTab::RULE()
	(new CTypedParamDef<CJCStringT>(_T("#filename"), 0, offsetof(CPluginDefault::ParserTrace, m_file_name) ) )
	);

CPluginDefault::ParserTrace::ParserTrace()
	: m_src_file(NULL)
	, m_line_buf(NULL)
	, m_last(NULL)
	, m_first(NULL)
	, m_output(NULL)
{
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

template <typename ITERATOR> 
class trace_gm : public qi::grammar<ITERATOR, CAtaTrace>
{
public:
	trace_gm() : trace_gm::base_type(trace_line)
	{
		using qi::lit;
		using ascii::char_;
		using qi::_1;
		using qi::string;
		using qi::int_;
		using qi::double_;
		using qi::float_;
		using qi::alpha;
		using qi::hex;

		using boost::phoenix::ref;

		trace_line = 
			(int_ [ref(content.m_id) = _1]) >> lit(',') >>
			(double_ [ref(content.m_start_time) = _1])  >> lit(',') >>
			(double_ [ref(content.m_busy_time) = _1])  >> lit(',') >>
			(hex [ref(content.m_cmd_code) = _1])  >> lit(',') >>
			(hex [ref(content.m_lba) = _1])  >> lit(',') >>
			(int_ [ref(content.m_sectors) = _1])  >> lit(',') >> lit('\n');
	}
	qi::rule<ITERATOR, CAtaTrace> trace_line;

	CAtaTrace content;

	void clear()
	{
		memset(&content, 0, sizeof(content));
	}
};

static 	trace_gm<const char *> gm;


void CPluginDefault::ParserTrace::Init(void)
{
	JCASSERT(NULL == m_src_file);
	if (!m_line_buf) m_line_buf = new char[SRC_READ_SIZE + SRC_BACK_SIZE];

	_tfopen_s(&m_src_file, m_file_name.c_str(), _T("r"));
	if (!m_src_file) THROW_ERROR(ERR_USER, _T("failure on openning file %s"), m_file_name.c_str() );
	fgets(m_line_buf, MAX_LINE_BUF, m_src_file);	// Skip header

	m_last = m_line_buf + SRC_READ_SIZE + SRC_BACK_SIZE;
	m_first = m_last;
}

bool CPluginDefault::ParserTrace::InvokeOnce(void)
{
	LOG_STACK_TRACE();
	JCASSERT(m_src_file);
	JCSIZE remain = m_last - m_first;

	if (m_output) m_output->Release(), m_output = NULL;

	if ( (remain < SRC_BACK_SIZE ) && !feof(m_src_file) )
	{	// charge buffer
		memcpy_s(m_line_buf, m_first - m_line_buf, m_first, remain);
		m_first = m_line_buf, m_last = m_line_buf + remain;
		JCSIZE read_size = fread(m_last, 1, SRC_READ_SIZE, m_src_file);
		m_last += read_size;
	}

	// parse line
	gm.clear();
	bool br = false;
	do
	{	// error handling
		br = qi::parse<const char *, trace_gm<const char *> >(
			m_first, m_last, gm);
		if ( br || (m_first==m_last) ) break;
		m_first++;
	} while (1);

	CAtaTraceRow * trace = new CAtaTraceRow(gm.content);
	m_output = static_cast<jcparam::IValue*>(trace);

	if ( (m_first == m_last) && (feof(m_src_file)) ) return false;
	return true;
}










