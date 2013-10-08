#pragma once

#include <jcparam.h>
#include <script_engine.h>


class CAtaTrace
{
public:
	JCSIZE	m_id;			// store
	double	m_start_time;	// time stamp in sec
	double	m_busy_time;	// command's busy time
	BYTE	m_cmd_code;
	JCSIZE	m_lba;
	JCSIZE	m_sectors;
	BYTE	m_status;
};

// 用于读取bus doctor文件的op

class CBusDoctorOp
	: virtual public jcscript::IAtomOperate
	//, virtual public ILoopOperate
	, public CJCInterfaceBase
{
public:
	CBusDoctorOp(const CJCStringT & file_name);
	~CBusDoctorOp(void);

protected:
	typedef jcparam::CTableRowBase<CAtaTrace>	CAtaTraceRow;
	typedef jcparam::CTypedTable<CAtaTrace>			CAtaTraceTable;

	// IAtomOperate
public:
	virtual bool GetResult(jcparam::IValue * & val);
	virtual bool Invoke(void);
	virtual void SetSource(UINT src_id, IAtomOperate * op);
// ILoopOperate
public:
	virtual void GetProgress(JCSIZE cur_prog, JCSIZE total_prog) const;
	virtual void Init(void);
	virtual bool InvokeOnce(void);

protected:
	//void ParseLine(const char * line, JCSIZE length);
	void ParseLine(const CJCStringA & line, JCSIZE length);
//public:
//	virtual bool PushParameter(const CJCStringT & var_name, jcparam::IValue * val) = 0;
//	virtual bool CheckAbbrev(TCHAR param_name, CJCStringT & var_name) const = 0;

protected:
	FILE * m_src_file;

	bool m_inited;
	CAtaTraceTable * m_trace_tab;

	
#ifdef _DEBUG
	// 用于检查编译结果
public:
	virtual void DebugOutput(LPCTSTR indentation, FILE * outfile);
#endif

};
