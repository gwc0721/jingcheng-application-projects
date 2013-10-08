#include "stdafx.h"
#include "../include/syntax_parser.h"

#pragma warning (disable: 4800)

#include "expression_op.h"

// LOG 输出定义：
//  WARNING:	运行错误
//	NOTICE:		script运行trace
//  TRACE:		调用堆栈

LOCAL_LOGGER_ENABLE(_T("expression_op"), LOGGER_LEVEL_NOTICE);

using namespace jcscript;

///////////////////////////////////////////////////////////////////////////////
void ToIValue(jcparam::VALUE_TYPE type, void * data, jcparam::IValue * & val)
{
	JCASSERT(NULL == val);
	switch (type)
	{
	case jcparam::VT_BOOL:	val = jcparam::CTypedValue<bool>::Create(*((bool*)data));		break;
	case jcparam::VT_CHAR:	val = jcparam::CTypedValue<char>::Create(*((char*)data));		break;
	case jcparam::VT_UCHAR:	val = jcparam::CTypedValue<BYTE>::Create(*((BYTE*)data));		break;
	case jcparam::VT_SHORT:	val = jcparam::CTypedValue<short>::Create(*((short*)data));		break;
	case jcparam::VT_USHORT:	val = jcparam::CTypedValue<WORD>::Create(*((WORD*)data));	break;
	case jcparam::VT_INT:	val = jcparam::CTypedValue<int>::Create(*((int*)data));			break;
	case jcparam::VT_UINT:	val = jcparam::CTypedValue<DWORD>::Create(*((DWORD*)data));		break;
	case jcparam::VT_INT64:	val = jcparam::CTypedValue<INT64>::Create(*((INT64*)data));		break;
	case jcparam::VT_UINT64:	val = jcparam::CTypedValue<UINT64>::Create(*((UINT64*)data));	break;
	case jcparam::VT_FLOAT:	val = jcparam::CTypedValue<float>::Create(*((float*)data));		break;
	case jcparam::VT_DOUBLE:	val = jcparam::CTypedValue<double>::Create(*((double*)data));	break;
	//case VT_STRING:	val = jcparam::CTypedValue<BYTE>::Create(*data);	break;
	}
	return;
}

///////////////////////////////////////////////////////////////////////////////
//-- CAthOpBase


