#include "stdafx.h"


#include <boost/spirit/include/qi.hpp> 
#include <boost/spirit/include/phoenix.hpp>     
#include <boost/bind.hpp>
#include <vector>

#include "parser_bus_doctor.h"

LOCAL_LOGGER_ENABLE(_T("parser.lecory"), LOGGER_LEVEL_WARNING);

///////////////////////////////////////////////////////////////////////////////
// Comm parser
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
using namespace boost::phoenix;
namespace spirit = boost::spirit;

static int store_id = 0;

static qi::rule<const char*> rule_cmd_begin = 
	qi::lit("_ATA Cmd.") >> (qi::int_[ref(store_id) = qi::_1]);

enum _TIME_UNIT { THOUR, TMIN, TSEC, TMS, TUS, TNS };
typedef std::vector<int> INT_VECTOR;

class time_gm : public qi::grammar<const char *, double(), qi::locals<INT_VECTOR, _TIME_UNIT>, ascii::space_type>
{
public:

	qi::rule<const char *, double(), qi::locals<INT_VECTOR, _TIME_UNIT>, ascii::space_type> time_rule;
	qi::symbols<char, _TIME_UNIT> rule_time_unit;

	static void process_time(INT_VECTOR t)
	{
		_TIME_UNIT u;
		double time_val;
		LOG_DEBUG(_T("processing time value, %d, %d"), t.size(), u);
		for (JCSIZE ii = 0; ii<t.size(); ++ii)
		{
			switch (u)
			{
			case THOUR:	time_val += t[ii] * 60 * 60 * 1000; break;
			case TMIN:	time_val += t[ii] * 60 * 1000; break;
			case TSEC:	time_val += t[ii] * 1000; break;
			case TMS:	time_val += t[ii] ; break;
			case TUS:	time_val += t[ii] / 1000.0 ; break;
			case TNS:	time_val += t[ii] / 1000.0 / 1000.0; break;
			}
			u = (_TIME_UNIT)((int)(u) + 1);
		}
	}

	struct _cal_time_val_
	{
		template <typename T1, typename T2>
		struct result
		{
			typedef double type;
		};

		template <typename T1, typename T2>
		double operator () (T1 t, T2 u) const
		{
			// T1 : INT_VECTOR
			// T2 : TIME_UNIT
			double time_val = 0;
			LOG_DEBUG(_T("processing time value, %d, %d"), t.size(), u);
			for (JCSIZE ii = 0; ii<t.size(); ++ii)
			{
				switch (u)
				{
				case THOUR:	time_val += t[ii] * 60 * 60 * 1000; break;
				case TMIN:	time_val += t[ii] * 60 * 1000; break;
				case TSEC:	time_val += t[ii] * 1000; break;
				case TMS:	time_val += t[ii] ; break;
				case TUS:	time_val += t[ii] / 1000.0 ; break;
				case TNS:	time_val += t[ii] / 1000.0 / 1000.0; break;
				}
				u = (_TIME_UNIT)((int)(u) + 1);
			}
			return time_val;
		};
	};

	static void f1(INT_VECTOR t, _TIME_UNIT u) {
		INT_VECTOR tt = t; 
		_TIME_UNIT uu = u; 
	};


	time_gm() : time_gm::base_type(time_rule)
	{
		rule_time_unit.add
			("min", TMIN)
			("s", TSEC)
			("ms", TMS)
			("us", TUS)
			("ns", TNS);

		static boost::phoenix::function<_cal_time_val_>	cal_time_val;

		time_rule = ( (
			((qi::int_ % ".")[qi::_a = qi::_1]) >> 
			(qi::lit("(") >> (rule_time_unit[qi::_b = qi::_1]) >> qi::lit(")") ) 
			)
			[qi::_val = cal_time_val(qi::_a, qi::_b )]
			);
	}
};

static time_gm rule_time_gm;


