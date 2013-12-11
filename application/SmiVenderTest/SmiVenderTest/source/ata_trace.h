#pragma once

#include <jcparam.h>

#include "plugin_trace.h"
#include "feature_base.h"
#include <SmiDevice.h>

#define CMD_WRITE_DMA		(0xCA)
#define CMD_READ_DMA		(0xC8)
#define CMD_WRITE_SECTOR	(0x30)
#define CMD_READ_SECTOR		(0x20)

#define MAX_COLUMN			(20)

///////////////////////////////////////////////////////////////////////////////
// vender command
class CVenderCmd
{
public:
	CVenderCmd(void);

public:
	WORD	m_cmd_id;	// 0, 1
	WORD	m_block;	// 2, 3
	WORD	m_page;		// 4, 5
	BYTE	m_chunk;	// 6
	BYTE	m_size;		// B
	BYTE	m_other;	// 7~F
};

///////////////////////////////////////////////////////////////////////////////
// Ata Trace
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
		CBinaryBuffer::Create(size, m_data);
		return m_data->Lock();
	}
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
// -- read trace file
class CPluginTrace::ParserTrace
	: virtual public jcscript::IFeature
	, public CFeatureBase<CPluginTrace::ParserTrace, CPluginTrace>
	, public CJCInterfaceBase
{
public:
	typedef CFeatureBase<CPluginTrace::ParserTrace, CPluginTrace> _BASE;

public:
	ParserTrace(void);
	virtual ~ParserTrace(void);

// IAtomOperate
public:
	virtual bool Invoke(jcparam::IValue * row, jcscript::IOutPort * outport);

// ILoopOperate
public:
	void Init(void);
	bool InvokeOnce(jcscript::IOutPort * outport);

protected:
	void ParseLine(const CJCStringA & line, JCSIZE length);
	static void StaticInit(void);
	void FillFileBuffer(void);
	bool ParsePayload(const char * &first, const char * last, CAtaTrace * trace);
	virtual bool IsRunning(void) {JCASSERT(0); return true;}

public:
	CJCStringT m_file_name;

protected:
	bool m_init;
	FILE * m_src_file;

	char * m_line_buf;
	const char * m_first;
	const char * m_last;

	// 通过解析头得道的列表
	CAtaTrace::COL_ID	m_col_list[MAX_COLUMN];
	JCSIZE	m_col_num;		// 文件中的列数

	// 源文件的行号
	JCSIZE m_line;

protected:
	static bool s_init;
};

///////////////////////////////////////////////////////////////////////////////
// parse vender command from ata trace
class CPluginTrace::VenderCmd
	: virtual public jcscript::IFeature
	, public CFeatureBase<CPluginTrace::VenderCmd, CPluginTrace>
	, public CJCInterfaceBase
{
public:
	typedef CFeatureBase<CPluginTrace::VenderCmd, CPluginTrace> _BASE;

public:
	VenderCmd(void);
	virtual ~VenderCmd(void);

public:
	virtual bool Invoke(jcparam::IValue * row, jcscript::IOutPort * outport);
	virtual bool IsRunning(void);

protected:
	void Init(void);

public:
	CJCStringT m_file_name;

protected:
	UINT	m_cmd_status;
	bool	m_init;
};