static const CONVERT_FUN CONV_FUN_TAB[jcparam::VT_MAXNUM-1][jcparam::VT_MAXNUM-1] = 
{
	// bool
	{	Convert<bool, bool>,	Convert<bool, char>,	Convert<bool, BYTE>,	Convert<bool, short>,	
		Convert<bool, WORD>,	Convert<bool, int>,		Convert<bool, DWORD>,	Convert<bool, INT64>,	
		Convert<bool, UINT64>,	Convert<bool, float>,	Convert<bool, double>,	NULL},
	// char
	{	Convert<char, bool>,	Convert<char, char>,	Convert<char, BYTE>,	Convert<char, short>,	
		Convert<char, WORD>,	Convert<char, int>,		Convert<char, DWORD>,	Convert<char, INT64>,	
		Convert<char, UINT64>,	Convert<char, float>,	Convert<char, double>, NULL},
	// BYTE
	{	Convert<BYTE, bool>,	Convert<BYTE, char>,	Convert<BYTE, BYTE>,	Convert<BYTE, short>,	
		Convert<BYTE, WORD>,	Convert<BYTE, int>,		Convert<BYTE, DWORD>,	Convert<BYTE, INT64>,	
		Convert<BYTE, UINT64>,	Convert<BYTE, float>,	Convert<BYTE, double>,	NULL},
	// short
	{	Convert<short, bool>,	Convert<short, char>,	Convert<short, BYTE>,	Convert<short, short>,	
		Convert<short, WORD>,	Convert<short, int>,	Convert<short, DWORD>,	Convert<short, INT64>,	
		Convert<short, UINT64>,	Convert<short, float>,	Convert<short, double>, NULL},
	// WORD
	{	Convert<WORD, bool>,	Convert<WORD, char>,	Convert<WORD, BYTE>,	Convert<WORD, short>,	
		Convert<WORD, WORD>,	Convert<WORD, int>,		Convert<WORD, DWORD>,	Convert<WORD, INT64>,	
		Convert<WORD, UINT64>,	Convert<WORD, float>,	Convert<WORD, double>,	NULL},
	// int
	{	Convert<int, bool>,		Convert<int, char>,		Convert<int, BYTE>,		Convert<int, short>,	
		Convert<int, WORD>,		Convert<int, int>,		Convert<int, DWORD>,	Convert<int, INT64>,	
		Convert<int, UINT64>,	Convert<int, float>,	Convert<int, double>,	NULL},
	// DWORD
	{	Convert<DWORD, bool>,	Convert<DWORD, char>,	Convert<DWORD, BYTE>,	Convert<DWORD, short>,	
		Convert<DWORD, WORD>,	Convert<DWORD, int>,	Convert<DWORD, DWORD>,	Convert<DWORD, INT64>,	
		Convert<DWORD, UINT64>,	Convert<DWORD, float>,	Convert<DWORD, double>, NULL},
	// INT64
	{	Convert<INT64, bool>,	Convert<INT64, char>,	Convert<INT64, BYTE>,	Convert<INT64, short>,	
		Convert<INT64, WORD>,	Convert<INT64, int>,	Convert<INT64, DWORD>,	Convert<INT64, INT64>,	
		Convert<INT64, UINT64>,	Convert<INT64, float>,	Convert<INT64, double>, NULL},
	// UINT64
	{	Convert<UINT64, bool>,	Convert<UINT64, char>,	Convert<UINT64, BYTE>,	Convert<UINT64, short>,	
		Convert<UINT64, WORD>,	Convert<UINT64, int>,	Convert<UINT64, DWORD>,	Convert<UINT64, INT64>,	
		Convert<UINT64, UINT64>,Convert<UINT64, float>,	Convert<UINT64, double>,NULL},
	// float
	{	Convert<float, bool>,	Convert<float, char>,	Convert<float, BYTE>,	Convert<float, short>,	
		Convert<float, WORD>,	Convert<float, int>,	Convert<float, DWORD>,	Convert<float, INT64>,	
		Convert<float, UINT64>,	Convert<float, float>,	Convert<float, double>, NULL},
	// double
	{	Convert<double, bool>,	Convert<double, char>,	Convert<double, BYTE>,	Convert<double, short>,	
		Convert<double, WORD>,	Convert<double, int>,	Convert<double, DWORD>,	Convert<double, INT64>,	
		Convert<double, UINT64>,Convert<double, float>, Convert<double, double>, NULL},
	// string
	{	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,NULL, NULL, NULL, NULL, 
	}
	// string
	//{	Convert<bool, bool>,				Convert<bool, char>,	Convert<bool, BYTE>,	Convert<bool, short>,	Convert<bool, WORD>, 
	//	Convert<bool, int>, Convert<bool, DWORD>,	Convert<bool, INT64>,	Convert<bool, UINT64>,	Convert<bool, float>, 
	//	Convert<bool, double>, NULL},
};


CONVERT_FUN GetConvFunc(jcparam::VALUE_TYPE t1, jcparam::VALUE_TYPE t2)
{
	JCASSERT( t1 < jcparam::VT_MAXNUM -1 );
	JCASSERT( t2 < jcparam::VT_MAXNUM -1 );

	return CONV_FUN_TAB[t1-1][t2-1];
}

template <class ATH_OP>
CAthOpBase<ATH_OP>::CAthOpBase(void)
	: m_conv_l(NULL)
	, m_conv_r(NULL)
	, m_op(NULL)
{
}

template <class ATH_OP>
CAthOpBase<ATH_OP>::~CAthOpBase(void)
{
}

template <class ATH_OP>
bool CAthOpBase<ATH_OP>::GetResult(jcparam::IValue * & val)
{
	ToIValue(m_res_type, m_res, val);
	return true;
}

