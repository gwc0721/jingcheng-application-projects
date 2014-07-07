#include "stdafx.h"

#include <boost/spirit/include/qi.hpp>     
#include <boost/spirit/include/phoenix.hpp>     
#include <boost/bind.hpp>

#include <vector>

#include "parser_bus_doctor.h"

LOCAL_LOGGER_ENABLE(_T("parser_bus_hound"), LOGGER_LEVEL_WARNING);

LPCTSTR CFeatureBase<CPluginTrace::BusHound, CPluginTrace>::m_feature_name = _T("bushound");

CParamDefTab CFeatureBase<CPluginTrace::BusHound, CPluginTrace>::m_param_def_tab(
	CParamDefTab::RULE()
	(new CTypedParamDef<CJCStringT>(_T("#filename"), 0, offsetof(CPluginTrace::BusHound, m_file_name) ) )
	//(new CTypedParamDef<bool>(_T("diff_time"), _T('d'), offsetof(CPluginTrace::BusHound, m_diff_time) ) )
	);

//const UINT CPluginTrace::BusHound::_BASE::m_property = jcscript::OPP_LOOP_SOURCE;

///////////////////////////////////////////////////////////////////////////////
//-- parser
LOG_CLASS_SIZE_T(CPluginTrace::BusHound, 0)

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

CPluginTrace::BusHound::BusHound()
	: m_src_file(NULL)
	, m_line_buf(NULL)
	, m_init(false)
	, m_neof(true)
{
	m_col_phase = MAX_LINE_BUF, m_col_data = MAX_LINE_BUF, m_col_desc = MAX_LINE_BUF, m_col_id = MAX_LINE_BUF, m_col_time = MAX_LINE_BUF;
	m_working_phase = -1;
	m_line_num = 0;
	TraceQueueInit();
}

CPluginTrace::BusHound::~BusHound(void)
{
	if (m_src_file) fclose(m_src_file);
	delete [] m_line_buf;
	for (JCSIZE ii = 0; ii < TRACE_QUEUE; ++ii)
	{
		if (m_trace_que[ii]) m_trace_que[ii]->Release();
	}
}

bool CPluginTrace::BusHound::Init(void)
{
	JCASSERT(NULL == m_src_file);
	if (!m_line_buf) m_line_buf = new char[MAX_LINE_BUF];
	_tfopen_s(&m_src_file, m_file_name.c_str(), _T("r"));
	if (!m_src_file) THROW_ERROR(ERR_USER, _T("failure on openning file %s"), m_file_name.c_str() );

	// read header
	while ( 1 )
	{
		if ( fgets(m_line_buf, MAX_LINE_BUF, m_src_file) == NULL) return false;
		m_line_num ++;
		if ( strstr(m_line_buf, "Phase") != m_line_buf )	continue;
		// 找到各列的位置
		
		const char * ptr = NULL;
		if ( ptr = strstr(m_line_buf, "Phase") )			m_col_phase= ptr - m_line_buf;
		if ( ptr = strstr(m_line_buf, "Data") )				m_col_data = ptr - m_line_buf;
		if ( ptr = strstr(m_line_buf, "Description") )		m_col_desc = ptr - m_line_buf;
		if ( ptr = strstr(m_line_buf, "Cmd.Phase.Ofs") )	m_col_id   = ptr - m_line_buf;
		if ( ptr = strstr(m_line_buf, "Time") )				m_col_time = ptr - m_line_buf;
		break;
	}
	if (m_col_id >= MAX_LINE_BUF)
	{
		THROW_ERROR(ERR_USER, _T("Column Cmd.Phase.Ofs is necessary"));
	}
	fgets(m_line_buf, MAX_LINE_BUF, m_src_file);
	m_line_num ++;
	m_init = true;
	return true;
}

// 读取文件直到解析出一个phase
using namespace boost::phoenix;

static int cmd_id =0, phase_id=0, phase_offset=0;

static	qi::rule<const char*> rule_phase_id =
		(qi::int_[ref(cmd_id) = qi::_1] )>> '.' 
		>> (qi::int_ [ref(phase_id) = qi::_1] )>> '.' 
		>> (qi::int_ [ref(phase_offset) = qi::_1]) >> (-(qi::lit('(') >> qi::int_ >> ')'));

