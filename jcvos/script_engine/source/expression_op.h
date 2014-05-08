#pragma once

#include "../include/iplugin.h"
#include "operate_base.h"
#include <jcparam.h>





namespace jcscript
{
	template <typename TS, typename TD>		// 元类型，结果类型
	void Convert(const void * s, void * d)
	{
		TS* _s = (TS*)(s);
		TD* _d = (TD*)(d);
		(*_d) = (TD)(*_s);
	}

	// 
	typedef void (*CONVERT_FUN)(const void*, void*);
	typedef void (*SOURCE_FUN)(jcparam::ITableRow *, int, void *);
	typedef void (*ATHOP_FUN)(const void * l, const void * r, void * res);
	typedef bool (*RELOP_FUN)(const void * l, const void * r);
	typedef bool (*FROM_IVALUE)(jcparam::IValue * val, void * data);


///////////////////////////////////////////////////////////////////////////////
// -- base
	class no_name
	{
	public:
		static const TCHAR m_name[];
	};


///////////////////////////////////////////////////////////////////////////////
// -- column value
	class CColumnVal
		: public COperatorBase<IAtomOperate, 1, CColumnVal>
		JCIFBASE
	{
	public:
		CColumnVal(const CJCStringT & col_name);
		virtual ~CColumnVal(void);

	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);

	public:
		// 返回计算结果
		virtual const void * GetValue(void)		{	return (void*)(m_res);	}
		virtual jcparam::VALUE_TYPE GetValueType(void) {return m_res_type;}
		virtual void DebugInfo(FILE * outfile);

	// debug info
	public:
		static const TCHAR m_name[];

	protected:
		FROM_IVALUE	m_convertor;
		CJCStringT m_col_name;
		int m_col_id;

		jcparam::VALUE_TYPE	m_res_type;
		BYTE m_res[sizeof(double)];
	};

///////////////////////////////////////////////////////////////////////////////
// -- Variable
	//-- CVariableOp
	// 用于从一个变量中取出其成员。即var1.var2.var3
	// 目前以嵌套方式实现成员变量
	class CVariableOp	
		: public COperatorBase<IAtomOperate, 1, CVariableOp>
		JCIFBASE 
	{
	public:
		// CVariableOp作为计算变量成员使用。
		CVariableOp(const CJCStringT & var_name);

		// CVariableOp作为IValue的封装使用
		CVariableOp(jcparam::IValue * val);
		virtual ~CVariableOp(void);

	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);
		virtual void DebugInfo(FILE * outfile);

		virtual const void * GetValue(void);
		virtual jcparam::VALUE_TYPE GetValueType(void);

	public:
		static const TCHAR m_name[];

	protected:
		// m_paraent是m_source的拷贝，用于表示需要被Invoke。 m_parent不需要引用计数
		CJCStringT m_var_name;
		jcparam::IValue * m_val;
		jcparam::CTypedValueBase *	m_typed_val;
	};



///////////////////////////////////////////////////////////////////////////////
// -- constant
	template <typename DTYPE>
	class CConstantOp	
		: public COpSourceSupport0<CConstantOp<DTYPE>, IOperator >
JCIFBASE 
	{
	protected:
		CConstantOp(const DTYPE & val) : m_val(val) {};
		virtual ~CConstantOp(void) {};

	public:
		friend class CSyntaxParser;
		friend class COpSourceSupport0<CConstantOp<DTYPE>, IOperator >;

	public:
		virtual bool GetResult(jcparam::IValue * & val)
		{
			JCASSERT(NULL == val);
			val = static_cast<jcparam::IValue*>(jcparam::CTypedValue<DTYPE>::Create(m_val));
			return true;
		}
		virtual bool Invoke(void) {return true;}
		// 直接实现SetSource要比继承COpSouceSupport0节省4字节
		virtual void SetSource(UINT src_id, IAtomOperate * op) { JCASSERT(0); }

		virtual const void * GetValue(void)	{ return (void*)(&m_val); }
		virtual jcparam::VALUE_TYPE GetValueType(void) { return jcparam::type_id<DTYPE>::id(); }
		virtual UINT GetProperty(void) const {return 0;} ;
		virtual UINT GetDependency(void) {return 0;} ;
		virtual void DebugInfo(FILE * outfile)
		{
			CJCStringT str;
			jcparam::CConvertor<DTYPE>::T2S(m_val, str);
			stdext::jc_fprintf(outfile, _T("%s"), str.c_str());
		}

	protected:
		DTYPE m_val;
		static const TCHAR name[];
	};

