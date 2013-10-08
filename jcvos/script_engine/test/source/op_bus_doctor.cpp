#include "stdafx.h"

#include "op_bus_doctor.h"

#include <boost/spirit/include/qi.hpp>     
#include <boost/spirit/include/phoenix.hpp>     
#include <boost/bind.hpp>


static const JCSIZE MAX_LINE_BUF = 1024;


LOCAL_LOGGER_ENABLE(_T("op_bus_doctor"), LOGGER_LEVEL_DEBUGINFO);


#define __COL_CLASS_NAME CAtaTrace
BEGIN_COLUMN_TABLE()
	COLINFO_DEC( JCSIZE, 	0, m_id,			_T("store") )
	COLINFO_FLOAT( double, 	1, m_start_time,	_T("time_stamp") )
	COLINFO_FLOAT( double, 	2, m_busy_time,		_T("busy_time") )
	COLINFO_HEX( BYTE,		3, m_cmd_code,		_T("cmd_code") )
	COLINFO_HEX( JCSIZE,	4, m_lba,			_T("lba") )
	COLINFO_DEC( WORD,		5, m_sectors,		_T("sectors") )
END_COLUMN_TABLE()
#undef __COL_CLASS_NAME

LOG_CLASS_SIZE_T( jcparam::CTableRowBase<CAtaTrace>, 1 );

CBusDoctorOp::CBusDoctorOp(const CJCStringT & file_name)
	: m_src_file(NULL)
	, m_trace_tab(NULL)
{
	_tfopen_s(&m_src_file, file_name.c_str(), _T("r"));
	m_inited = false;

}

CBusDoctorOp::~CBusDoctorOp(void)
{
	if (m_src_file) fclose(m_src_file);
	if (m_trace_tab) m_trace_tab->Release();
}

bool CBusDoctorOp::GetResult(jcparam::IValue * & val)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == val);

	// 返回当前的命令
	//CAtaTraceRow * row = new CAtaTraceRow(m_trace);
	//val = static_cast<jcparam::IValue*>(row);
	if (m_trace_tab)
	{
		val = static_cast<jcparam::IValue*>(m_trace_tab);
		val->AddRef();
	}
	return true;
}

bool CBusDoctorOp::Invoke(void)
{
	//if (!m_inited) 
	//{	// 读取标题行，建立column info
	//	Init();

	//	m_inited = true;
	//	return true;
	//}
	//else
	//{
	//// 每执行一次，读取一个完整的command
	//	return InvokeOnce();
	//}

	Init();
	//bool br = true;
	//while (br)
	//{
	//	br = InvokeOnce();
	//}
	InvokeOnce();
	return true;

}

void CBusDoctorOp::SetSource(UINT src_id, IAtomOperate * op)
{
	JCASSERT(0);
}

void CBusDoctorOp::GetProgress(JCSIZE cur_prog, JCSIZE total_prog) const
{
}

void CBusDoctorOp::Init(void)
{
	JCASSERT(m_src_file);
	char line_buf[MAX_LINE_BUF];
	fgets(line_buf, MAX_LINE_BUF, m_src_file);
	fgets(line_buf, MAX_LINE_BUF, m_src_file);

	CAtaTraceTable::Create(100, m_trace_tab);
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

		using namespace qi::labels;
		using boost::phoenix::ref;

		bd_line = lit('|') >> bdf_store >> 
			'|' >> bdf_time >>
			'|' >> bdf_speed >>
			'|' >> bdf_direction >>
			'|' >> bdf_fis >>
			'|' >> bdf_descrip;

		bdf_store = int_ [ref(line_content.m_store) = _1];
		bdf_time = double_ [ref(line_content.m_time_stamp) = _1];
		bdf_speed = (float_ [ref(line_content.m_speed) = _1]) >> string("Gbps");
		bdf_direction = string("H->D") [ref(line_content.m_dir) = 1] 
			| string("D->H") [ref(line_content.m_dir) = 2] ; 

		bdf_fis = string("FIS") >> (int_ [ref(line_content.m_fis) = _1])
			>> '-' >> bdf_fis_contents;
		bdf_fis_contents = bdf_fis_cmd | bdf_fis_status | bd_any_string;
		bdf_fis_cmd = string("Cmd:") >> string("0x") 
			>> (hex [ref(line_content.m_cmd.m_code) = _1]) >> '=' >> bd_any_string;
		bdf_fis_status = string("Status:") >> string("0x") 
			>> (hex [ref(line_content.m_status.m_status) = _1]) >> bd_any_string;

		bdf_descrip = bdf_dis_cmd | bd_any_string;
		bdf_dis_cmd = string("LBA =") >> string("0x") 
			>> (hex [ref(line_content.m_cmd.m_lba) = _1]) 
			>> string("Sec Cnt =") >> string("0x") 
			>> (hex [ref(line_content.m_cmd.m_sectors) = _1]);

		//bd_fields = qi::int_ [ref(line_content.m_store) = _1];

		bd_any_string = *(char_ - '|');
	}

	//qi::rule<ITERATOR, unsigned(), ascii::space_type> bd_fields;

	qi::rule<ITERATOR, unsigned(), ascii::space_type> bdf_store, 
		bdf_time, bdf_speed, bdf_direction, bdf_fis, bdf_descrip
		, bdf_fis_contents, bdf_fis_cmd, bdf_fis_status, bdf_dis_cmd;

	qi::rule<ITERATOR> bd_any_string;

	qi::rule<ITERATOR, bus_doctor_line, ascii::space_type> bd_line;

	bus_doctor_line line_content;

	void clear()
	{
		memset(&line_content, 0, sizeof(line_content));
	}
};

//void CBusDoctorOp::ReadHeader(void)
//{
//}

bool CBusDoctorOp::InvokeOnce(void)
{
	JCASSERT(m_src_file);
	char line_buf[MAX_LINE_BUF];

	busdoctor_gm<CJCStringA::const_iterator> gm;
	CAtaTrace	trace;

	while (1)
	{
		// read a line from file
		if ( fgets(line_buf, MAX_LINE_BUF, m_src_file) == NULL ) return false;
		CJCStringA str_line(line_buf);
		// parse line
		gm.clear();
		qi::phrase_parse(str_line.begin(), str_line.end(), gm, ascii::space);
		bus_doctor_line & line = gm.line_content;

		switch (line.m_fis)
		{
		case 27:	// command
			trace.m_id = line.m_store;
			trace.m_start_time = line.m_time_stamp;
			trace.m_cmd_code = line.m_cmd.m_code;
			trace.m_lba = line.m_cmd.m_lba;
			trace.m_sectors = line.m_cmd.m_sectors;
			break;

		case 34:	// status
			trace.m_status = line.m_status.m_status;
			trace.m_busy_time = line.m_time_stamp - trace.m_start_time;
			// 单位转换
			trace.m_start_time /= (1000 * 1000 * 1000);	// ns -> s
			trace.m_busy_time /= (1000 * 1000);			// ns -> ms
			m_trace_tab->push_back(trace);
			break;

		case 39:	// dma active
		case 46:	// data
			// ignor
			break;
		}
	}
	return true;
}

void CBusDoctorOp::ParseLine(const CJCStringA & line, JCSIZE length)
{

}

#ifdef _DEBUG
// 用于检查编译结果
void CBusDoctorOp::DebugOutput(LPCTSTR indentation, FILE * outfile)
{
	stdext::jc_fprintf(outfile, indentation);
	stdext::jc_fprintf(outfile, _T("bus_doctor, [%08X], <%08X>\n"),
		(UINT)(static_cast<IAtomOperate*>(this)), (UINT)(0) );
}

#endif