template <class ATH_OP>
bool CAthOpBase<ATH_OP>::Invoke(void)
{
	if ( ! m_op )
	{	// 初始化，设定转换与比较函数

		// 左值的类型，右值的类型
		jcparam::VALUE_TYPE tl, tr;
		// 取得L,R的类型
		tl = m_src[0]->GetValueType();
		tr = m_src[1]->GetValueType();
		// 类型推导
		m_res_type = max(tl, tr);
		// 设置类型转换
		/*if (tl != m_res_type) */m_conv_l = GetConvFunc(tl, m_res_type);
		/*if (tl != m_res_type) */m_conv_r = GetConvFunc(tr, m_res_type);

		// 
		m_op = AthOp[m_res_type];
		JCASSERT(m_op);

		LOG_NOTICE(_T("{%08X} ath %s: type_l=%d, type_r=%d, type_res"), 
			static_cast<IAtomOperate*>(this), _T("+"), tl, tr, m_res_type);
	}
	m_conv_l(m_src[0]->GetValue(), m_sl);
	m_conv_r(m_src[1]->GetValue(), m_sr);
	m_op(m_sl, m_sr, m_res);

	LOG_NOTICE(_T("{%08X} ath %s: "), 
		static_cast<IAtomOperate*>(this), _T("+") );
	return true;
}


///////////////////////////////////////////////////////////////////////////////
//-- relation operator

const ATHOP_FUN CAthAdd::AthOp[] = {
	AthAdd<bool>, AthAdd<char>, AthAdd<BYTE>, AthAdd<short>, AthAdd<WORD>,
	AthAdd<int>, AthAdd<DWORD>, AthAdd<INT64>, AthAdd<UINT64>, AthAdd<float>, AthAdd<double>, NULL, 
};
template class CAthOpBase<CAthAdd>;


const ATHOP_FUN CAthSub::AthOp[] = {
	AthSub<bool>, AthSub<char>, AthSub<BYTE>, AthSub<short>, AthSub<WORD>,
	AthSub<int>, AthSub<DWORD>, AthSub<INT64>, AthSub<UINT64>, AthSub<float>, AthSub<double>, NULL, 
};
template class CAthOpBase<CAthSub>;


///////////////////////////////////////////////////////////////////////////////
//-- Column Value

LOG_CLASS_SIZE(CColumnVal);

CColumnVal::CColumnVal(const CJCStringT & col_name)
	: m_col_name(col_name)
	, m_res_type(jcparam::VT_UNKNOW)
{
}

CColumnVal::~CColumnVal(void)
{
}

bool CColumnVal::GetResult(jcparam::IValue * & val)
{
	ToIValue(m_res_type, m_res, val);
	return true;
}

