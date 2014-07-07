#pragma once

#include <jcparam.h>

#include "plugin_trace.h"
//#include "feature_base.h"
#include <SmiDevice.h>

#define CMD_WRITE_DMA		(0xCA)
#define CMD_READ_DMA		(0xC8)
#define CMD_WRITE_SECTOR	(0x30)
#define CMD_READ_SECTOR		(0x20)
#define CMD_WRITE_DMA_EX	(0x35)
#define CMD_READ_DMA_EX		(0x25)

#define MAX_COLUMN			(20)

class CTracePayload;	// 用于格式化输出IN/OUT的data
///////////////////////////////////////////////////////////////////////////////
// Ata Trace
class CFis
{
public:
public:
	JCSIZE	m_id;	// store
	BYTE	m_type;	// fis type id
	double	m_time_stamp;
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
};

///////////////////////////////////////////////////////////////////////////////
// Ata Trace
class CAtaTrace
{
public:
public:
	CAtaTrace(void) : m_data(NULL) {memset(this, 0, sizeof(CAtaTrace));}
	CAtaTrace(const CAtaTrace & trace) 
		: m_id(trace.m_id), m_start_time(trace.m_start_time), m_busy_time(trace.m_busy_time)
		, m_feature(trace.m_feature), m_cmd_code(trace.m_cmd_code), m_lba(trace.m_lba)
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
	double	m_start_time;	// time stamp in (sec)
	double	m_busy_time;	// command's busy time in (ms)
	BYTE	m_feature;
	BYTE	m_cmd_code;
	JCSIZE	m_lba;
	JCSIZE	m_sectors;
	BYTE	m_status;
	BYTE	m_error;
	CBinaryBuffer	* m_data;

public:
	BYTE * CreateBuffer(JCSIZE size)
	{
		CBinaryBuffer::Create(size, m_data);
		return m_data->Lock();
	}
};

class CTracePayload	: public jcparam::COLUMN_INFO_BASE
{
public:
	CTracePayload(int id, JCSIZE offset, LPCTSTR title) 
		: jcparam::COLUMN_INFO_BASE(id, jcparam::VT_OTHERS, offset, title) {};

public:
	virtual void GetText(void * row, CJCStringT & str) const;
	virtual void CreateValue(BYTE * src, jcparam::IValue * & val) const;
};

typedef jcparam::CTableRowBase<CAtaTrace>	CAtaTraceRow;
typedef jcparam::CTypedTable<CAtaTrace>		CAtaTraceTable;

///////////////////////////////////////////////////////////////////////////////
// vender command
class CVenderCmd
{
public:
	CVenderCmd(void);
	CVenderCmd(const CVenderCmd & cmd);
	~CVenderCmd(void);

	static const JCSIZE CMD_BUF_LENGTH = 16;

public:
	void SetCmdData(BYTE * data, JCSIZE length);

public:
	JCSIZE	m_id;		// store
	WORD	m_cmd_id;		// 0, 1 command id
	WORD	m_block;	// 2, 3
	WORD	m_page;		// 4, 5
	BYTE	m_chunk;	// 6
	int	m_read_write;
	JCSIZE	m_sectors;		// actual data size in sector
	//BYTE[CMD_BUF_LENGTH]	m_cmd_data;	// 7~F
	CBinaryBuffer * m_cmd_data;
	CBinaryBuffer * m_data;
};

typedef jcparam::CTableRowBase<CVenderCmd>	CVenderCmdRow;

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
	virtual bool Init(void);
	bool InvokeOnce(jcscript::IOutPort * outport);

protected:
	void ParseLine(const CJCStringA & line, JCSIZE length);
	static void StaticInit(void);
	void FillFileBuffer(void);
	bool ParsePayload(const char * &first, const char * last, CAtaTrace * trace);
	//virtual bool IsRunning(void) {return m_running;}

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

	//bool m_running;

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
	//virtual bool IsRunning(void);

protected:
	bool Init(void);

public:
	CJCStringT m_file_name;

protected:
	UINT	m_cmd_status;
	bool	m_init;
	JCSIZE m_store;
	CVenderCmdRow * m_cmd;
};