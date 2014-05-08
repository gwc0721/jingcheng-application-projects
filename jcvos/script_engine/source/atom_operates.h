#pragma once

#include "../include/iplugin.h"
#include <list>

#include "expression_op.h"

// 定义一个基本执行单位，包括从何处取参数1和2，执行什么过程(IProxy)，经结果保存在何处。
// 如果执行的过程为NULL，则直接将参数1保存到结果
//

namespace jcscript
{
	typedef std::list<IAtomOperate*>	OP_LIST;
	void DeleteOpList(OP_LIST & aolist);

#define IChainOperate	IAtomOperate

	///////////////////////////////////////////////////////////////////////////////
	//-- CAssignOp
	// 用于从source中取出结果，赋值到制定op的var中
	class CAssignOp 
		: public CAtomOpBase<IAtomOperate, 1, CAssignOp>
		JCIFBASE
	{
	public:
		CAssignOp(IAtomOperate * dst_op, const CJCStringT & dst_name);
		virtual ~CAssignOp(void);

		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);

	public:
		virtual void DebugInfo(FILE * outfile);

	protected:
		IAtomOperate * m_dst_op;
		CJCStringT	m_dst_name;

	public:
		static const TCHAR m_name[];
	};

	///////////////////////////////////////////////////////////////////////////////
	//-- CPushParamOp
	// 用于从source中取出结果，设置函数的参数
	class CPushParamOp
		: public CAtomOpBase<IAtomOperate, 1, CPushParamOp>
		JCIFBASE
	{
	public:
		CPushParamOp(IFeature * dst_op, const CJCStringT & dst_name);
		virtual ~CPushParamOp(void);
	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);
		virtual UINT GetProperty(void) const {return 0;};

	public:
		void SetParamName(const CJCStringT & param_name)
		{m_param_name = param_name;};
		virtual void DebugInfo(FILE * outfile);

	protected:
		IFeature * m_function;
		CJCStringT	m_param_name;

	public:
		static const TCHAR m_name[];
	};


	///////////////////////////////////////////////////////////////////////////////
	//-- CLoopVarOp
	// 用于combo执行循环时，处理循环变量
	class CLoopVarOp
		: public CAtomOpBase<IAtomOperate, 1, CLoopVarOp>
		JCIFBASE
	{
	public:
		CLoopVarOp(void);
		virtual ~CLoopVarOp(void);

		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);

		virtual void GetProgress(JCSIZE &cur_prog, JCSIZE &total_prog) const;
		virtual void Init(void);

		virtual UINT GetProperty(void) const {return OOP_STATE;}
		void SetOutPort(IOutPort * outport)
		{
			JCASSERT(outport);
			m_outport = outport;
		}
		virtual void DebugInfo(FILE * outfile);
	public:
		static const TCHAR m_name[];

	protected:
		jcparam::ITable		* m_table;	
		IOutPort			* m_outport;

		JCSIZE	m_table_size;
		JCSIZE	m_cur_index;		// 循环计数器
	};

	///////////////////////////////////////////////////////////////////////////////
	//-- CCollectOp
	// 在ComoboSt的循环中，将最后语句输出的表格连接在一起。
	class CCollectOp
		: public CAtomOpBase<IOutPort, 1, CCollectOp>
		JCIFBASE
	{
	public:
		CCollectOp(void);
		virtual ~CCollectOp(void);

		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);
		virtual void DebugInfo(FILE * outfile);

	public:
		static const TCHAR m_name[];

	protected:
		// 保存结果
		jcparam::IVector * m_table;
	};

	//class CHelpProxy
	//	: virtual public IAtomOperate
	//	, public COpSourceSupport0<CHelpProxy>
	//	JCIFBASE
	//{
	//protected:
	//	CHelpProxy(IHelpMessage * help);
	//	virtual ~CHelpProxy(void);

	//public:
	//	friend class COpSourceSupport0<CHelpProxy>;
	//	static void Create(IHelpMessage * help, CHelpProxy * & proxy);

	//public:
	//	virtual bool GetResult(jcparam::IValue * & val);
	//	virtual bool Invoke();
	//	virtual void SetSource(UINT src_id, IAtomOperate * op);


	//protected:
	//	IHelpMessage * m_help;
	//	static const TCHAR name[];
	//};

	///////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////
	//-- CSaveToFileOp
	// 将变量保存到文件
	class CSaveToFileOp
		: public CAtomOpBase<IAtomOperate, 1, CSaveToFileOp>
		JCIFBASE 
	{
	public:
		CSaveToFileOp(const CJCStringT &filename);
		~CSaveToFileOp(void);
	public:
		virtual UINT GetProperty(void) const {return OOP_STATE;}
		virtual bool GetResult(jcparam::IValue * & val) {JCASSERT(0); return false;} ;
		virtual bool Invoke(void);
		virtual void DebugInfo(FILE * outfile);

	public:
		static const TCHAR m_name[];

	protected:
		CJCStringT m_file_name;
		FILE * m_file;

		jcparam::IStream * m_stream;
	};

	///////////////////////////////////////////////////////////////////////////////
	//-- CExitOp
	class CExitOp
		: public COpSourceSupport0<CExitOp, IAtomOperate>
		JCIFBASE 
	{
	public:
		virtual bool GetResult(jcparam::IValue * & val) {JCASSERT(0); return false;} ;
		virtual bool Invoke(void);
	public:
		static const TCHAR name[];
	};