bool ParseTimeValue(const char * &first, const char * last, double & val)
{
	val = 0;
	std::vector<int>	t;
	_TIME_UNIT	u;
	
	bool br = qi::phrase_parse(first, last, (qi::int_ % "."), ascii::space, t);
	if (!br) return false;
	
	br = qi::phrase_parse(first, last, (qi::lit("(") >> (rule_time_gm.rule_time_unit[ref(u) = qi::_1]) >> qi::lit(")")), ascii::space);
	if (!br) return false;

	for (JCSIZE ii = 0; ii<t.size(); ++ii)
	{
		switch (u)
		{
		case THOUR:	val += t[ii] * 60 * 60 * 1000; break;
		case TMIN:	val += t[ii] * 60 * 1000; break;
		case TSEC:	val += t[ii] * 1000; break;
		case TMS:	val += t[ii] ; break;
		case TUS:	val += t[ii] / 1000.0 ; break;
		case TNS:	val += t[ii] / 1000.0 / 1000.0; break;
		}
		u = (_TIME_UNIT)((int)(u) + 1);
	}
	return true;
}

#define REG_BUF_SIZE	16


bool ParseRegister(const char * &first, const char * last, BYTE * reg)
{
	typedef std::vector<UINT> REG_VECTOR;
	static qi::rule<const char *, REG_VECTOR(), ascii::space_type> reg_rule = (* qi::lit('.') ) >> ( * qi::hex);

	REG_VECTOR r;
	bool br = qi::phrase_parse(first, last, (* qi::lit('.') ) >> ( * qi::hex), ascii::space, r);
	if (!br) return false;

	JCSIZE size = min(r.size(), REG_BUF_SIZE);
	for (JCSIZE ii = 0; ii < size; ++ii)
	{
		reg[ii] = r[ii];
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// LeCory Command
LPCTSTR CFeatureBase<CPluginTrace::LeCory, CPluginTrace>::m_feature_name = _T("lecory");

CParamDefTab CFeatureBase<CPluginTrace::LeCory, CPluginTrace>::m_param_def_tab(
	CParamDefTab::RULE()
	(new CTypedParamDef<CJCStringT>(_T("#filename"), 0, offsetof(CPluginTrace::LeCory, m_file_name) ) )
	);

//const UINT CPluginTrace::LeCory::_BASE_TYPE::m_property = jcscript::OPP_LOOP_SOURCE;

LOG_CLASS_SIZE_T(CPluginTrace::LeCory, 0);

///////////////////////////////////////////////////////////////////////////////
//-- parser
CPluginTrace::LeCory::LeCory(void)
	: m_inited(false)
	, m_line_buf(NULL)
	, m_trace(NULL)
	, m_src_file(NULL)
{
	LOG_CLASS_SIZE_T(CPluginTrace::LeCory, 0);
	LOG_CLASS_SIZE(CAtaTrace)
	LOG_CLASS_SIZE(CAtaTraceRow)
}

CPluginTrace::LeCory::~LeCory(void)
{
	if (m_trace) m_trace->Release();
	if (m_src_file)	fclose(m_src_file);
	delete [] m_line_buf;
}

bool CPluginTrace::LeCory::Init(jcscript::IOutPort * outport)
{
	// open file
	JCASSERT(NULL == m_src_file);
	_tfopen_s(&m_src_file, m_file_name.c_str(), _T("r"));
	if (!m_src_file) THROW_ERROR(ERR_USER, _T("failure on openning file %s"), m_file_name.c_str() );

	if (!m_line_buf) m_line_buf = new char[MAX_LINE_BUF];
	m_line_num = 0;
	// skip head
	m_inited = true;
	return true;
}


bool CPluginTrace::LeCory::Invoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	if (!m_inited) Init(outport);
	while (1)
	{
		if ( !fgets(m_line_buf, MAX_LINE_BUF, m_src_file) )
		{
			if (m_trace)
			{
				outport->PushResult(m_trace);
				m_trace->Release(), m_trace = NULL;
			}
			return false;
		}

		const char * first = m_line_buf;
		const char * last = m_line_buf + strlen(m_line_buf);
		m_line_num ++;

		if ( qi::parse(first, last, rule_cmd_begin) )
		{
			if (m_trace)
			{
				outport->PushResult(m_trace);
				m_trace->Release(), m_trace = NULL;
			}
			m_trace = new CAtaTraceRow;
			m_trace->m_id = store_id;
			break;
		}
		else if ( first = strstr(m_line_buf, "Start time:") )
		{
			JCASSERT(m_trace);
			first += 12;
			double time_val = 0;
			bool br = qi::phrase_parse(first, last, rule_time_gm, ascii::space, time_val);
			//bool br = ParseTimeValue(first, last, time_val);
			if (br)	m_trace->m_start_time = time_val / 1000;
			else LOG_ERROR(_T("wrong time format in line %d"), m_line_num);
		}
		else if ( first = strstr(m_line_buf, "Input") )
		{
			JCASSERT(m_trace);
			first += 5;
			BYTE reg[REG_BUF_SIZE] = {0};
			bool br = ParseRegister(first, last, reg);
			if (br) 
			{
				m_trace->m_cmd_code = reg[0];
				switch (m_trace->m_cmd_code)
				{
				case CMD_READ_DMA_EX:
				case CMD_WRITE_DMA_EX:
					m_trace->m_sectors = reg[3];
					m_trace->m_lba = MAKELONG(MAKEWORD(reg[5], reg[6]), 
						MAKEWORD(reg[7], reg[8]));
					break;

				case CMD_READ_DMA:
				case CMD_WRITE_DMA:
				case CMD_READ_SECTOR:
				case CMD_WRITE_SECTOR:
					m_trace->m_feature = reg[1];
					m_trace->m_sectors = reg[2];
					m_trace->m_lba = MAKELONG(MAKEWORD(reg[3], reg[4]), 
						MAKEWORD(reg[5], reg[6] & 0x0F));

				default:
					m_trace->m_feature = reg[1];
					break;
				}
			}
			else	LOG_ERROR(_T("wrong input format in line %d"), m_line_num);

		}
		else if ( first = strstr(m_line_buf, "Output") )
		{
			JCASSERT(m_trace);
			first += 6;
			BYTE reg[REG_BUF_SIZE] = {0};
			bool br = ParseRegister(first, last, reg);
			if (br)
			{
				switch (m_trace->m_cmd_code)
				{
				case CMD_READ_DMA_EX:
					m_trace->m_status = reg[10];
					m_trace->m_error = reg[0];	
					break;

				default:
					m_trace->m_status = reg[6];
					m_trace->m_error = reg[0];
					break;
				}
			}
			else LOG_ERROR(_T("wrong output format in line %d"), m_line_num);
		}
		else if ( first = strstr(m_line_buf, "Duration Time:") )
		{
			JCASSERT(m_trace);
			first += 14;
			double time_val = 0;
			bool br = qi::phrase_parse(first, last, rule_time_gm, ascii::space, time_val);
//			bool br = ParseTimeValue(first, last, time_val);
			if (br) m_trace->m_busy_time = time_val;
			else LOG_ERROR(_T("wrong time format in line %d"), m_line_num);
		}
	}
	return true;
}


///////////////////////////////////////////////////////////////////////////////
// LeCory Fis
LPCTSTR CPluginTrace::LeCoryFis::_BASE_TYPE::m_feature_name = _T("lecoryfis");

CParamDefTab CPluginTrace::LeCoryFis::_BASE_TYPE::m_param_def_tab(
	CParamDefTab::RULE()
	(new CTypedParamDef<CJCStringT>(_T("#filename"), 0, offsetof(CPluginTrace::LeCoryFis, m_file_name) ) )
	);

class fis_type_gm : public qi::grammar<const char *, CFis, ascii::space_type>
{
public:
	fis_type_gm() : fis_type_gm::base_type(fis_type_rule)
	{
		//fis_type_rule = 
		//	( (qi::lit("Read DMA Ext; ")[ref(qi::_val.m_type) = 0x24]) >> qi::lit("LBA: ") >> qi::lit("0x") >> (qi::_hex[ref(qi::_val.m_cmd.m_lba) = qi::_1]))
		//	| ( qi::lit("Data FIS (FIS 46)")[ref(qi::_val.m_type) = 0x46] )
		//	| ( qi::lit("D->H Reg. (FIS 34)")[ref(qi::_val.m_type) = 0x34] );
		fis_type_rule = 
			( (qi::lit("Read DMA Ext; ")[ref(fis_data.m_type) = 0x27]) 
				>> qi::lit("LBA: 0x") >> (qi::hex[ref(fis_data.m_cmd.m_lba) = qi::_1]))
			| ( qi::lit("Data FIS (FIS 46)")[ref(fis_data.m_type) = 0x46] )
			| ( qi::lit("D->H Reg. (FIS 34)")[ref(fis_data.m_type) = 0x34] );
	}
	CFis fis_data;

	qi::rule<const char *, CFis, ascii::space_type> fis_type_rule;
};

static fis_type_gm	_fis_type_gm;

class fis_gm : public qi::grammar<const char *, CFis, ascii::space_type>
{
public:

	struct _set_fis_
	{
		// T1 : CFis & (input), T2 : CFis & (output)
		template <typename T1, typename T2>
		int operator () (T1 in, T2 & out) const
		{
			out.m_type = in.m_type;
			if (in.m_type == 0x24) 
			{
				out.m_cmd.m_code = in.m_cmd.m_code;
				out.m_cmd.m_lba = in.m_cmd.m_lba;
			}
		}
	} set_fis;

	fis_gm() : fis_gm::base_type(fis_line)
	{
		fis_type_rule = 
			( (qi::lit("Read DMA Ext; ")[ref(fis_data.m_type) = 0x27]) 
				>> qi::lit("LBA: 0x") >> (qi::hex[ref(fis_data.m_cmd.m_lba) = qi::_1]))
			| ( qi::lit("Data FIS (FIS 46)")[ref(fis_data.m_type) = 0x46] )
			| ( qi::lit("D->H Reg. (FIS 34)")[ref(fis_data.m_type) = 0x34] );

		fis_line = (time_rule[ref(fis_data.m_time_stamp) = qi::_1]) >> "," 
			>> (qi::lit("D1") | qi::lit("H1")) >> "," >> (fis_type_rule/*[set_fis(qi::_1, qi::_val)]*/) 
			>> "," >> (qi::lexeme[*(qi::char_)]);
	};

	time_gm	time_rule;

	qi::rule<const char *, CFis, ascii::space_type> fis_line;
	qi::rule<const char *, CFis, ascii::space_type>	fis_type_rule;

	CFis fis_data;
};

static fis_gm	_fis_gm;

CPluginTrace::LeCoryFis::LeCoryFis(void)
	: m_line_buf(NULL)
	, m_init(false)
	, m_col_info(NULL)
	, m_src_file(NULL)
{
	m_line_buf = new char[MAX_LINE_BUF];
	// create a column define
	m_col_info = new jcparam::CColInfoList;
	m_col_info->AddInfo( new jcparam::CTypedColInfo<double>(0, 0, _T("time_stamp")) );
	m_col_info->AddInfo( new jcparam::CTypedColInfo<double>(1, 0, _T("cmd_fis(us)")) );
	m_col_info->AddInfo( new jcparam::CTypedColInfo<double>(2, 0, _T("data_fis_1(us)")) );
	m_col_info->AddInfo( new jcparam::CTypedColInfo<double>(3, 0, _T("data_fis_2(us)")) );
	m_col_info->AddInfo( new jcparam::CTypedColInfo<UINT, jcparam::CHexConvertor<UINT> >(4, 0, _T("lba")) );
}

CPluginTrace::LeCoryFis::~LeCoryFis(void)
{
	delete [] m_line_buf;
	if (m_col_info) m_col_info->Release();
}

bool CPluginTrace::LeCoryFis::Init(void)
{
	JCASSERT(NULL == m_src_file);
	_tfopen_s(&m_src_file, m_file_name.c_str(), _T("r"));
	if (!m_src_file) THROW_ERROR(ERR_USER, _T("failure on openning file %s"), m_file_name.c_str() );

	//if (!m_line_buf) m_line_buf = new char[MAX_LINE_BUF];
	//m_line_num = 0;

	m_init = true;
	return true;
}

bool CPluginTrace::LeCoryFis::ParseLine(CFis & fis)
{
	if (!fgets(m_line_buf, MAX_LINE_BUF, m_src_file) ) return false;
	const char * first = m_line_buf;

	for (int ff = 0; true ; ++ff)
	{
		const char * last = strchr(first, ',');
		if (!last) break;
		switch (ff)
		{
		case 0:		{// time stamp
			double time_val = 0;
			qi::phrase_parse(first, last, rule_time_gm, ascii::space, time_val);
			fis.m_time_stamp = time_val;
			break;		}
		case 2:		{// fis type
			qi::phrase_parse(first, last, _fis_type_gm, ascii::space);
			fis.m_type = _fis_type_gm.fis_data.m_type;
			fis.m_cmd.m_lba = _fis_type_gm.fis_data.m_cmd.m_lba;
			return true;
			break;		}
		case 3:
			return true;
		}
		first = last + 1;
	}
	return true;
}

bool CPluginTrace::LeCoryFis::Invoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	if (!m_init) Init();

	enum FIS {FIS_CMD, FIS_DATA1, FIS_DATA2, FIS_STATUS};

	FIS fis_status = FIS_CMD;
	double ts0 = 0, ts =0, t_cmd = 0, t_data1 = 0, t_data2 = 0;
	JCSIZE lba = 0;
	int data_sn = 0;


	while (1)
	{
		//if (!fgets(m_line_buf, MAX_LINE_BUF, m_src_file) ) return false;
		//const char * first = m_line_buf;
		//const char * last = m_line_buf + strlen(m_line_buf);
		//bool br = qi::phrase_parse(first, last, _fis_gm, ascii::space);
		//if (!br) continue;
		//CFis & fis = _fis_gm.fis_data;

		CFis fis;
		fis.m_type = 0;
		if (!ParseLine(fis)) return false;
		
		BYTE fis_id = fis.m_type;
		switch (fis_id)
		{
		case 0:	continue; break;
		case 0x27:	// command
			ts0 = fis.m_time_stamp, ts = ts0;
			lba = fis.m_cmd.m_lba;
			break;

		case 0x46:	// data
			if (0 == data_sn)	t_cmd = fis.m_time_stamp - ts;
			else if (1 == data_sn)	t_data1 = fis.m_time_stamp - ts;
			ts = fis.m_time_stamp;;
			data_sn ++;
			break;

		case 0x34:	{// status
			if (1 == data_sn) 	t_data1 = fis.m_time_stamp - ts;
			else if (2 == data_sn)	t_data2 = fis.m_time_stamp - ts;
			TCHAR str[128];
			//	ts(s), t_cmd(us), t_data1(us), t_data2(us)
			stdext::jc_sprintf(str, _T("%g,%g,%g,%g,0x%08X"), ts0 / 1000.0, t_cmd * 1000, t_data1 * 1000, t_data2 * 1000, lba);
			jcparam::CGeneralRow * row = NULL;
			jcparam::CGeneralRow::CreateGeneralRow(m_col_info, row);
			row->SetValueText(str);
			outport->PushResult(static_cast<jcparam::ITableRow*>(row) );
			row->Release();
			return true;
			break;	}
		}

		//if (_fis_gm.fis_data.m_

		//CFis fis(_fis_gm.fis_data);


	}
	return true;
}