template <typename DTYPE>
const TCHAR CConstantOp<DTYPE>::name[] = _T("constant");

///////////////////////////////////////////////////////////////////////////////
// -- CAthOpBase 算术运算符 
	// 表达式操作符的基础类。支持运行时类型转换。
	//  运行时类型转换：主要针对表格处理的优化。在表格中，每一行中，相同列的类型是相同的。
	//		在第一次运行时，判断操作数的类型并且推导类型转换规则。其Source必须是IOperator接口
	//	CAthOpBase : 运算符的基础类
	//		主要实现：source的处理
	//	其派生类包括：	CRelOpBase:		关系运算符的基础类 （返回类型一定是bool型）
	//					CBoolOpBase:	逻辑运算符基础类	(source和返回类型都是bool型）
	//					CArithOpBase:	算术运算符的基础类

	template <class ATH_OP>
	class CAthOpBase
		: public COperatorBase<IOperator, 2, ATH_OP>
		JCIFBASE
	{
	public:
		CAthOpBase(void);
		virtual ~CAthOpBase(void);
	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);

	public:
		// 返回计算结果
		virtual const void * GetValue(void)		{	return m_res;	}
		virtual jcparam::VALUE_TYPE GetValueType(void) {return m_res_type;}

	// debug info
	protected:
		jcparam::VALUE_TYPE m_res_type;		// 用于决定src的目的类型。

		// 数据缓存，[0]:L, [1]:R, [2]:res
		BYTE m_res[sizeof(double)];

	protected:
		CONVERT_FUN m_conv_l;
		CONVERT_FUN m_conv_r;
		ATHOP_FUN	m_op;
	};

///////////////////////////////////////////////////////////////////////////////
// -- ath operator
	//-- +
	template <typename T>
	void AthAdd(const void * l, const void* r, void * res)
	{	T* _l = (T*)l, * _r = (T*)r; 	*((T*)res) = *_l + *_r; 	}
	struct CAthAdd 
	{	
		static const ATHOP_FUN AthOp[jcparam::VT_MAXNUM - 1];	
		static const TCHAR m_name[];
	};
	typedef CAthOpBase<CAthAdd>	CAthOpAdd;

	//-- -
	template <typename T>
	void AthSub(const void * l, const void* r, void * res)
	{	T* _l = (T*)l, * _r = (T*)r; 	*((T*)res) = *_l - *_r; 	}
	struct CAthSub 
	{	
		static const ATHOP_FUN AthOp[jcparam::VT_MAXNUM - 1];	
		static const TCHAR m_name[];
	};
	typedef CAthOpBase<CAthSub>	CAthOpSub;

