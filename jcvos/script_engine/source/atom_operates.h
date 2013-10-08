#pragma once

#include "../include/iplugin.h"
#include <list>

#include "expression_op.h"

// 定义一个基本执行单位，包括从何处取参数1和2，执行什么过程(IProxy)，经结果保存在何处。
// 如果执行的过程为NULL，则直接将参数1保存到结果

namespace jcscript
{
	typedef std::list<IAtomOperate*>	OP_LIST;
	void DeleteOpList(OP_LIST & aolist);
	void InvokeOpList(OP_LIST & op_list);

	class CComboStatement;

	///////////////////////////////////////////////////////////////////////////////
	//-- CScriptOp
	class CScriptOp
		: virtual public IAtomOperate
		, public CJCInterfaceBase
	{
	protected:
		CScriptOp(void);
		virtual ~CScriptOp(void);
	public:
		static void Create(CScriptOp * & program);

	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);
		virtual void SetSource(UINT src_id, IAtomOperate * op) { JCASSERT(0); };

	public:
		void PushBackAo(IAtomOperate * op);
		void Merge(CComboStatement * combo);
		
	protected:
		OP_LIST	m_op_list;

	#ifdef _DEBUG
	public:
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile);
	#endif
	};

	///////////////////////////////////////////////////////////////////////////////
	//-- CAssignOp
	// 用于从source中取出结果，赋值到制定op的var中
	class CAssignOp 
		: virtual public IAtomOperate
		, public CJCInterfaceBase
	{
	public:
		CAssignOp(IAtomOperate * dst_op, const CJCStringT & dst_name);
		virtual ~CAssignOp(void);

		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);
		virtual void SetSource(UINT src_id, IAtomOperate * op);

	public:
		IAtomOperate * GetSource(void) const {return m_src_op;};

	protected:
		IAtomOperate * m_src_op;

		IAtomOperate * m_dst_op;
		CJCStringT	m_dst_name;

	#ifdef _DEBUG
	public:
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile);
	#endif
	};

	///////////////////////////////////////////////////////////////////////////////
	//-- CPushParamOp
	// 用于从source中取出结果，设置函数的参数
	class CPushParamOp
		: virtual public IAtomOperate
		, public CJCInterfaceBase
	{
	public:
		CPushParamOp(IFeature * dst_op, const CJCStringT & dst_name);
		virtual ~CPushParamOp(void);
	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);
		virtual void SetSource(UINT src_id, IAtomOperate * op);
	public:
		void SetParamName(const CJCStringT & param_name)
		{m_param_name = param_name;};

	protected:
		IAtomOperate * m_src_op;
		IFeature * m_function;
		CJCStringT	m_param_name;

	#ifdef _DEBUG
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile);
	#endif
	};


	///////////////////////////////////////////////////////////////////////////////
	//-- CLoopVarOp
	// 用于combo执行循环时，处理循环变量
	class CLoopVarOp
		: virtual public ILoopOperate
		, public CJCInterfaceBase
	{
	public:
		CLoopVarOp(void);
		virtual ~CLoopVarOp(void);

		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);
		virtual void SetSource(UINT src_id, IAtomOperate * op);

		virtual void GetProgress(JCSIZE &cur_prog, JCSIZE &total_prog) const;
		virtual void Init(void);
		virtual bool InvokeOnce(void);

	protected:
		jcparam::ITable		* m_table;	
		jcparam::IValue		* m_row_val;	// 每次循环后更新
		IAtomOperate		* m_source;

		JCSIZE	m_table_size;
		JCSIZE	m_cur_index;		// 循环计数器

	#ifdef _DEBUG
	public:
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile);
	#endif
	};

	///////////////////////////////////////////////////////////////////////////////
	//-- CCollectOp
	// 在ComoboSt的循环中，将最后语句输出的表格连接在一起。
	class CCollectOp
		: virtual public IAtomOperate
		, public CJCInterfaceBase \
	{
	public:
		CCollectOp(void);
		virtual ~CCollectOp(void);

		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);
		virtual void SetSource(UINT src_id, IAtomOperate * op);


	protected:
		IAtomOperate * m_source;
		// 保存结果
		jcparam::ITable * m_table;

	#ifdef _DEBUG
	public:
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile);
	#endif
	};

	///////////////////////////////////////////////////////////////////////////////
	//-- CLoopOp
	class CLoopOp
		: virtual public IAtomOperate
		, public CJCInterfaceBase
	{
	public:
		friend class CComboStatement;

	public:
		CLoopOp(void);
		virtual ~CLoopOp(void);

	// interface
	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke();
		virtual void SetSource(UINT src_id, IAtomOperate * op) { JCASSERT(0); };

	public:
		// 在循环体中添加一个操作符
		void AddLoopOp(IAtomOperate * ao/*, bool is_func*/);

		// 设置循环变量的操作符，每个复合语句只能设置一个循环变量。不支持嵌套循环。
		// 当复合语句已经设置循环变量时，返回false
		// 如果is_func为true，则作为下一个分句的输入
		void SetLoopVar(ILoopOperate * expr_op);

	protected:
		// 循环变量
		ILoopOperate * m_loop_var;
		// 循环体（序列）
		OP_LIST		m_loop_operates;

	// 输出编译信息
	#ifdef _DEBUG
	public:
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile);
		JCSIZE m_line;
	#endif	
	};

	///////////////////////////////////////////////////////////////////////////////
	//-- CComboStatement
	// 实现一个复合语句
	class CComboStatement 
		: virtual public IAtomOperate
		, public CJCInterfaceBase
	{
	public:
		friend class CScriptOp;
	protected:
		CComboStatement(void);
		virtual ~CComboStatement(void);

	public:
		static void Create(CComboStatement * & proxy);

	// management fucntions
	public:
		// 在循环体中添加一个操作符
		// is_func
		void AddLoopOp(IAtomOperate * ao);

		// 添加一个预处理操作符，预处理在循环之前执行
		void AddPreproOp(IAtomOperate * op);

		// 设置循环变量的操作符，每个复合语句只能设置一个循环变量。不支持嵌套循环。
		// 当复合语句已经设置循环变量时，返回false
		// 如果is_func为true，则作为下一个分句的输入
		//bool SetLoopVar(IAtomOperate * expr_op);

		void SetAssignOp(IAtomOperate * op);

		//void GetLoopVar(IAtomOperate * & op);

		IAtomOperate * LastChain(IAtomOperate * op) 
		{ 
			IAtomOperate * last = m_last_chain;
			if (op) m_last_chain = op;
			return last;
		}

		bool Merge(CComboStatement * combo, IAtomOperate *& last);

		// 编译的最后处理
		void CompileClose(void);

	// interface
	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke();
		virtual void SetSource(UINT src_id, IAtomOperate * op) { JCASSERT(0); };

	protected:
		OP_LIST		m_prepro_operates;
		CLoopOp		* m_loop;

		// 非循环参数： expr_op -> push_op -> func_op, 
		//    expr_op和push_op在 m_prepro_operates中，func_op在m_loop_operates中

		// 循环参数：expr_op -> m_loop_op -> push_op -> func_op, 
		//    expr_op在m_prepro_operates中，
		//    m_loop_op独立存放，并且在每次执行m_loop_operates前执行一次，返回结果做为循环结束的标志
		//    push_op和func_op在m_loop_operates中
		// 如果循环参数是复合语句的第一个分句则：expr_op -> m_loop_op, （！！未实现）
		//    expr_op在m_prepro_operates中，
		//	  m_loop_op独立存放，并且在每次执行m_loop_operates前执行一次，返回结果做为循环结束的标志
		//

		// 复合语句运行结果的设置目的。通常由复合语句最后的赋值符号给出。
		IAtomOperate * m_assignee;

		// 串联调用的最终运行结果集合
		jcparam::IValue * m_result;

		// 用于检查是否做过编译后处理
		bool m_closed;

		// 指向串联调用的前一个函数，作为后一个函数的source, 不需要修改引用计数
		IAtomOperate	* m_last_chain;

	// 输出编译信息
	#ifdef _DEBUG
	public:
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile);
		JCSIZE m_line;
	#endif
	};

	class CHelpProxy
		: virtual public IAtomOperate
		, public CJCInterfaceBase
	{
	protected:
		CHelpProxy(IHelpMessage * help);
		virtual ~CHelpProxy(void);

	public:
		static void Create(IHelpMessage * help, CHelpProxy * & proxy);

	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke();
		virtual void SetSource(UINT src_id, IAtomOperate * op);

	protected:
		IHelpMessage * m_help;

	#ifdef _DEBUG
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile) {};
	#endif
	};

	///////////////////////////////////////////////////////////////////////////////
	//-- CVariableOp
	// 用于从一个变量中取出其成员。即var1.var2.var3
	// 目前以嵌套方式实现成员变量
	class CVariableOp	
		: virtual public IAtomOperate
		, public COpSourceSupport<IAtomOperate, 1>
		, public CJCInterfaceBase 
	{
	public:
		// CVariableOp作为计算变量成员使用。
		CVariableOp(CVariableOp * parent, const CJCStringT & var_name);

		// CVariableOp作为IValue的封装使用
		CVariableOp(jcparam::IValue * val);
		virtual ~CVariableOp(void);

	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);
		//virtual void SetSource(UINT src_id, IAtomOperate * op);

		void SetVariableName(const CJCStringT & name);

	protected:
		// m_paraent是m_source的拷贝，用于表示需要被Invoke。 m_parent不需要引用计数
		CVariableOp * m_parent;		
		//IAtomOperate * m_source;
		CJCStringT m_var_name;
		jcparam::IValue * m_val;

	#ifdef _DEBUG
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile);
	#endif
	};


	///////////////////////////////////////////////////////////////////////////////
	//-- CSaveToFileOp
	// 将变量保存到文件
	class CSaveToFileOp
		: virtual public IAtomOperate
		, public CJCInterfaceBase 
	{
	public:
		CSaveToFileOp(const CJCStringT &filename);
		~CSaveToFileOp(void);
	public:
		virtual bool GetResult(jcparam::IValue * & val) {JCASSERT(0); return false;} ;
		virtual bool Invoke(void);
		virtual void SetSource(UINT src_id, IAtomOperate * op);

	protected:
		IAtomOperate * m_src_op;
		CJCStringT m_file_name;
		FILE * m_file;

	#ifdef _DEBUG
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile);
	#endif		
	};

	///////////////////////////////////////////////////////////////////////////////
	//-- CExitOp
	class CExitOp
		: virtual public IAtomOperate
		, public COpSourceSupport0
		, public CJCInterfaceBase 
	{
	public:
		virtual bool GetResult(jcparam::IValue * & val) {JCASSERT(0); return false;} ;
		virtual bool Invoke(void);
		//virtual void SetSource(UINT src_id, IAtomOperate * op) {JCASSERT(0)};

	#ifdef _DEBUG
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile);
	#endif		

	};

