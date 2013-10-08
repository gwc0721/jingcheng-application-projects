#include "stdafx.h"
#include "../include/syntax_parser.h"
#include "atom_operates.h"

// LOG 输出定义：
//  WARNING:	运行错误
//	NOTICE:		script运行trace
//  TRACE:		调用堆栈

LOCAL_LOGGER_ENABLE(_T("atom_operates"), LOGGER_LEVEL_WARNING);

using namespace jcscript;

///////////////////////////////////////////////////////////////////////////////
//-- CAssignOp
LOG_CLASS_SIZE(CAssignOp)

CAssignOp::CAssignOp(IAtomOperate * dst_op, const CJCStringT & dst_name)
	: m_src_op(NULL)
	, m_dst_op(dst_op)
	, m_dst_name(dst_name)
{
	LOG_STACK_TRACE();
	JCASSERT(m_dst_op);
	m_dst_op->AddRef();
	//strcpy_s(_class_name, "Assign");
}

CAssignOp::~CAssignOp(void)
{
	LOG_STACK_TRACE();
	if (m_dst_op) m_dst_op->Release();
	if (m_src_op) m_src_op->Release();
}

bool CAssignOp::GetResult(jcparam::IValue * & val)
{
	// 应当不会被调用
	LOG_STACK_TRACE();
	JCASSERT(0);
	return false;
}

bool CAssignOp::Invoke(void)
{
	LOG_STACK_TRACE();
	JCASSERT(m_src_op);
	JCASSERT(m_dst_op);

	jcparam::IValue *val = NULL;
	m_src_op->GetResult(val);

	if (val)
	{
#if 1
		// 赋值的实现1: m_dst_op基于IAtomOperate的界面
		jcparam::IValue * val_dst = NULL;
		m_dst_op->GetResult(val_dst);
		val_dst->SetSubValue(m_dst_name.c_str(), val);
		val_dst->Release();
#else
		// 赋值的实现2：m_dst_op基于IVariableSet / IFeature的界面
		m_dst_op->SetVariable(m_dst_name, val);
		// or
		m_dst_op->PushParameter(m_dst_name, val);
#endif
		val->Release();
	}

	return true;
}

void CAssignOp::SetSource(UINT src_id, IAtomOperate * op)
{
	LOG_STACK_TRACE();
	JCASSERT(op);
	JCASSERT(NULL == m_src_op);
	
	m_src_op = op;
	op->AddRef();
}

#ifdef _DEBUG
void CAssignOp::DebugOutput(LPCTSTR indentation, FILE * outfile)
{
	stdext::jc_fprintf(outfile, indentation);
	stdext::jc_fprintf(outfile, _T("assign, [%08X], <%08X>, dst=<%08X>, var=%s\n"),
		(UINT)(static_cast<IAtomOperate*>(this)),
		(UINT)(m_src_op), 
		(UINT)(m_dst_op), m_dst_name.c_str());
}
#endif


///////////////////////////////////////////////////////////////////////////////
//-- CPushParamOp
LOG_CLASS_SIZE(CPushParamOp)

CPushParamOp::CPushParamOp(IFeature * dst_op, const CJCStringT & dst_name)
	: m_src_op(NULL)
	, m_function(dst_op)
	, m_param_name(dst_name)
{
	LOG_STACK_TRACE();
	JCASSERT(m_function);
	m_function->AddRef();
	//strcpy_s(_class_name, "PushOp");
}

CPushParamOp::~CPushParamOp(void)
{
	LOG_STACK_TRACE();
	if (m_function) m_function->Release();
	if (m_src_op) m_src_op->Release();
}

bool CPushParamOp::GetResult(jcparam::IValue * & val)
{
	// 应当不会被调用
	LOG_STACK_TRACE();
	JCASSERT(0);
	return false;
}

