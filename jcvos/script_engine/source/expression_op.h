#pragma once

#include "../include/iplugin.h"

namespace jcscript
{
	template <typename TS, typename TD>		// 元类型，结果类型
	void Convert(void * s, void * d)
	{
		TS* _s = (TS*)(s);
		TD* _d = (TD*)(d);
		(*_d) = (TD)(*_s);
	}

	// 
	typedef void (*CONVERT_FUN)(void*, void*);
	typedef void (*SOURCE_FUN)(jcparam::ITableRow *, int, void *);
	typedef void (*ATHOP_FUN)(void * l, void * r, void * res);
	typedef bool (*RELOP_FUN)(void * l, void * r);

	class IOperator : virtual public IAtomOperate
	{
	public:
		virtual void * GetValue(void) = 0;
		virtual jcparam::VALUE_TYPE GetValueType(void) = 0;
	};

	template <class SRC_TYPE, UINT src_num>
	class COpSourceSupport : virtual public IAtomOperate
	{
	public:
		COpSourceSupport(void) {memset(m_src, 0, sizeof(SRC_TYPE*) * src_num); }
		virtual ~COpSourceSupport(void) { for (UINT ii = 0; ii < src_num; ++ii) if (m_src[ii]) m_src[ii]->Release(); }

	public:
		virtual void SetSource(UINT src_id,  IAtomOperate * op)
		{
			JCASSERT(src_id < src_num);
			JCASSERT(NULL == m_src[src_id]);
			m_src[src_id] = dynamic_cast<SRC_TYPE*>(op);
			JCASSERT(m_src[src_id]);
			/*if (m_src[src_id]) */m_src[src_id]->AddRef();
		}
	protected:
		SRC_TYPE * m_src[src_num];
	};

	class COpSourceSupport0 : virtual public IAtomOperate
	{
	public:
		virtual void SetSource(UINT src_id,  IAtomOperate * op)	{ JCASSERT(0); }
	};

///////////////////////////////////////////////////////////////////////////////
// -- debug information support for IAtomOperate
	class CDebugInfo0
		: virtual public IAtomOperate
	{
	public:
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile)
		{
			// format: name, [this ptr], <source 1 ptr>, <source 2 ptr>, other information
			stdext::jc_fprintf(outfile, indentation);
			stdext::jc_fprintf(outfile, _T("%s, [%08X], <%08X>, <%08X>,"),
				name(), (UINT)(static_cast<IAtomOperate*>(this)),
				(UINT)( source(0) ), (UINT)( source(1) ) );
			DebugInfo(outfile);
			stdext::jc_fprintf(outfile, _T("\n") );
		}

		virtual IAtomOperate * source(UINT src) {return NULL;}
		virtual LPCTSTR name() const {return _T(""); };
		virtual void DebugInfo(FILE * outfile) {};
	};

///////////////////////////////////////////////////////////////////////////////
// -- column value
	class CColumnVal
		: virtual public IOperator
		, public COpSourceSupport<IAtomOperate, 1>
		, public CDebugInfo0
		, public CJCInterfaceBase
	{
	public:
		CColumnVal(const CJCStringT & col_name);
		virtual ~CColumnVal(void);

	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);

	public:
		// 返回计算结果
		virtual void * GetValue(void)		{	return (void*)(m_res);	}
		virtual jcparam::VALUE_TYPE GetValueType(void) {return m_res_type;}

	// debug info
	virtual IAtomOperate * source(UINT src) {return m_src[0];}
	virtual LPCTSTR name() const {return _T("column_val"); };
	virtual void DebugInfo(FILE * outfile) 
	{
		stdext::jc_fprintf(outfile, _T("col=%s"), m_col_name.c_str()); 
	};

	protected:
		CJCStringT m_col_name;
		int m_col_id;

		jcparam::VALUE_TYPE	m_res_type;
		BYTE m_res[sizeof(double)];
	};

///////////////////////////////////////////////////////////////////////////////
// -- constant
	template <typename DTYPE>
	class CConstantOp	
		: virtual public IOperator
		, public CDebugInfo0
		, public CJCInterfaceBase 
	{
	protected:
		CConstantOp(const DTYPE & val) : m_val(val) {};
		virtual ~CConstantOp(void) {};

	public:
		friend class CSyntaxParser;

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

		virtual void * GetValue(void)	{ return (void*)(&m_val); }
		virtual jcparam::VALUE_TYPE GetValueType(void) { return jcparam::type_id<DTYPE>::id(); }

	protected:
		DTYPE m_val;

	// debug information support
	protected:
		virtual LPCTSTR name() const {return _T("constant");}
		virtual void DebugInfo(FILE * outfile) const { }
	};


