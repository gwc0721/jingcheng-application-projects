#include "stdafx.h"

#include <boost/spirit/include/qi.hpp>     
#include <boost/spirit/include/phoenix.hpp>     
#include <boost/bind.hpp>

#include "parser_bus_doctor.h"

LOCAL_LOGGER_ENABLE(_T("parser.bus_doctor"), LOGGER_LEVEL_WARNING);

LPCTSTR CFeatureBase<CPluginTrace::BusDoctor, CPluginTrace>::m_feature_name = _T("busdoc");

CParamDefTab CFeatureBase<CPluginTrace::BusDoctor, CPluginTrace>::m_param_def_tab(
	CParamDefTab::RULE()
	(new CTypedParamDef<CJCStringT>(_T("#filename"), 0, offsetof(CPluginTrace::BusDoctor, m_file_name) ) )
	(new CTypedParamDef<bool>(_T("diff_time"), _T('d'), offsetof(CPluginTrace::BusDoctor, m_diff_time) ) )
	);

const UINT CPluginTrace::BusDoctor::_BASE::m_property = jcscript::OPP_LOOP_SOURCE;

CPluginTrace::BusDoctor::BusDoctor()
	: m_src_file(NULL)
	, m_diff_time(false)
	, m_inited(false)
	, m_line_buf(NULL)
	, m_last(NULL)
	, m_first(NULL)
	, m_init(false)
{
}

CPluginTrace::BusDoctor::~BusDoctor(void)
{
	if (m_src_file) fclose(m_src_file);
	delete [] m_line_buf;
}

bool CPluginTrace::BusDoctor::Invoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();

	if (!m_init)	Init();
	return InvokeOnce(outport);
}

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

struct bus_doctor_line
{
public:
	JCSIZE m_store;
	double m_time_stamp;
	BYTE	m_fis;
	union
	{
		struct _cmd
		{
			BYTE	m_code;
			JCSIZE	m_lba;
			JCSIZE	m_sectors;
		} m_cmd;

		struct _status
		{
			BYTE	m_status;
			BYTE	m_error;
		} m_status;

	};
	float	m_speed;
	int		m_dir;
};

template <typename ITERATOR> 
class busdoctor_gm : public qi::grammar<ITERATOR, bus_doctor_line, ascii::space_type>
{
public:
	busdoctor_gm() : busdoctor_gm::base_type(bd_line)
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
		using qi::lexeme;

		using qi::fail;
		using qi::on_error;

		using namespace qi::labels;
		using boost::phoenix::ref;
        using boost::phoenix::construct;
        using boost::phoenix::val;

		bd_line = lit('|') >> bdf_store >> 
			'|' >> bdf_time >>
			'|' >> bdf_speed >>
			'|' >> bdf_direction >>
			'|' >> bdf_fis >>
			'|' >> bdf_dis_cmd;

		bdf_store = int_ [ref(line_content.m_store) = _1];
		bdf_time = double_ [ref(line_content.m_time_stamp) = _1];
		bdf_speed = (float_ [ref(line_content.m_speed) = _1]) >> string("Gbps");
		bdf_direction = string("H->D") [ref(line_content.m_dir) = 1] 
			| string("D->H") [ref(line_content.m_dir) = 2] ; 

		bdf_fis = string("FIS") >> (hex [ref(line_content.m_fis) = _1])
			>> '-' >> bdf_fis_contents;
		bdf_fis_contents = bdf_fis_cmd | bdf_fis_status | bd_any_string;
		bdf_fis_cmd = string("Cmd:") >> string("0x") 
			>> (hex [ref(line_content.m_cmd.m_code) = _1]) >> '=' >> bd_any_string;
		bdf_fis_status = string("Status:") >> string("0x") 
			>> (hex [ref(line_content.m_status.m_status) = _1]) >> bd_any_string;

		bdf_dis_cmd = -(string("LBA =") >> string("0x") >> (hex [ref(line_content.m_cmd.m_lba) = _1]) )
			>> -(string("Sec Cnt =") >> string("0x") >> (hex [ref(line_content.m_cmd.m_sectors) = _1]) )
			>> *(bd_any_string - '|' );