bool CPushParamOp::Invoke(void)
{
	LOG_STACK_TRACE_O(_T(""));
	JCASSERT(m_src_op);
	JCASSERT(m_function);

	LOG_NOTICE(_T("{%08X} push_param  <%08X> -> <%08X>:%s"), 
		static_cast<IAtomOperate*>(this),
		(UINT)(m_src_op), (UINT)(m_function), m_param_name.c_str() );

	jcparam::IValue *val = NULL;
	m_src_op->GetResult(val);

	m_function->PushParameter(m_param_name, val);
	val->Release();
	return true;
}

void CPushParamOp::SetSource(UINT src_id, IAtomOperate * op)
{
	LOG_STACK_TRACE();
	JCASSERT(op);
	JCASSERT(NULL == m_src_op);
	
	m_src_op = op;
	op->AddRef();
}

#ifdef _DEBUG
void CPushParamOp::DebugOutput(LPCTSTR indentation, FILE * outfile)
{
	stdext::jc_fprintf(outfile, indentation);
	stdext::jc_fprintf(outfile, _T("push_param, [%08X], <%08X>, func=<%08X>, param=%s\n"),
		(UINT)(static_cast<IAtomOperate*>(this)),
		(UINT)(m_src_op), 
		(UINT)(static_cast<IAtomOperate*>(m_function)), m_param_name.c_str());
}
#endif

///////////////////////////////////////////////////////////////////////////////
//-- CVariableOp
LOG_CLASS_SIZE(CVariableOp)

CVariableOp::CVariableOp(CVariableOp * parent, const CJCStringT & var_name)
	: /*m_source(NULL)
	, */m_parent(NULL)
	, m_val(NULL)
	, m_var_name(var_name)
{
	LOG_STACK_TRACE();
	m_parent = parent;
	if (m_parent) 
	{
		m_src[0] = static_cast<IAtomOperate*>(m_parent);
		m_src[0]->AddRef();
	}
}

CVariableOp::CVariableOp(jcparam::IValue * val)
	: /*m_source(NULL)
	, */m_parent(NULL)
	, m_val(val)
{
	if (m_val) m_val->AddRef();
}

CVariableOp::~CVariableOp(void)
{
	LOG_STACK_TRACE();
	//if (m_source) m_source->Release();
	if (m_val) m_val->Release();
}


bool CVariableOp::GetResult(jcparam::IValue * & val)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == val);

	val = m_val;
	if (val) val->AddRef();
	return true;
}

bool CVariableOp::Invoke(void)
{
	LOG_STACK_TRACE();

	if (m_parent) m_parent->Invoke();
	if (m_src[0])
	{
		JCASSERT(NULL == m_val);
		jcparam::IValue * val = NULL;
		m_src[0]->GetResult(val);		// 如果m_parent被设，m_src[0]一定等于m_parent。
		val->GetSubValue(m_var_name.c_str(), m_val);
		val->Release();
	}
	return true;
}

//void CVariableOp::SetSource(UINT src_id, IAtomOperate * op)
//{
//	LOG_STACK_TRACE();
//	JCASSERT(op);
//	JCASSERT(NULL == m_source);
//
//	m_source = op;
//	m_source->AddRef();
//}

void CVariableOp::SetVariableName(const CJCStringT & name)
{
	LOG_STACK_TRACE();
	m_var_name = name;
}

#ifdef _DEBUG
void CVariableOp::DebugOutput(LPCTSTR indentation, FILE * outfile)
{
	stdext::jc_fprintf(outfile, indentation);
	stdext::jc_fprintf(outfile, _T("variable, [%08X], <%08X>, sub=%s\n"),
		(UINT)(static_cast<IAtomOperate*>(this)),
		(UINT)(m_src[0]), 
		m_var_name.c_str());

	if (m_parent) m_parent->DebugOutput(indentation-1, outfile);
}
#endif

///////////////////////////////////////////////////////////////////////////////
//-- CSaveToFileOp
LOG_CLASS_SIZE(CSaveToFileOp)

