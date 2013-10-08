#include "stdafx.h"

#include <boost/spirit/include/qi.hpp>     
#include <boost/spirit/include/phoenix.hpp>     
#include <boost/bind.hpp>

#include "parser_bus_doctor.h"

LOCAL_LOGGER_ENABLE(_T("parser_bus_doctor"), LOGGER_LEVEL_WARNING);

LPCTSTR CFeatureBase<CPluginDefault::ParserBD, CPluginDefault>::m_feature_name = _T("parser");

CParamDefTab CFeatureBase<CPluginDefault::ParserBD, CPluginDefault>::m_param_def_tab(
	CParamDefTab::RULE()
	(new CTypedParamDef<CJCStringT>(_T("#filename"), 0, offsetof(CPluginDefault::ParserBD, m_file_name) ) )
	(new CTypedParamDef<bool>(_T("diff_time"), _T('d'), offsetof(CPluginDefault::ParserBD, m_diff_time) ) )
	);

CPluginDefault::ParserBD::ParserBD()
	: m_src_file(NULL)
	, m_trace_tab(NULL)
	, m_trace_row(NULL)
	, m_diff_time(false)
	, m_inited(false)
	, m_line_buf(NULL)
	, m_last(NULL)
	, m_first(NULL)
	, m_output(NULL)
{
}

CPluginDefault::ParserBD::~ParserBD(void)
{
	if (m_src_file) fclose(m_src_file);
	if (m_trace_tab) m_trace_tab->Release();
	if (m_trace_row) m_trace_row->Release();
	if (m_output) m_output->Release();
	delete [] m_line_buf;
}

bool CPluginDefault::ParserBD::GetResult(jcparam::IValue * & val)
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

bool CPluginDefault::ParserBD::Invoke(void)
{
	LOG_STACK_TRACE();
	
	Init();
	
	CAtaTraceTable::Create(100, m_trace_tab);
	while ( InvokeOnce() )
	{
		if (m_trace_row) m_trace_tab->push_back(*m_trace_row);
	}

	m_output = static_cast<jcparam::IValue*>(m_trace_tab);
	m_output->AddRef();
	return true;
}

void CPluginDefault::ParserBD::GetProgress(JCSIZE &cur_prog, JCSIZE &total_prog) const
{
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

void CPluginDefault::ParserBD::Init(void)
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
}

static busdoctor_gm<const char *> gm;

bool CPluginDefault::ParserBD::InvokeOnce(void)
{
	LOG_STACK_TRACE();
	JCASSERT(m_src_file);

	// 处理一条trace
	stdext::auto_interface<CAtaTraceRow>	trace_row(new CAtaTraceRow);
	if (m_trace_row)	m_trace_row->Release(), m_trace_row = NULL;

	while (1)
	{		// 处理源文件1行
		JCSIZE remain = m_last - m_first;
		if ( (remain < SRC_BACK_SIZE ) && !feof(m_src_file) )
		{	// charge buffer
			LOG_DEBUG(_T("[reading]") );
			memcpy_s(m_line_buf, m_first - m_line_buf, m_first, remain);
			m_first = m_line_buf, m_last = m_line_buf + remain;
			JCSIZE read_size = fread(m_last, 1, SRC_READ_SIZE, m_src_file);
			m_last += read_size;
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

			//if (m_trace_row) m_trace_row->Release(), m_trace_row = NULL;
			trace_row.detach(m_trace_row);
			m_output = m_trace_row;
			m_output->AddRef();
			return true;
			//m_trace_tab->push_back(trace);
			break;

		case 0x39:	// dma active
		case 0x46:	// data
			// ignor
			break;
		}

		// 处理未匹配部分
		for ( ; ('\n' != *(m_first++) ) && (m_first != m_last); );
		if ( (m_first == m_last) && (feof(m_src_file)) ) return false;
	}

	return false;
}