///////////////////////////////////////////////////////////////////////////////
// filter statement
	class CFilterSt
		: public CAtomOpBase<IAtomOperate, 2, CFilterSt>
		JCIFBASE
	{
	public:
		CFilterSt(IOutPort * out_port);
		~CFilterSt(void);

		enum SRC_ID {	SRC_TAB = 0, SRC_EXP = 1 };

	public:
		virtual UINT GetProperty(void) const {return OOP_STATE;}
		virtual bool GetResult(jcparam::IValue * & val) {JCASSERT(0); return false;};
		virtual bool Invoke(void);

	public:
		static const TCHAR m_name[];

	protected:
		// src0: the source of bool expression.
		// src1: the source of table
		bool m_init;
		IOutPort	* m_outport;
	};

///////////////////////////////////////////////////////////////////////////////
//-- CValueFileName
	// 用于处理文件名
	class CValueFileName
		: virtual public jcparam::IValue
		JCIFBASE
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

///////////////////////////////////////////////////////////////////////////////
//-- 
	class CInPort
		: public CAtomOpBase<IOutPort, 1, CInPort>
		JCIFBASE
	{
	public:
		CInPort(void);
		~CInPort(void);

	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);
		virtual UINT GetProperty(void) const {return OPP_LOOP_SOURCE;};
		virtual UINT GetDependency(void) {return m_dependency +1;}

	public:
		static const TCHAR m_name[];

	protected:
		jcparam::IValue * m_row;

	};


///////////////////////////////////////////////////////////////////////////////
// -- feature wrapper
	// feature wrapper is a state op, it need to be insert in to single statement
	class CFeatureWrapper
		: public CAtomOpBase<IAtomOperate, 1, CFeatureWrapper>
		JCIFBASE
	{
	public:
		CFeatureWrapper(IFeature * feature);
		virtual ~CFeatureWrapper(void);

	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);
		virtual UINT GetProperty(void) const {return OOP_STATE;}
		virtual void DebugInfo(FILE * outfile);

	public:
		void SetOutPort(IOutPort * outport);

	public:
		static const TCHAR m_name[];
	protected:
		IFeature	* m_feature;
		IOutPort	* m_outport;
	};

///////////////////////////////////////////////////////////////////////////////
// -- vector maker
	class COpVectorMaker
		: public CAtomOpBase<IAtomOperate, 3, COpVectorMaker>
		JCIFBASE
	{
	public:
		COpVectorMaker(void);
		virtual ~COpVectorMaker(void);

	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);
		virtual UINT GetProperty(void) const {return OOP_STATE;}
		virtual void DebugInfo(FILE * outfile);

	public:
		void SetOutPort(IOutPort * outport);
	public:
		static const TCHAR m_name[];

	protected:
		IOutPort	* m_outport;
		bool m_init;
		int	m_current, m_step, m_end;
	};
};
