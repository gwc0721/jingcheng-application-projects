#pragma once

#include <script_engine.h>
using namespace jcscript;

///////////////////////////////////////////////////////////////////////////////
//-- CProxySyntest

class CProxySyntest 
	: virtual public IFeature
	, virtual public ILoopOperate
	, public CJCInterfaceBase
{
public:
	CProxySyntest(const CJCStringT & name);
	~CProxySyntest(void);

public:
	virtual bool GetResult(jcparam::IValue * & result);
	virtual bool Invoke(void);
	virtual void SetSource(UINT src_id, IAtomOperate * op);

	virtual bool PushParameter(const CJCStringT & var_name, jcparam::IValue * val);
	virtual bool CheckAbbrev(TCHAR param_name, CJCStringT & var_name) const;

	virtual void GetProgress(JCSIZE &cur_prog, JCSIZE &total_prog) const 
	{cur_prog = m_count, total_prog = 10;};
	virtual void Init(void) {m_count = 0;};
	virtual bool InvokeOnce(void) { m_count ++; return (m_count<10);};

protected:
	IAtomOperate * m_source;
	CJCStringT m_name;
	JCSIZE m_count;

#ifdef _DEBUG
public:
	virtual void DebugOutput(LPCTSTR indentation, FILE * outfile);
#endif
};

///////////////////////////////////////////////////////////////////////////////
//-- CPluginSyntest

class CPluginSyntest : virtual public IPlugin, public CJCInterfaceStatic
{
//public:
//	CPluginSyntest(const CJCStringT & name) : m_name(name) {};
public:
	virtual bool Reset(void); 
	virtual void GetFeature(const CJCStringT & cmd_name, IFeature * & pr);
	virtual void ShowFunctionList(FILE * output) const;
	virtual LPCTSTR name() const;
	virtual void Release(void);
};

class CValueSyntest;

///////////////////////////////////////////////////////////////////////////////
//-- CRowSyntest
class CRowSyntest : virtual public jcparam::ITableRow, public CJCInterfaceBase
{
public:
	CRowSyntest(CValueSyntest *);
public:
	virtual JCSIZE GetRowID(void) const {return 1;};
	virtual int GetColumnSize() const {return 5;}

	virtual void GetColumnData(int field, jcparam::IValue * &)	const;

	virtual LPCTSTR GetColumnName(int field_id) const
	{
		return _T("column");
	}
	virtual const jcparam::COLUMN_INFO_BASE * GetColumnInfo(int field) const {return NULL;};
	
	virtual void GetSubValue(LPCTSTR name, IValue * & val) {};
	// 如果name不存在，则插入，否则修改name的值
	virtual void SetSubValue(LPCTSTR name, IValue * val) {};
	virtual bool CreateTable(jcparam::ITable * & tab);
// for memory leak debug
public:
	virtual void AddRef(void);
	virtual void Release(void);

protected:
	CValueSyntest * m_table;
};


///////////////////////////////////////////////////////////////////////////////
//-- CValueSyntest
class CValueSyntest : virtual public jcparam::ITable, public CJCInterfaceBase
{
public:
	CValueSyntest(void);
	~CValueSyntest(void);
// IValue interface
public:
	virtual void GetSubValue(LPCTSTR name, jcparam::IValue * & val);
	virtual void SetSubValue(LPCTSTR name, jcparam::IValue * val) {};

// ITable interface
public:
	virtual JCSIZE GetRowSize() const {return 10;}
	virtual void GetRow(JCSIZE index, IValue * & row);

	virtual JCSIZE GetColumnSize() const {return m_row->GetColumnSize();}
	virtual void Append(IValue * source) {}
	virtual void PushBack(jcparam::IValue * row) {};


// for memory leak debug
public:
	virtual void AddRef(void);
	virtual void Release(void);

protected:
	CRowSyntest * m_row;
};



class CContainerSyntest : virtual public IPluginContainer, public CJCInterfaceStatic
{
public:
	CContainerSyntest(void);
	~CContainerSyntest(void);

public:
	virtual bool GetPlugin(const CJCStringT & name, IPlugin * & plugin);
	virtual void GetVarOp(IAtomOperate * & op);
	virtual bool ReadFileOp(LPCTSTR type, const CJCStringT & filename, IAtomOperate *& op);

public:
	void Delete(void);

protected:
	IAtomOperate * m_var_op;		// CConstantOp
	CValueSyntest * m_variable;
};