bool CColumnVal::Invoke(void)
{
	LOG_STACK_TRACE();
	JCASSERT(m_src[0]);

	stdext::auto_interface<jcparam::IValue> val;
	m_src[0]->GetResult(val);
	jcparam::ITableRow * row = NULL;
	row = val.d_cast<jcparam::ITableRow*>();

	if (!row) THROW_ERROR(ERR_USER, _T("input is not a row"));

	if (jcparam::VT_UNKNOW == m_res_type)
	{
		const jcparam::CColInfoBase * info = row->GetColumnInfo(m_col_name.c_str());
		if (!info) THROW_ERROR(ERR_USER, _T("Unknow column %s"), m_col_name.c_str() );
		m_res_type = info->m_type;
		m_col_id = info->m_id;
		// 选择转换函数
		LOG_NOTICE(_T("{%08X} col_val: col=%d, type=%d"), 
			static_cast<IAtomOperate*>(this), m_col_id, m_res_type);
	}
	row->GetColVal(m_col_id, m_res);

	LOG_NOTICE(_T("{%08X} col_val: val=%X"), 
		static_cast<IAtomOperate*>(this), *((UINT*)(m_res)) );
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//-- bool operator
template <class BOOL_OP>
CBoolOpBase<BOOL_OP>::CBoolOpBase(void)
{
}

template <class BOOL_OP>
bool CBoolOpBase<BOOL_OP>::GetResult(jcparam::IValue * & val)
{
	JCASSERT(NULL == val);
	val = static_cast<jcparam::IValue*>(jcparam::CTypedValue<bool>::Create(m_res));
	return m_res;
}

template <class BOOL_OP>
bool CBoolOpBase<BOOL_OP>::Invoke(void)
{
	bool *_r = (bool*)(m_src[0]->GetValue());
	bool *_l = (bool*)(m_src[1]->GetValue());
	m_res = op(* _r, * _l);
	return m_res;
}

template class CBoolOpBase<CBoolAnd>;
template class CBoolOpBase<CBoolOr>;

///////////////////////////////////////////////////////////////////////////////
//-- Constant
LOG_CLASS_SIZE_T(CConstantOp<int>, 1)
LOG_CLASS_SIZE_T(CConstantOp<INT64>, 2)


///////////////////////////////////////////////////////////////////////////////
//-- relation operator
template <class REL_OP>
CRelOpBase<REL_OP>::CRelOpBase(void)
	: m_op(NULL), m_conv_l(NULL), m_conv_r(NULL)
{
}

template <class REL_OP>
bool CRelOpBase<REL_OP>::GetResult(jcparam::IValue * & val)
{
	JCASSERT(NULL == val);
	val = static_cast<jcparam::IValue*>(jcparam::CTypedValue<bool>::Create(m_res));
	return m_res;
}

template <class REL_OP>
bool CRelOpBase<REL_OP>::Invoke(void)
{
	if ( ! m_op )
	{	// 初始化，设定转换与比较函数

		// 左值的类型，右值的类型
		jcparam::VALUE_TYPE tl, tr;
		// 取得L, R的类型
		tl = m_src[0]->GetValueType();
		tr = m_src[1]->GetValueType();
		// 类型推导
		m_res_type = max(tl, tr);
		// 设置类型转换
		/*if (tl != m_res_type) */m_conv_l = GetConvFunc(tl, m_res_type);
		/*if (tr != m_res_type) */m_conv_r = GetConvFunc(tr, m_res_type);

		m_op = RelOp[m_res_type];
		JCASSERT(m_op);
	}
	m_conv_l(m_src[0]->GetValue(), m_sl);
	m_conv_r(m_src[1]->GetValue(), m_sr);
	m_res = m_op(m_sl, m_sr);
	return m_res;
}


///////////////////////////////////////////////////////////////////////////////
//-- relation operator
LOG_CLASS_SIZE(CRopEQ);
LOG_CLASS_SIZE(CRelOpEQ);

const RELOP_FUN CRopEQ::RelOp[] = {
	CmpEQ<bool>, CmpEQ<char>, CmpEQ<BYTE>, CmpEQ<short>, CmpEQ<WORD>,
	CmpEQ<int>, CmpEQ<DWORD>, CmpEQ<INT64>, CmpEQ<UINT64>, CmpEQ<float>, CmpEQ<double>, NULL, 
};

template class CRelOpBase<CRopEQ>;

LOG_CLASS_SIZE(CRelOpLT);

const RELOP_FUN CRopLT::RelOp[] = {
	CmpLT<bool>, CmpLT<char>, CmpLT<BYTE>, CmpLT<short>, CmpLT<WORD>,
	CmpLT<int>, CmpLT<DWORD>, CmpLT<INT64>, CmpLT<UINT64>, CmpLT<float>, CmpLT<double>, NULL, 
};

template class CRelOpBase<CRopLT>;

LOG_CLASS_SIZE(CRelOpLE);

const RELOP_FUN CRopLE::RelOp[] = {
	CmpLE<bool>, CmpLE<char>, CmpLE<BYTE>, CmpLE<short>, CmpLE<WORD>,
	CmpLE<int>, CmpLE<DWORD>, CmpLE<INT64>, CmpLE<UINT64>, CmpLE<float>, CmpLE<double>, NULL, 
};

template class CRelOpBase<CRopLE>;