		bd_any_string %= lexeme[*(char_ - '|')];
	}

	qi::rule<ITERATOR, unsigned(), ascii::space_type> bdf_store, 
		bdf_time, bdf_speed, bdf_direction, bdf_fis, bdf_descrip
		, bdf_fis_contents, bdf_fis_cmd, bdf_fis_status, bdf_dis_cmd;

	qi::rule<ITERATOR> bd_any_string;

	qi::rule<ITERATOR, bus_doctor_line, ascii::space_type> bd_line;

	bus_doctor_line line_content;

	void clear()
	{
		memset(&line_content, 0, sizeof(line_content));
		test = 0;
	}

	int test;
};

void CPluginTrace::BusDoctor::Init(void)
{
	JCASSERT(NULL == m_src_file);
	if (!m_line_buf) m_line_buf = new char[SRC_READ_SIZE + SRC_BACK_SIZE];
	_tfopen_s(&m_src_file, m_file_name.c_str(), _T("r"));
	if (!m_src_file) THROW_ERROR(ERR_USER, _T("failure on openning file %s"), m_file_name.c_str() );

	fgets(m_line_buf, MAX_LINE_BUF, m_src_file);
	fgets(m_line_buf, MAX_LINE_BUF, m_src_file);

	m_acc_time_stamp = 0;
	m_last = m_line_buf + SRC_READ_SIZE + SRC_BACK_SIZE;
	m_first = m_last;

	m_init = true;
}

static busdoctor_gm<const char *> gm;

bool CPluginTrace::BusDoctor::InvokeOnce(jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();
	JCASSERT(m_src_file);

	// 处理一条trace
	stdext::auto_interface<CAtaTraceRow>	trace_row(new CAtaTraceRow);

	while (1)
	{		// 处理源文件1行
		JCSIZE remain = m_last - m_first;
		if ( (remain < SRC_BACK_SIZE ) && !feof(m_src_file) )
		{	// charge buffer
			LOG_DEBUG(_T("m_line_buf:<%08X>, end_buf:<%08X>"), (UINT)(m_line_buf), (UINT)(m_line_buf + MAX_LINE_BUF) );
			LOG_DEBUG(_T("m_first:<%08X>, m_last:<%08X>, remain:%X"), (UINT)(m_first), (UINT)(m_last), remain );
			LOG_DEBUG(_T("[reading]") );
			memcpy_s(m_line_buf, m_first - m_line_buf, m_first, remain);
			m_first = m_line_buf, m_last = m_line_buf + remain;
			JCSIZE read_size = fread(m_last, 1, SRC_READ_SIZE, m_src_file);
			m_last += read_size;
			LOG_DEBUG(_T("m_first:<%08X>, m_last:<%08X>, read_size:%X"), (UINT)(m_first), (UINT)(m_last), read_size );
		}

		LOG_DEBUG(_T("[parsing]") );
		gm.clear();
		bool br = qi::phrase_parse<const char *, busdoctor_gm<const char *> >(
			m_first, m_last, gm, ascii::space);
		
		LOG_DEBUG(_T("[others]") );
		bus_doctor_line & line = gm.line_content;
		if (m_diff_time)	m_acc_time_stamp += line.m_time_stamp;
		else				m_acc_time_stamp = line.m_time_stamp;

		switch (line.m_fis)
		{
		case 0x27:	// command

			trace_row->m_id = line.m_store;
			trace_row->m_start_time = m_acc_time_stamp;

			trace_row->m_cmd_code = line.m_cmd.m_code;
			trace_row->m_lba = line.m_cmd.m_lba;
			trace_row->m_sectors = line.m_cmd.m_sectors;
			break;

		case 0x34:	// status
			trace_row->m_status = line.m_status.m_status;
			trace_row->m_busy_time = m_acc_time_stamp - trace_row->m_start_time;
			// 单位转换
			trace_row->m_start_time /= (1000 * 1000 * 1000);	// ns -> s
			trace_row->m_busy_time /= (1000 * 1000);			// ns -> ms
			// 取得一个完整的trace
			outport->PushResult(static_cast<jcparam::ITableRow*>(trace_row) );

			return true;
			break;

		case 0x39:	// dma active
		case 0x46:	// data
			// ignor
			break;
		}

		// 处理未匹配部分
		LOG_DEBUG(_T("processing: %c"), *m_first);
		// '\n' != *(m_first++): 忽略其余字段，直到EOL，并且把m_first指针指向下一行首。
		for ( ; ('\n' != *(m_first++) ) && (m_first < m_last); );
		if ( (m_first >= m_last) && (feof(m_src_file)) ) return false;
	}

	return false;
}

//bool CPluginTrace::BusDoctor::IsRunning(void)
//{
//	return !( m_init && (m_first == m_last) && (feof(m_src_file)) );
//}


