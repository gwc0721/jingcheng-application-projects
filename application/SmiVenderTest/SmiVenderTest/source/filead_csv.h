#pragma once

#include <jcparam.h>
#include <script_engine.h>
#include <vector>
#include <map>


template <typename ITERATOR> class csv_line_gm;

///////////////////////////////////////////////////////////////////////////////
//-- csv table



class CCsvColInfo : public jcparam::CColInfoBase
{
public:
	CCsvColInfo( const CJCStringT & name, jcparam::VALUE_TYPE type, JCSIZE length, JCSIZE offset)
		: jcparam::CColInfoBase(0, jcparam::VT_OTHERS, offset, name.c_str()), m_type(type)
	{
	}

public:
	jcparam::VALUE_TYPE	m_type;
};

class CCsvRow
	: virtual public jcparam::ITableRow
	, public CJCInterfaceBase
{
public:
	CCsvRow(void);
	~CCsvRow(void);

public:
	virtual void GetSubValue(LPCTSTR name, IValue * & val);
	virtual void SetSubValue(LPCTSTR name, IValue * val);
	virtual JCSIZE GetRowID(void) const;
	virtual int GetColumnSize() const;

	virtual void GetColumnData(int field, IValue * &)	const;
	virtual const jcparam::CColInfoBase * GetColumnInfo(int field) const;
	// 从row的类型创建一个表格
	virtual bool CreateTable(jcparam::ITable * & tab);
};

class CCsvTable
	: virtual public jcparam::ITable
	, public CJCInterfaceBase
{
public:
	CCsvTable(void);
	~CCsvTable(void);
public:
	static bool Create(CCsvTable * & table);

public:
	virtual void GetSubValue(LPCTSTR name, IValue * & val);
	virtual void SetSubValue(LPCTSTR name, IValue * val);
	virtual JCSIZE GetRowSize() const;
	virtual void GetRow(JCSIZE index, IValue * & row);
	//virtual void NextRow(IValue * & row) = 0;
	virtual JCSIZE GetColumnSize() const;
	virtual void Append(jcparam::IValue * source);
	virtual void AddRow(jcparam::ITableRow * row);

public:
	void AddColumn(const CJCStringT & name, jcparam::VALUE_TYPE, JCSIZE length);
	const jcparam::CColInfoBase * GetColInfo(JCSIZE col) const;
protected:
	// 列定义表
	jcparam::CColumnInfoTable	m_cols_info;
	// 所有列的总长度
	JCSIZE	m_total_length;
	// ROW列表
	typedef std::vector<CCsvRow *>::iterator	row_iterator;
	std::vector<CCsvRow *>	m_rows;
};


///////////////////////////////////////////////////////////////////////////////
//-- CCsvReaderOp
// 用于读取bus doctor文件的op

class CCsvReaderOp
	: virtual public jcscript::IAtomOperate
	//, virtual public ILoopOperate
	, public CJCInterfaceBase
{
public:
	CCsvReaderOp(const CJCStringT & file_name);
	~CCsvReaderOp(void);

protected:
	//typedef jcparam::CTableRowBase<CAtaTrace>	CAtaTraceRow;
	//typedef jcparam::CTypedTable<CAtaTrace>			CAtaTraceTable;

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
	CJCStringT	m_src_fn;
	FILE * m_src_file;

	//bool m_inited;
	CCsvTable * m_table;
	csv_line_gm<CJCStringA::const_iterator> * m_line_gm;
	
#ifdef _DEBUG
	// 用于检查编译结果
public:
	virtual void DebugOutput(LPCTSTR indentation, FILE * outfile);
#endif

};
