#pragma once

#include "../include/iplugin.h"
#include <list>

#include "atom_operates.h"

namespace jcscript
{

///////////////////////////////////////////////////////////////////////////////
// -- 一个操作序列
	class CSequenceOp
		: virtual public IAtomOperate
	{
	public:
		typedef std::list<IAtomOperate*>	OP_LIST;
		bool InvokeOpList(void);

	public:
		CSequenceOp(CSequenceOp * super);
		virtual ~CSequenceOp(void);

	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual IAtomOperate * GetChain(void)	{ JCASSERT(0); return NULL; }; 
		virtual UINT GetDependency(void) {return m_dependency;};

	public:
		virtual void AddOp(IAtomOperate * op);
		virtual CSequenceOp * GetSuper(void);
		virtual bool IsEmpty(void) {return m_op_list.empty();}
		
	protected:
		OP_LIST	m_op_list;
		CSequenceOp * m_super_seq;
		UINT m_dependency;
		// m_result_op的输出结果作为整个Comob的输出结果
		IAtomOperate * m_result_op;

	public:
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile);

		inline virtual void AddRef() {++m_ref;}
		inline virtual void Release(void)		{	if ( (--m_ref) == 0) delete this;	}
		inline virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr) {return false;}
	protected:
		long m_ref;
	};

///////////////////////////////////////////////////////////////////////////////
//-- CScriptOp
	class CScriptOp
		: public CSequenceOp
		JCIFBASE
	{
	public:
		CScriptOp(CSequenceOp * super);
		virtual ~CScriptOp(void);

	public:
		virtual bool Invoke(void);
		virtual void SetSource(UINT src_id, IAtomOperate * op) {JCASSERT(0); };
		virtual UINT GetProperty(void) const {return 0;};

	public:
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile);
	};

///////////////////////////////////////////////////////////////////////////////
// -- combo statement
// 实现一个复合语句
	class CComboSt 
		: public CSequenceOp
		JCIFBASE
	{
	public:
		CComboSt(CSequenceOp * super);
		virtual ~CComboSt(void);

	// management fucntions
	public:
		virtual bool Invoke(void);
		virtual void SetSource(UINT src_id, IAtomOperate * op) { JCASSERT(0); };
		virtual UINT GetProperty(void) const {return 0;};
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile);

	public:
		virtual void AddOp(IAtomOperate * op);


		// 设置循环变量的操作符，每个复合语句只能设置一个循环变量。不支持嵌套循环。
		// 当复合语句已经设置循环变量时，返回false
		// 如果is_func为true，则作为下一个分句的输入
		void SetAssignOp(IAtomOperate * op);

		// 编译的最后处理
		void CompileClose(void);

	// 输出编译信息
	public:
		JCSIZE m_line;
		static const TCHAR m_name[];
	};



///////////////////////////////////////////////////////////////////////////////
// single statement
	class CSingleSt
		: virtual public IAtomOperate
		, virtual public IOutPort 
		, public CSequenceOp
		JCIFBASE
	{
	public:
		CSingleSt(CSequenceOp * super);
		virtual ~CSingleSt(void);

	public:
		virtual void SetSource(UINT src_id, IAtomOperate * src);
		virtual bool Invoke(void);
		virtual UINT GetProperty(void) const {return m_property;};
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile);
		virtual bool IsEmpty(void) {return m_outport.empty(); };

	public:
		virtual void AddOp(IAtomOperate * op);

	// implement of inport
	public:
		CInPort * GetInPort(void)	{return m_inport;}

	protected:
		CInPort		* m_inport;
		// op for loop control. do not need AddRef()
		IAtomOperate * m_state_op;	// state machine

	// implement of outport
	public:
		virtual bool PopupResult(jcparam::IValue * & val);
		virtual bool PushResult(jcparam::IValue * val);
		typedef std::list<jcparam::IValue *>		ROW_LIST;
	protected:
		ROW_LIST	m_outport;

		UINT		m_property;
		static const TCHAR m_name[];

		bool	m_state_op_run_more;		// default true
		bool	m_init;			// 第一次强制运行
	};

///////////////////////////////////////////////////////////////////////////////
// -- if-then-else 语句
	//class CStIf
	//	: virtual public IAtomOperate
	//	, public COpSourceSupport<IOperator, 1, CStIf>
	//{
	//public:
	//	CStIf(void);
	//	virtual ~CStIf(void);

	//public:
	//	virtual bool GetResult(jcparam::IValue * & val) {JCASSERT(0); return false;}
	//	virtual bool Invoke(void);
	//	virtual void SetSource(UINT src_id, IAtomOperate * op) { JCASSERT(0); };

	//	CSequenceOp * GetSubSequenc(bool id);

	//public:
	//	static const TCHAR m_name[];

	//protected:
	//	CSequenceOp  *m_true_seq;
	//	CSequenceOp	*m_false_seq;
	//};
};