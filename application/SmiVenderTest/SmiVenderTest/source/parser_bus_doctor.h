#pragma once

#include <jcparam.h>

#include "ata_trace.h"

#define TRACE_QUEUE		(16)
#define TQ_MASK			(TRACE_QUEUE -1)

///////////////////////////////////////////////////////////////////////////////
// 用于读取bus doctor文件的feature
class CPluginTrace::BusDoctor
	: virtual public jcscript::IFeature
	, public CFeatureBase<CPluginTrace::BusDoctor, CPluginTrace>
	, public CJCInterfaceBase
{
public:
	//typedef CFeatureBase<CPluginTrace::BusDoctor, CPluginTrace> _BASE;

public:
	BusDoctor(void);
	virtual ~BusDoctor(void);

	// IFeature
public:
	virtual bool Invoke(jcparam::IValue * row, jcscript::IOutPort * outport);
	bool InvokeOnce(jcscript::IOutPort * outport);

protected:
	virtual bool Init(void);
	void ParseLine(const CJCStringA & line, JCSIZE length);

public:
	CJCStringT m_file_name;
	bool m_diff_time;

protected:
	FILE * m_src_file;
	bool m_inited;

	// 累计时间戳，如果源文件中的时间戳以差分表示，
	//	则此变量用于累计源文件时间戳，否则等于源文件的时间戳
	double m_acc_time_stamp;
	char * m_line_buf;
	char * m_last;
	const char * m_first;

	bool m_init;
};


///////////////////////////////////////////////////////////////////////////////
// 用于读取bus hound文件的feature
class CPluginTrace::BusHound
	: virtual public jcscript::IFeature
	, public CFeatureBase<CPluginTrace::BusHound, CPluginTrace>
	, public CJCInterfaceBase
{
public:
	typedef CFeatureBase<CPluginTrace::BusHound, CPluginTrace> _BASE;

public:
	enum PHASE_TYPE
	{
		PT_CMD, PT_OUT, PT_IN, PT_ASTS, PT_ATA,
		PT_OK,	PT_SRB, PT_SSTS, PT_SPT, PT_UNKNOWN,
	};

	struct bus_hound_phase
	{
	public:
		PHASE_TYPE	m_type;
		JCSIZE		m_id, m_cmd_id;
		BYTE 		m_data[SECTOR_SIZE];
		JCSIZE		m_data_len;

		void clear(void)
		{
			m_type = PT_UNKNOWN;
			m_id = 0, m_cmd_id = 0;
			m_data_len = 0;
		};
	};

public:
	BusHound(void);
	virtual ~BusHound(void);

public:
	virtual bool Init(void);
	virtual bool Invoke(jcparam::IValue * row, jcscript::IOutPort * outport);
	bool InvokeOnce(jcscript::IOutPort * outport);

protected:
	bool ReadPhase(bus_hound_phase * &);
	bool ParseScsiCmd(BYTE * buf, JCSIZE len, CAtaTraceRow * trace);

public:
	CJCStringT m_file_name;

protected:
	FILE * m_src_file;
	bool m_init;
	bool m_neof;

	JCSIZE m_line_num;

	// 累计时间戳，如果源文件中的时间戳以差分表示，
	//	则此变量用于累计源文件时间戳，否则等于源文件的时间戳
	char * m_line_buf;

	// 列的起始位置
	JCSIZE	m_col_phase, m_col_data, m_col_desc, m_col_id, m_col_time;

	// 双缓存处理phase，使用数组以减少new开销。
	bus_hound_phase m_phase_buf[2];
	// 工作指针
	int m_working_phase;

	// 由于bus hound文件中，不同cmd之间，phase的顺序会有交错，
	// 例如：cmd 1 phase 1, cmd 2 phase 1, cmd 1 phase 2, cmd 2 phase 2 ...
	// 使用一个trace队列，保存部分过去cmd，用于填入后续phase
	CAtaTraceRow * m_trace_que[TRACE_QUEUE];
	JCSIZE m_trq_head, m_trq_tail, m_trq_size;

	inline void TraceQueueInit(void)
	{
		memset(m_trace_que, 0, sizeof(CAtaTraceRow*) * TRACE_QUEUE);
		m_trq_head = 0, m_trq_tail = 0, m_trq_size = 0;
	}

	inline CAtaTraceRow * TraceQueuePopup(void)
	{
		JCASSERT(m_trq_size > 0);
		CAtaTraceRow * row = m_trace_que[m_trq_tail & TQ_MASK];
		m_trace_que[m_trq_tail & TQ_MASK] = NULL;
		m_trq_size --, m_trq_tail++;
		return (row);
	}

	inline void TraceQueuePushback(CAtaTraceRow* row)
	{
		JCASSERT(m_trq_size < TRACE_QUEUE);
		m_trace_que[m_trq_head & TQ_MASK] = row;
		m_trq_size ++, m_trq_head++;
	}

	inline CAtaTraceRow * TraceQueueSearch(JCSIZE store)
	{
		JCASSERT( m_trq_size > 0 && m_trq_head > 0);
		if ( 0 == m_trq_size ) return NULL;
		JCSIZE ptr = m_trq_head - 1;
		for ( ; ptr >= m_trq_tail; --ptr)
		{
			if (store == m_trace_que[ptr & TQ_MASK]->m_id)
				return m_trace_que[ptr & TQ_MASK];
		}
		return NULL;
	}
};


///////////////////////////////////////////////////////////////////////////////
// 用于读取lecory sata suite文件的feature
class CPluginTrace::LeCory
	: virtual public jcscript::IFeature
	, public CFeatureBase<CPluginTrace::LeCory, CPluginTrace>
	, public CJCInterfaceBase
{
public:
	//typedef CFeatureBase<CPluginTrace::LeCory, CPluginTrace> _BASE;

	enum PARSE_SM {
		PSM_BEGIN, PSM_CONTENT, PSM_END,
	};
public:
	LeCory(void);
	virtual ~LeCory(void);

public:
	virtual bool Invoke(jcparam::IValue * row, jcscript::IOutPort * outport);
protected:
	bool Init(jcscript::IOutPort * outport);


public:
	CJCStringT m_file_name;

protected:
	FILE * m_src_file;
	bool	m_inited;
	char * m_line_buf;
	JCSIZE m_line_num;
	CAtaTraceRow * m_trace;
};

///////////////////////////////////////////////////////////////////////////////
// 用于读取lecory sata suite文件，转换成SATA FIS
class CPluginTrace::LeCoryFis
	: virtual public jcscript::IFeature
	, public CFeatureBase<CPluginTrace::LeCoryFis, CPluginTrace>
	, public CJCInterfaceBase
{
public:
	LeCoryFis(void);
	virtual ~LeCoryFis(void);

public:
	virtual bool Invoke(jcparam::IValue * row, jcscript::IOutPort * outport);
protected:
	virtual bool Init(void);
	bool ParseLine(CFis & fis);

public:
	CJCStringT m_file_name;

protected:
	bool m_init;
	FILE * m_src_file;
	char * m_line_buf;
	jcparam::CColInfoList * m_col_info;
};