static qi::symbols<char, CPluginTrace::BusHound::PHASE_TYPE>	rule_phase_type;

class InitRulePhaseType
{
public:
	InitRulePhaseType(void)
	{
		rule_phase_type.add
			("CMD", CPluginTrace::BusHound::PT_CMD)
			("OUT", CPluginTrace::BusHound::PT_OUT)
			("IN", CPluginTrace::BusHound::PT_IN)
			("ASTS", CPluginTrace::BusHound::PT_ASTS)
			("ATA", CPluginTrace::BusHound::PT_ATA)
			("ok", CPluginTrace::BusHound::PT_OK)
			("SRB", CPluginTrace::BusHound::PT_SRB)
			("SSTS", CPluginTrace::BusHound::PT_SSTS)
			("SPT", CPluginTrace::BusHound::PT_SPT);
	}
};

static InitRulePhaseType _init_rule;

bool CPluginTrace::BusHound::ReadPhase(bus_hound_phase * & cur_phase)
{
	JCASSERT(NULL == cur_phase);

	using qi::hex;

	while (1)
	{
		// 读取并且解析一行
		if ( fgets(m_line_buf, MAX_LINE_BUF, m_src_file) == NULL)
		{
			if (m_working_phase >= 0)	
			{
				cur_phase = m_phase_buf + m_working_phase; 
				m_working_phase = -1;
			}
			return false;
		}
		const char * last = m_line_buf + strlen(m_line_buf);
		m_line_num ++;

		const char * cur_ptr = NULL;
		// phase type
		PHASE_TYPE	phase_type = PT_UNKNOWN;
		if (m_col_phase < MAX_LINE_BUF)
		{
			cur_ptr = m_line_buf + m_col_phase;
			qi::parse(cur_ptr, last, rule_phase_type, phase_type);
		}
		
		// data
		std::vector<BYTE> d;
		bool data_valid = false;
		if (m_col_data < MAX_LINE_BUF)
		{
			cur_ptr = m_line_buf + m_col_data;
			data_valid = qi::phrase_parse(cur_ptr, last, (*hex), ascii::space, d);
		}

		// desc

		// id
		if (m_col_id < MAX_LINE_BUF)
		{
			cur_ptr = m_line_buf + m_col_id;
			bool br = qi::phrase_parse(cur_ptr, last, rule_phase_id, ascii::space);
			if (!br) THROW_ERROR(ERR_USER, _T("format failed in cmd id field. at line %d"), m_line_num);
		}

		// time

		// 处理行
		bool new_phase = false;
		if (0 == phase_offset)
		{	// 开始新的phase
			if (m_working_phase >= 0)
			{	// 第n次处理phase
				cur_phase = m_phase_buf + m_working_phase;
				m_working_phase = 1-m_working_phase;
			}
			else m_working_phase = 0;
			m_phase_buf[m_working_phase].clear();
			m_phase_buf[m_working_phase].m_cmd_id = cmd_id;
			m_phase_buf[m_working_phase].m_id = phase_id;
			new_phase = true;
		}

		if (m_working_phase >= 0)
		{
			bus_hound_phase * working_phase = m_phase_buf + m_working_phase;
			if (working_phase->m_cmd_id != cmd_id || working_phase->m_id != phase_id)
			{	// error handling
			}

			if (PT_UNKNOWN != phase_type) 		working_phase->m_type = phase_type;
			if (data_valid)
			{
				JCSIZE offset = phase_offset;
				std::vector<BYTE>::iterator it, endit = d.end();
				for (it = d.begin(); (it != endit) && (offset < SECTOR_SIZE); ++it, ++offset)
					working_phase->m_data[offset] = (*it);
				working_phase->m_data_len = phase_offset + d.size();
			}
		}

		if (new_phase) break;
	}
	return true;
}