///////////////////////////////////////////////////////////////////////////////
// -- base support
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
		: virtual public IOperator
		, public ATH_OP
		, public COpSourceSupport<IOperator, 2>
		, public CDebugInfo0
		, public CJCInterfaceBase
	{
	public:
		CAthOpBase(void);
		virtual ~CAthOpBase(void);
	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);

	public:
		// 返回计算结果
		virtual void * GetValue(void)		{	return m_res;	}
		virtual jcparam::VALUE_TYPE GetValueType(void) {return m_res_type;}

	// debug info
	virtual IAtomOperate * source(UINT src) { return m_src[src];}
	virtual LPCTSTR name() const {return _T("ath_op"); };

	protected:
		jcparam::VALUE_TYPE m_res_type;		// 用于决定src的目的类型。

		//jcparam::VALUE_TYPE	m_res_type;
		// 数据缓存，[0]:L, [1]:R, [2]:res
		BYTE m_sl[sizeof(double)];
		BYTE m_sr[sizeof(double)];
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
	void AthAdd(void * l, void* r, void * res)
	{	T* _l = (T*)l, * _r = (T*)r; 	*((T*)res) = *_l + *_r; 	}
	struct CAthAdd {	static const ATHOP_FUN AthOp[jcparam::VT_MAXNUM - 1];	};
	typedef CAthOpBase<CAthAdd>	CAthOpAdd;

	//-- -
	template <typename T>
	void AthSub(void * l, void* r, void * res)
	{	T* _l = (T*)l, * _r = (T*)r; 	*((T*)res) = *_l - *_r; 	}
	struct CAthSub {	static const ATHOP_FUN AthOp[jcparam::VT_MAXNUM - 1];	};
	typedef CAthOpBase<CAthSub>	CAthOpSub;

///////////////////////////////////////////////////////////////////////////////
// -- bool operator
	template <class BOOL_OP>
	class CBoolOpBase
		: virtual public IOperator
		, public BOOL_OP
		, public COpSourceSupport<IOperator, 2>
		, public CDebugInfo0
		, public CJCInterfaceBase
	{
	public:
		CBoolOpBase(void);
		virtual ~CBoolOpBase(void){};
	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);

	public:
		// 返回计算结果
		virtual void * GetValue(void)		{	return &m_res;	}
		virtual jcparam::VALUE_TYPE GetValueType(void) {return jcparam::VT_BOOL;}

		// debug info
		virtual IAtomOperate * source(UINT src) { return m_src[src];}
		virtual LPCTSTR name() const {return _T("bool_op"); };

	protected:
		bool m_res;
	};

	struct CBoolAnd {bool op(bool r, bool l)	{return r && l;} };
	typedef CBoolOpBase<CBoolAnd> CBoolOpAnd;

	struct CBoolOr  {bool op(bool r, bool l)	{return r || l;} };
	typedef CBoolOpBase<CBoolOr>	CBoolOpOr;

	class CBoolOpNot
		: virtual public IOperator
		, public COpSourceSupport<IOperator, 1>
		, public CDebugInfo0
		, public CJCInterfaceBase
	{
	public:
		CBoolOpNot(void) {};
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
		virtual void * GetValue(void)		{	return &m_res;	}
		virtual jcparam::VALUE_TYPE GetValueType(void) {return jcparam::VT_BOOL;}

		// debug info
		virtual IAtomOperate * source(UINT src) { return m_src[0];}
		virtual LPCTSTR name() const {return _T("not"); };

	protected:
		bool m_res;
	};
///////////////////////////////////////////////////////////////////////////////
// -- relation operator
	template <class REL_OP>
	class CRelOpBase
		: virtual public IOperator
		, public REL_OP
		, public COpSourceSupport<IOperator, 2>
		, public CDebugInfo0
		, public CJCInterfaceBase
	{
	public:
		CRelOpBase(void);
		virtual ~CRelOpBase(void) {};

	public:
		virtual bool GetResult(jcparam::IValue * & val);
		virtual bool Invoke(void);

	public:
		// 返回计算结果
		virtual void * GetValue(void)		{	return &m_res;	}
		virtual jcparam::VALUE_TYPE GetValueType(void) {return jcparam::VT_BOOL;}

	// debug info
	virtual IAtomOperate * source(UINT src) { return m_src[src];}
	virtual LPCTSTR name() const {return _T("rop"); };

	protected:
		//IOperator * m_src[2];
		jcparam::VALUE_TYPE m_res_type;		// 用于决定src的目的类型。

		BYTE m_sl[sizeof(double)];
		BYTE m_sr[sizeof(double)];
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
	bool CmpEQ(void * l, void* r)
	{	T* _l = (T*)l, * _r = (T*)r; 		return ( (*_l) == (*_r) ); 	}

	struct CRopEQ {	static const RELOP_FUN RelOp[jcparam::VT_MAXNUM - 1];	};

	typedef CRelOpBase<CRopEQ>	CRelOpEQ;

	//-- <=
	template <typename T>
	bool CmpLT(void * l, void* r)
	{	T* _l = (T*)l, * _r = (T*)r; 		return ( (*_l) <= (*_r) ); 	}

	struct CRopLT {	static const RELOP_FUN RelOp[jcparam::VT_MAXNUM - 1];	};

	typedef CRelOpBase<CRopLT>	CRelOpLT;

	//-- >=
	template <typename T>
	bool CmpLE(void * l, void* r)
	{	T* _l = (T*)l, * _r = (T*)r; 		return ( (*_l) < (*_r) ); 	}

	struct CRopLE {	static const RELOP_FUN RelOp[jcparam::VT_MAXNUM - 1];	};

	typedef CRelOpBase<CRopLE>	CRelOpLE;
};