///////////////////////////////////////////////////////////////////////////////
// -- CBoolOpBase 逻辑运算符
	template <class BOOL_OP>
	class CBoolOpBase
		: public COperatorBase<IOperator, 2, BOOL_OP>
		JCIFBASE
	{
	public:
		CBoolOpBase(void);
		virtual ~CBoolOpBase(void){};
	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);

	public:
		// 返回计算结果
		virtual const void * GetValue(void)		{	return &m_res;	}
		virtual jcparam::VALUE_TYPE GetValueType(void) {return jcparam::VT_BOOL;}

		// debug info

	protected:
		bool m_res;
	};

	struct CBoolAnd 
	{
		static bool op(bool r, bool l)	{return r && l;} 
		static const TCHAR m_name[];
	};
	typedef CBoolOpBase<CBoolAnd> CBoolOpAnd;

	struct CBoolOr  
	{
		static bool op(bool r, bool l)	{return r || l;} 
		static const TCHAR m_name[];
	};
	typedef CBoolOpBase<CBoolOr>	CBoolOpOr;

	class CBoolOpNot
		: public COperatorBase<IOperator, 1, CBoolOpNot>
		JCIFBASE
	{
	public:
		CBoolOpNot(void) { };
		virtual ~CBoolOpNot(void){};
	public:
		virtual bool GetResult(jcparam::IValue * & val)
		{
			JCASSERT(NULL == val);
			val = static_cast<jcparam::IValue*>(jcparam::CTypedValue<bool>::Create(m_res));
			return m_res;
		}
		virtual bool Invoke(void)
		{
			bool *_r = (bool*)(m_src[0]->GetValue());
			m_res = !(*_r);
			return m_res;
		}

	public:
		// 返回计算结果
		virtual const void * GetValue(void)		{	return &m_res;	}
		virtual jcparam::VALUE_TYPE GetValueType(void) {return jcparam::VT_BOOL;}

		// debug info
	public:
		static const TCHAR m_name[];

	protected:
		bool m_res;
	};

///////////////////////////////////////////////////////////////////////////////
// -- CRelOpBase 关系运算符
	template <class REL_OP>
	class CRelOpBase
		: public COperatorBase<IOperator, 2, REL_OP>
		JCIFBASE
	{
	public:
		CRelOpBase(void);
		virtual ~CRelOpBase(void) {};

	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);

	public:
		// 返回计算结果
		virtual const void * GetValue(void)		{	return &m_res;	}
		virtual jcparam::VALUE_TYPE GetValueType(void) {return jcparam::VT_BOOL;}

	// debug info
	protected:
		jcparam::VALUE_TYPE m_res_type;		// 用于决定src的目的类型。
		bool m_res;

	protected:
		CONVERT_FUN m_conv_l;
		CONVERT_FUN m_conv_r;
		RELOP_FUN	m_op;
	};


///////////////////////////////////////////////////////////////////////////////
// -- relation operator
	//-- ==
	template <typename T>
	bool CmpEQ(const void * l, const void* r)
	{	T* _l = (T*)l, * _r = (T*)r; 		return ( (*_l) == (*_r) ); 	}

	struct CRopEQ 
	{	
		static const RELOP_FUN RelOp[jcparam::VT_MAXNUM - 1];	
		static const TCHAR m_name[];
	};

	typedef CRelOpBase<CRopEQ>	CRelOpEQ;

	//-- <=
	template <typename T>
	bool CmpLT(const void * l, const void* r)
	{	T* _l = (T*)l, * _r = (T*)r; 		return ( (*_l) <= (*_r) ); 	}

	struct CRopLT 
	{
		static const RELOP_FUN RelOp[jcparam::VT_MAXNUM - 1];	
		static const TCHAR m_name[];
	};

	typedef CRelOpBase<CRopLT>	CRelOpLT;

	//-- >=
	template <typename T>
	bool CmpLE(const void * l, const void* r)
	{	T* _l = (T*)l, * _r = (T*)r; 		return ( (*_l) < (*_r) ); 	}

	struct CRopLE 
	{	
		static const RELOP_FUN RelOp[jcparam::VT_MAXNUM - 1];	
		static const TCHAR m_name[];
	};

	typedef CRelOpBase<CRopLE>	CRelOpLE;

///////////////////////////////////////////////////////////////////////////////
// -- pre define variables

	// index, 用于记录当前single st执行的次数
	class CPdvIndex
		: public COperatorBase<IAtomOperate, 1, CPdvIndex>
		JCIFBASE
	{
	public:
		CPdvIndex(void) : m_index(0) {};
		virtual ~CPdvIndex(void) {};

	public:
		virtual bool GetResult(jcparam::IValue * & val) {JCASSERT(0); return false;}
		virtual bool Invoke(void)	{m_index ++; return true;}
		virtual const void * GetValue(void)		{	return (void*)(&m_index);	}
		virtual jcparam::VALUE_TYPE GetValueType(void) {return jcparam::VT_UINT;}

	public:
		static const TCHAR m_name[];

	protected:
		UINT	m_index;
	};

};