bool CPluginTrace::BusHound::ParseScsiCmd(BYTE * buf, JCSIZE len, CAtaTraceRow * trace)
{
	JCASSERT(buf);
	JCASSERT(trace);

	JCASSERT(len > 0);

	switch (buf[0])
	{
	case 0x2A:	// WRITE
		if (len < 10) THROW_ERROR(ERR_USER, _T("data format error at line %d"), m_line_num);
		trace->m_cmd_code = CMD_WRITE_DMA;
		trace->m_lba = MAKELONG(MAKEWORD(buf[5], buf[4]), MAKEWORD(buf[3], buf[2]) );
		trace->m_sectors = buf[8];
		break;

	case 0x28:
		if (len < 10) THROW_ERROR(ERR_USER, _T("data format error at line %d"), m_line_num);
		trace->m_cmd_code = CMD_READ_DMA;
		trace->m_lba = MAKELONG(MAKEWORD(buf[5], buf[4]), MAKEWORD(buf[3], buf[2]) );
		trace->m_sectors = buf[8];
		break;

	case 0x25:
	case 0x35:
	default:
		break;

	}

	return true;
}

//bool CPluginTrace::BusHound::IsRunning(void)
//{
//	return m_neof;
//}

bool CPluginTrace::BusHound::InvokeOnce(jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();

	bool neof = true;

	bus_hound_phase * phase = NULL;
	neof = ReadPhase(phase);

	if (phase/* && (PT_CMD == phase->m_type || PT_ATA == phase->m_type)*/ )
	{
		CAtaTraceRow * working_trace = NULL;
		if (1 == phase->m_id )
		{	// 新的 command
			// 如果trace不是以cmd phase开头，则忽略这个trace
			if ( (PT_CMD != phase->m_type) && (PT_ATA != phase->m_type) )	return true;

			// 如果queue满，则取出tail到 output
			if (TRACE_QUEUE == m_trq_size)
			{	// 出队列
				CAtaTraceRow * _trace =  TraceQueuePopup();
				outport->PushResult( static_cast<jcparam::ITableRow*>(_trace) );
				_trace->Release();
				LOG_DEBUG(_T("POPUP   , s=%d, h=%d, t=%d, id=%d"), m_trq_size, m_trq_head, m_trq_tail, _trace->m_id);
			}

			// 添加新的trace
			working_trace = new CAtaTraceRow;
			working_trace->m_id = phase->m_cmd_id;
			TraceQueuePushback(working_trace);
			LOG_DEBUG(_T("PUSHBACK, s=%d, h=%d, t=%d, id=%d"), m_trq_size, m_trq_head, m_trq_tail, working_trace->m_id);
		}
		else working_trace = TraceQueueSearch(phase->m_cmd_id);

		if (working_trace)
		{
			// 在queue中匹配trace id
			switch (phase->m_type)
			{
			case PT_CMD:
				// 解析CMD
				ParseScsiCmd(phase->m_data, phase->m_data_len, working_trace);
				break;

			case PT_ATA:
				if (phase->m_data_len >= 7)
				{
					BYTE * d = phase->m_data;
					working_trace->m_cmd_code = d[6];
					working_trace->m_lba = MAKELONG(MAKEWORD(d[2], d[3]), MAKEWORD(d[4], d[5] & 0xF));
					working_trace->m_sectors = d[1];
				}
				break;

			case PT_ASTS:
				if (phase->m_data_len >= 7)
				{
					BYTE * d = phase->m_data;
					working_trace->m_status = d[6];
				}
				break;

			case PT_OUT:
			case PT_IN:	{
				JCASSERT(NULL == working_trace->m_data);
				BYTE * buf = working_trace->CreateBuffer(phase->m_data_len);
				memcpy_s(buf, phase->m_data_len, phase->m_data, phase->m_data_len);
				working_trace->m_data->Unlock();
				break;	}

			case PT_OK:
			case PT_SRB:
			case PT_SSTS:
			case PT_SPT:
				break;
			default:
				THROW_ERROR(ERR_USER, _T("unknow phase type (%d) at line %d"), (int)(phase->m_type), m_line_num);
			}
		}
	}

	if (!neof)
	{
		while (m_trq_size > 0)
		{
			CAtaTraceRow * trace = TraceQueuePopup();
			outport->PushResult( static_cast<jcparam::ITableRow*>(trace) );
			trace->Release();
			LOG_DEBUG(_T("POPUP   , s=%d, h=%d, t=%d, id=%d"), m_trq_size, m_trq_head, m_trq_tail, trace->m_id);
		}

		m_neof = false;
		return false;
	}
	else return true;
}

bool CPluginTrace::BusHound::Invoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	if ( !m_init) 
	{
		Init();
		m_init = true;
	}
	return InvokeOnce(outport);
}
