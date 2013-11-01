#pragma once

#include <jcparam.h>

#include "plugin_default.h"
#include "feature_base.h"
#include <SmiDevice.h>

#define CMD_WRITE_DMA		(0xCA)
#define CMD_READ_DMA		(0xC8)
#define CMD_WRITE_SECTOR	(0x30)

#define MAX_COLUMN			(20)


class CAtaTrace
{
public:
	class TracePayload;	// 用于格式化输出IN/OUT的data
public:
	CAtaTrace(void) : m_data(NULL) {memset(this, 0, sizeof(CAtaTrace));}
	CAtaTrace(const CAtaTrace & trace) 
		: m_id(trace.m_id), m_start_time(trace.m_start_time), m_busy_time(trace.m_busy_time)
		, m_featue(trace.m_featue), m_cmd_code(trace.m_cmd_code), m_lba(trace.m_lba)
		, m_sectors(trace.m_sectors), m_status(trace.m_status), m_data(trace.m_data) 
	{if (m_data) m_data->AddRef();}
	~CAtaTrace(void) {if (m_data) m_data->Release(); }

public:
	enum COL_ID
	{
		COL_UNKNOW = 0, COL_STORE, COL_TIME_STAMP, COL_BUSY_TIME, COL_CMD_ID, 
		COL_CMDX, COL_LBA, COL_SECTORS, COL_STATUS, COL_DATA,
	};

public:
	JCSIZE	m_id;			// store
	double	m_start_time;	// time stamp in sec
	double	m_busy_time;	// command's busy time
	BYTE	m_featue;
	BYTE	m_cmd_code;
	JCSIZE	m_lba;
	JCSIZE	m_sectors;
	BYTE	m_status;
	CBinaryBuffer	* m_data;

public:
	BYTE * CreateBuffer(JCSIZE size)
	{
		//JCASSERT(NULL == m_data);
		CBinaryBuffer::Create(size, m_data);
		return m_data->Lock();
	}
	//BYTE	* m_data;
	//JCSIZE	m_data_len;
};

class CAtaTrace::TracePayload	: public jcparam::CColInfoBase
{
public:
	TracePayload(int id) : jcparam::CColInfoBase(id, jcparam::VT_OTHERS, offsetof(CAtaTrace, m_data), _T("data")) {};

public:
	virtual void GetText(void * row, CJCStringT & str) const;
	virtual void CreateValue(BYTE * src, jcparam::IValue * & val) const;
};

typedef jcparam::CTableRowBase<CAtaTrace>	CAtaTraceRow;
typedef jcparam::CTypedTable<CAtaTrace>		CAtaTraceTable;



///////////////////////////////////////////////////////////////////////////////
// -- trace parser
class CPluginDefault::ParserTrace
	: virtual public jcscript::ILoopOperate
	, public CFeatureBase<CPluginDefault::ParserTrace, CPluginDefault>
	, public CJCInterfaceBase
{
public:
	typedef CFeatureBase<CPluginDefault::ParserTrace, CPluginDefault> _BASE;

public:


public:
	ParserTrace(void);
	virtual ~ParserTrace(void);

// IAtomOperate
public:
	virtual bool GetResult(jcparam::IValue * & val);
	virtual bool Invoke(void);

// ILoopOperate
public:
	virtual void GetProgress(JCSIZE &cur_prog, JCSIZE &total_prog) const;
	virtual void Init(void);
	virtual bool InvokeOnce(void);

protected:
	void ParseLine(const CJCStringA & line, JCSIZE length);
	static void StaticInit(void);
	void FillFileBuffer(void);
	bool ParsePayload(const char * &first, const char * last, CAtaTrace * trace);

public:
	CJCStringT m_file_name;

protected:
	FILE * m_src_file;

	char * m_line_buf;
	const char * m_first;
	const char * m_last;

	jcparam::IValue * m_output;

	// 通过解析头得道的列表
	CAtaTrace::COL_ID	m_col_list[MAX_COLUMN];
	JCSIZE	m_col_num;		// 文件中的列数
	//std::vector<CAtaTrace::COL_ID>	m_col_list;

	// 源文件的行号
	JCSIZE m_line;

protected:
	static bool s_init;
};