///////////////////////////////////////////////////////////////////////////////
// filter statement
	class CFilterSt
		: virtual public IAtomOperate
		, public COpSourceSupport<IAtomOperate, 2>
		, public CDebugInfo0
		, public CJCInterfaceBase
	{
	public:
		CFilterSt(void);
		~CFilterSt(void);

		enum SRC_ID {	SRC_TAB = 0, SRC_EXP = 1 };

	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);

	// debug information
	public:
		virtual LPCTSTR name() const {return _T("filter");}
		virtual IAtomOperate * source(UINT src) 
		{
			JCASSERT(src < 2);
			return m_src[src];
		}

	protected:
		// src0: the source of bool expression.
		// src1: the source of table
		bool m_init;
		jcparam::ITableRow * m_row;
	};

///////////////////////////////////////////////////////////////////////////////
//-- CValueFileName
	// 用于处理文件名
	class CValueFileName
		: virtual public jcparam::IValue
		, public CJCInterfaceBase
	{
	public:
		CValueFileName(const CJCStringT & fn) : m_file_name(fn) {}
		LPCTSTR GetFileName(void) const {return m_file_name.c_str(); }

	public:
		virtual void GetSubValue(LPCTSTR name, jcparam::IValue * & val) {}
		virtual void SetSubValue(LPCTSTR name, jcparam::IValue * val) {}

	protected:
		CJCStringT m_file_name;
	};

};