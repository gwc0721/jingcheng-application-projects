#pragma once

#include <jcparam.h>

#include "plugin_default.h"
#include "feature_base.h"

#define CMD_WRITE_DMA		(0xCA)
#define CMD_READ_DMA		(0xC8)
#define CMD_WRITE_SECTOR	(0x30)

class CAtaTrace
{
public:
	class TracePayload;	// 用于格式化输出IN/OUT的data
public:
	CAtaTrace(void) {memset(this, 0, sizeof(CAtaTrace));}
	~CAtaTrace(void) {delete [] m_data;}
public:
	JCSIZE	m_id;			// store
	double	m_start_time;	// time stamp in sec
	double	m_busy_time;	// command's busy time
	BYTE	m_featue;
	BYTE	m_cmd_code;
	JCSIZE	m_lba;
	JCSIZE	m_sectors;
	BYTE	m_status;
	BYTE	* m_data;
	JCSIZE	m_data_len;
};

class CAtaTrace::TracePayload	: public jcparam::CColInfoBase
{
public:
	TracePayload(int id) : jcparam::CColInfoBase(id, jcparam::VT_OTHERS, offsetof(CAtaTrace, m_data), _T("data")) {};

public:
	virtual void GetText(void * row, CJCStringT & str) const
	{
		CAtaTrace * trace = reinterpret_cast<CAtaTrace*>(row);
		if (NULL == trace->m_data) return;
		JCSIZE len = min(trace->m_data_len, 16);
		JCSIZE str_len = len * 3 + 1;

		str.resize(len * 3 + 1);
		TCHAR * _str = const_cast<TCHAR*>(str.data());
		for (JCSIZE ii = 0; ii < len; ++ii)
		{
			JCSIZE written = stdext::jc_sprintf(_str, str_len, _T("%02X "), trace->m_data[ii]);
			_str += written;
			str_len -= written;
		}
	}

	virtual void CreateValue(BYTE * src, jcparam::IValue * & val) const
	{
		CJCStringT str;
		GetText(src, str);
		val = jcparam::CTypedValue<CJCStringT>::Create(str);
	}
};

typedef jcparam::CTableRowBase<CAtaTrace>	CAtaTraceRow;
typedef jcparam::CTypedTable<CAtaTrace>		CAtaTraceTable;

class CPluginDefault::ParserTrace
	: virtual public jcscript::ILoopOperate
	, public CFeatureBase<CPluginDefault::ParserTrace, CPluginDefault>
	, public CJCInterfaceBase
{
public:
	typedef CFeatureBase<CPluginDefault::ParserTrace, CPluginDefault> _BASE;

public:
	enum COL_ID
	{
		COL_STORE, COL_TIME_STAMP, COL_BUSY_TIME, COL_CMD_ID, 
		COL_CMDX, COL_LBA, COL_SECTORS, COL_STATUS, COL_DATA,
	};

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

public:
	CJCStringT m_file_name;

protected:
	FILE * m_src_file;

	char * m_line_buf;
	char * m_last;
	const char * m_first;

	jcparam::IValue * m_output;
};