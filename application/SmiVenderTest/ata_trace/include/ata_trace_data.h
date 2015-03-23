#pragma once

#include <jcparam.h>
#include "binary_buffer.h"

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
class CAtaTrace
{
public:
public:
	CAtaTrace(void) : m_data(NULL), m_id(UINT_MAX) {memset(this, 0, sizeof(CAtaTrace));}
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
	jcparam::IBinaryBuffer	* m_data;
};

typedef jcparam::CTableRowBase<CAtaTrace>	CAtaTraceRow;
typedef jcparam::CTypedTable<CAtaTrace>		CAtaTraceTable;

class CAtaTraceLoader
{
public:
	CAtaTraceLoader(void);
	~CAtaTraceLoader(void);

public:
	// 用于解析文件头，输入一行字符串
	bool Initialize(const CJCStringT & data_file, const char *);
	bool Initialize(jcparam::IJCStream * stream);

	// 解析一行trace并且输出trace
	bool ParseLine(const char * str, CAtaTraceRow * & trace);
	bool ParseLine(jcparam::IJCStream * stream);

protected:
	bool ParsePayload(const char * & first, const char * last, CAtaTrace * trace);
	
protected:
	// 通过解析头得道的列表
	CAtaTrace::COL_ID	m_col_list[MAX_COLUMN];
	JCSIZE	m_col_num;		// 文件中的列数
	CJCStringT	m_data_fn;	// 二进制数据的文件名
	CFileMapping	* m_file_mapping;

protected:
	static bool m_init;

};