CSaveToFileOp::CSaveToFileOp(const CJCStringT & filename)
	: m_file_name(filename)
	, m_src_op(NULL)
	, m_file(NULL)
{
}
		
CSaveToFileOp::~CSaveToFileOp(void)
{
	if (m_file)		fclose(m_file);
	if (m_src_op)	m_src_op->Release();
}

bool CSaveToFileOp::Invoke(void)
{
	// 支持逐次运行
	LOG_STACK_TRACE();
	JCASSERT(m_src_op);

	// 行写入
	stdext::auto_interface<jcparam::IValue> val;
	m_src_op->GetResult(val);
	if ( !val) 
	{	// warnint
		return true;
	}
	stdext::auto_cif<jcparam::IValueFormat>	format;
	val->QueryInterface(jcparam::IF_NAME_VALUE_FORMAT, format);
	if ( ! format)
	{	// !! warning
		return true;
	}

	if (NULL == m_file)
	{	// 初始化
		_tfopen_s(&m_file, m_file_name.c_str(), _T("w+b"));
		if ( !m_file ) THROW_ERROR(ERR_PARAMETER, 
			_T("failure on openning file %s"), m_file_name.c_str());
		format->WriteHeader(m_file);
	}

	format->Format(m_file, _T("") );
	return true;
}

void CSaveToFileOp::SetSource(UINT src_id, IAtomOperate * op)
{
	LOG_STACK_TRACE();
	JCASSERT(op);
	m_src_op = op;
	m_src_op->AddRef();
}

#ifdef _DEBUG
void CSaveToFileOp::DebugOutput(LPCTSTR indentation, FILE * outfile)
{
	stdext::jc_fprintf(outfile, indentation);
	stdext::jc_fprintf(outfile, _T("save_file, [%08X], <%08X>, fn=%s\n"),
		(UINT)(static_cast<IAtomOperate*>(this)),
		(UINT)(m_src_op), 
		m_file_name.c_str());
}
#endif


///////////////////////////////////////////////////////////////////////////////
//-- CExitOp
LOG_CLASS_SIZE(CExitOp)
bool CExitOp::Invoke(void)
{
	_tprintf( _T("Bye bye!\r\n") );
	throw CExitException();
	return false;
}

#ifdef _DEBUG
void CExitOp::DebugOutput(LPCTSTR indentation, FILE * outfile)
{
	stdext::jc_fprintf(outfile, indentation);
	stdext::jc_fprintf(outfile, _T("exit, [%08X], <%08X>\n"),
		(UINT)(static_cast<IAtomOperate*>(this)), (UINT)(0) );
}
#endif	

///////////////////////////////////////////////////////////////////////////////
// filter statement
LOG_CLASS_SIZE(CFilterSt);

CFilterSt::CFilterSt(void)
	: m_row(NULL)
	, m_init(false)
{
}
		
CFilterSt::~CFilterSt(void)
{
	if (m_row) m_row->Release();
}

bool CFilterSt::GetResult(jcparam::IValue * & val) 
{
	JCASSERT(NULL == val);
	val = m_row;
	if (val) val->AddRef();
	return true;
}

bool CFilterSt::Invoke(void)
{
	// src0: the source of bool expression.
	// src1: the source of table
	if (! m_init)
	{
		JCASSERT(m_src[SRC_TAB]);
		JCASSERT(m_src[SRC_EXP]);
		m_init = true;
	}

	if (m_row)	m_row->Release(), m_row = NULL;

	stdext::auto_interface<jcparam::IValue> val;
	stdext::auto_interface<jcparam::IValue> _row;
	if ( m_src[SRC_EXP]->GetResult(val) )
	{
		m_src[SRC_TAB]->GetResult(_row);
		m_row = _row.d_cast<jcparam::ITableRow*>();
		if ( !m_row ) THROW_ERROR(ERR_USER, _T("The input of filter should be a row"));
		m_row->AddRef();
	}
	return true;
}

