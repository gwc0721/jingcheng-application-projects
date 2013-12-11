#include "stdafx.h"
#include "atom_operates.h"

// jcscript log 使用规则
//   编译器部分：	DEBUGINFO : 用于编译器本身调试
//					TRACE :		用于编译器本身的跟踪
//					NOTICE :	输出编译结果，用于脚本调试
//					WARNING :
//   执行器部分：	DEBUGINFO : 用于执行器本身调试
//					TRACE :		用于执行器本身的跟踪
//					NOTICE :	输出执行结果，用于脚本调试
//					WARNING :

LOCAL_LOGGER_ENABLE(_T("CComboStatement"), LOGGER_LEVEL_WARNING);

using namespace jcscript;

///////////////////////////////////////////////////////////////////////////////
//-- CLoopVarOp
LOG_CLASS_SIZE(CLoopVarOp)

CLoopVarOp::CLoopVarOp(void)
	: m_table(NULL)
	, m_row_val(NULL)
	, m_source(NULL)
	, m_table_size(0)
	, m_cur_index(0)
	, m_dependency(0)
{
	LOG_STACK_TRACE();
}

CLoopVarOp::~CLoopVarOp(void)
{
	LOG_STACK_TRACE();
	if (m_table)	m_table->Release();
	if (m_row_val)	m_row_val->Release();
	if (m_source)	m_source->Release();
}

bool CLoopVarOp::GetResult(jcparam::IValue * & val)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == val);
	JCASSERT(m_row_val);

	val = m_row_val;
	val->AddRef();
	return true;
}

bool CLoopVarOp::PopupResult(jcparam::ITableRow * & val)
{
	JCASSERT(NULL == val);
	if (!m_row_val) return false;

	val = dynamic_cast<jcparam::ITableRow*>(m_row_val);
	if (val) m_row_val = NULL;
	return true;
}

bool CLoopVarOp::Invoke(void)
{
	LOG_STACK_TRACE();
	JCASSERT(m_source);

	if (!m_table) Init();
	return InvokeOnce();
}

void CLoopVarOp::SetSource(UINT src_id, IAtomOperate * op)
{
	LOG_STACK_TRACE();
	JCASSERT(op);
	JCASSERT(NULL == m_source);

	m_source = op;
	op->AddRef();

	m_dependency = m_source->GetDependency() + 1;
}

void CLoopVarOp::GetProgress(JCSIZE &cur_prog, JCSIZE &total_prog) const
{
	cur_prog = m_cur_index;
	total_prog = m_table_size;
}

void CLoopVarOp::Init(void)
{
	LOG_STACK_TRACE();
	JCASSERT(m_source);
	// 第一次运行时获取表格
	JCASSERT(NULL == m_table);		// 不能重复调用
	jcparam::IValue * val = NULL;
	m_source->GetResult(val);
	m_table = dynamic_cast<jcparam::ITable*>(val);
	if (!m_table) 
	{
		if (val) val->Release();
		THROW_ERROR(ERR_PARAMETER, _T("loop variable must be a table."));
	}
	m_table_size = m_table->GetRowSize();
	m_cur_index = 0;
}

bool CLoopVarOp::InvokeOnce(void)
{
	if (m_cur_index >= m_table_size) return false;	//返回false表示循环结束
	if (m_row_val)	m_row_val->Release(), m_row_val = NULL;
	m_table->GetRow(m_cur_index, m_row_val);
	JCASSERT(m_row_val);
	m_cur_index ++;
	return true;
}


#ifdef _DEBUG
void CLoopVarOp::DebugOutput(LPCTSTR indentation, FILE * outfile)
{
	stdext::jc_fprintf(outfile, indentation);
	stdext::jc_fprintf(outfile, _T("loop_var, [%08X], <%08X>\n"), 
		(UINT)((this)),
		(UINT)(m_source));
}
#endif


///////////////////////////////////////////////////////////////////////////////
//-- CCollectOp
LOG_CLASS_SIZE(CCollectOp)

CCollectOp::CCollectOp(void)
	: m_source(NULL)
	, m_table(NULL)
{
	//strcpy(_class_name, "CollOp");
}

CCollectOp::~CCollectOp(void)
{
	if (m_source)	m_source->Release();
	if (m_table)	m_table->Release();
}

bool CCollectOp::GetResult(jcparam::IValue * & val)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == val);
	if (m_table)
	{
		val = static_cast<jcparam::IValue*>(m_table);
		val->AddRef();
	}
	return true;
}

bool CCollectOp::Invoke(void)
{
	LOG_STACK_TRACE();

	LOG_NOTICE(_T("collect"));

	JCASSERT(m_source);

	jcparam::IValue * pre_out = NULL;
	m_source->GetResult(pre_out);

	if (pre_out)
	{
		jcparam::ITable * _tab = NULL;
		jcparam::ITableRow * _row = NULL;
		// here '=' is correct, rather than '=='
		if (_tab = dynamic_cast<jcparam::ITable*>(pre_out) )
		{	// function返回table
			if (m_table)	m_table->Append(_tab);
			else			m_table = _tab, m_table->AddRef();
		}
		// here '=' is correct, rather than '=='
		else if (_row  = dynamic_cast<jcparam::ITableRow*>(pre_out))
		{
			if ( !m_table)	_row->CreateTable(m_table);
			m_table->AddRow(_row);
		}
		pre_out->Release();
	}

	return true;
}

void CCollectOp::SetSource(UINT src_id, IAtomOperate * op)
{
	LOG_STACK_TRACE();
	JCASSERT(op);

	m_source = op;
	m_source->AddRef();
}

#ifdef _DEBUG
void CCollectOp::DebugOutput(LPCTSTR indentation, FILE * outfile)
{
	stdext::jc_fprintf(outfile, indentation);
	stdext::jc_fprintf(outfile, _T("collect, [%08X], <%08X>\n"), 
		(UINT)(static_cast<IAtomOperate*>(this)),
		(UINT)(m_source));
}
#endif


/*

///////////////////////////////////////////////////////////////////////////////
//-- CComboStatement
LOG_CLASS_SIZE(CComboStatement)

// 关于combo的优化方法
//	(1)当combo语句中指定@符号时，@符号后的字句作为循环变量
//	(2)否则，如果第一个feature支持 ILoopOp接口，则将第一个feature作为循环变量
//  (3)否则，没有循环
//  优化，1, 将所有不依赖于循环变量的op提取到上层

CComboStatement::CComboStatement(void) 
	: m_assignee(NULL),
	m_result(NULL),
	m_closed(false),
	m_loop(NULL)
	, m_last_chain(NULL)
{
	//m_loop = new CLoopOp;
}

CComboStatement::~CComboStatement(void) 
{
	DeleteOpList(m_prepro_operates);
	if (m_assignee) m_assignee->Release();
	if (m_loop) m_loop->Release();
}
	
void CComboStatement::Create(CComboStatement * & proxy)
{
	JCASSERT(NULL == proxy);
	//proxy = new CComboStatement;
}

void CComboStatement::AddLoopOp(IAtomOperate * op)
{
	JCASSERT(m_loop);
	m_loop->AddLoopOp(op);
}

void CComboStatement::AddPreproOp(IAtomOperate * op)
{
	LOG_STACK_TRACE();
	JCASSERT(op);

	m_prepro_operates.push_back(op);
	op->AddRef();
}

//bool CComboStatement::SetLoopVar(IAtomOperate * expr_op)
//{
//	LOG_STACK_TRACE();
//	JCASSERT(expr_op);
//
//	if (m_loop->m_loop_var) return false;
//
//	ILoopOperate * loop = dynamic_cast<ILoopOperate*>(expr_op);
//
//	if (! loop)
//	{	// 如果不支持expr_op支持循环接口，否则作为变量处理
//		AddPreproOp(expr_op);
//		CLoopVarOp * loop_var = new CLoopVarOp;
//		loop_var->SetSource(0, expr_op);
//		loop = static_cast<ILoopOperate*>(loop_var);
//	}
//	else	loop->AddRef();
//
//	m_loop->SetLoopVar(loop);
//	loop->Release();
//
//	return true;
//}

//void  CComboStatement::GetLoopVar(IAtomOperate * & op)
//{
//	LOG_STACK_TRACE();
//	JCASSERT(NULL == op);
//	if (m_loop) op = static_cast<IAtomOperate*>(m_loop->m_loop_var);
//	if (op) op->AddRef();
//}

void CComboStatement::SetAssignOp(IAtomOperate * op)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == m_assignee);		// 只能被设置一次。
	JCASSERT(op);

	m_assignee = op;
	m_assignee->AddRef();
}

bool CComboStatement::GetResult(jcparam::IValue * & result)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == result);
	result = m_result;
	if (result) result->AddRef();
	return true;
}

bool CComboStatement::Invoke(void)
{
	LOG_STACK_TRACE();
	InvokeOpList(m_prepro_operates);
	return true;
}

bool CComboStatement::Merge(CComboStatement * combo, IAtomOperate *& last)
{
	JCASSERT(combo);
	JCASSERT(NULL == last);
	// 目前考虑Merge仅在Table是被调用，loop的第一个feature一定是loop var
	if (m_loop->m_loop_var) return false;

	m_prepro_operates.splice(m_prepro_operates.end(), combo->m_prepro_operates);

	last = combo->m_loop->m_loop_operates.back();
	JCASSERT(last);
	last->AddRef();

	IAtomOperate * first = combo->m_loop->m_loop_operates.front();
	combo->m_loop->m_loop_operates.pop_front();
	ILoopOperate * loop = dynamic_cast<ILoopOperate*>(first);
	JCASSERT(loop);
	m_loop->SetLoopVar(loop);
	first->Release(), first=NULL;
	
	m_loop->m_loop_operates.splice(m_loop->m_loop_operates.end(), combo->m_loop->m_loop_operates);
	return true;
}


void CComboStatement::CompileClose(void)
{
	// 如果不需要循环，则将m_loop展开到m_perpro_operates;
	if ( NULL == m_loop->m_loop_var )
	{
		if (m_assignee)		
		{
			m_loop->AddLoopOp(m_assignee);
			if (m_last_chain) m_assignee->SetSource(0, m_last_chain);
			m_last_chain = m_assignee;
		}
		m_prepro_operates.splice(
			m_prepro_operates.end(), 
			m_loop->m_loop_operates);
		m_loop->Release();
		m_loop = NULL;
	}
	else
	{	// 否则将m_loop添加到m_perpro_operates
		m_prepro_operates.push_back(m_loop);
		m_loop->AddRef();

		if (m_assignee)
		{	// 自动添加collect op用于叠加处理循环的结果
			IAtomOperate * op = static_cast<IAtomOperate*>(new CCollectOp);
			m_loop->AddLoopOp(op);
			op->SetSource(0, m_last_chain);
			m_last_chain = op;
			m_assignee->SetSource(0, op);
			m_prepro_operates.push_back(m_assignee);
			m_assignee = NULL;
			op->Release();
		}
	}
	m_closed = true;
}


#ifdef _DEBUG
void CComboStatement::DebugOutput(LPCTSTR indentation, FILE * outfile)
{
	stdext::jc_fprintf(outfile, indentation);
	stdext::jc_fprintf(outfile, _T("combo_pre_process, line=%d\n"), m_line );


	OP_LIST::iterator it = m_prepro_operates.begin(), endit = m_prepro_operates.end();
	for (; it != endit; ++it)
	{
		IAtomOperate * op = *it;
		JCASSERT(op);
		op->DebugOutput(indentation-1, outfile);
	}	
	if (m_assignee)
	{
		stdext::jc_fprintf(outfile, indentation);
		stdext::jc_fprintf(outfile, _T("combo_assign\n") );
		m_assignee->DebugOutput(indentation-1, outfile);
	}
	stdext::jc_fprintf(outfile, indentation);
	stdext::jc_fprintf(outfile, _T("combo_end\n") );
}

#endif
*/

///////////////////////////////////////////////////////////////////////////////
//--CLoopOp
/*
LOG_CLASS_SIZE(CLoopOp);

CLoopOp::CLoopOp(void)
	: m_loop_var(NULL)
{
}

CLoopOp::~CLoopOp(void)
{
	if (m_loop_var)	m_loop_var->Release();
	DeleteOpList(m_loop_operates);
}

bool CLoopOp::GetResult(jcparam::IValue * & val)
{
	return false;
}

bool CLoopOp::Invoke(void)
{
	JCASSERT(m_loop_var);

	// 初始化循环变量
	m_loop_var->Init();

	while (1)
	{
		// 执行循环变量
		if ( !m_loop_var->InvokeOnce() ) break;
		// 执行循环体
		InvokeOpList(m_loop_operates);
	}
	return true;
}

void CLoopOp::AddLoopOp(IAtomOperate * op)
{
	LOG_STACK_TRACE();
	JCASSERT(op);

	m_loop_operates.push_back(op);
	op->AddRef();
}

void CLoopOp::SetLoopVar(ILoopOperate * expr_op)
{
	LOG_STACK_TRACE();
	JCASSERT(expr_op);
	JCASSERT(NULL == m_loop_var); //不可重复设置循环变量

	m_loop_var = expr_op;
	m_loop_var->AddRef();

	//if (is_func) m_pre_func = static_cast<IAtomOperate*>(m_loop_var);
}

#ifdef _DEBUG
void CLoopOp::DebugOutput(LPCTSTR indentation, FILE * outfile)
{
	OP_LIST::iterator it, endit;
	if (m_loop_var)
	{
		stdext::jc_fprintf(outfile, indentation);
		stdext::jc_fprintf(outfile, _T("loop_var\n") );
		//m_loop_var->DebugOutput(indentation-1, outfile);

		stdext::jc_fprintf(outfile, indentation);
		stdext::jc_fprintf(outfile, _T("loop_body\n") );

		it = m_loop_operates.begin(), endit = m_loop_operates.end();
		for (; it != endit; ++it)
		{
			IAtomOperate * op = *it;
			JCASSERT(op);
			op->DebugOutput(indentation-1, outfile);
		}
		stdext::jc_fprintf(outfile, indentation);
		stdext::jc_fprintf(outfile, _T("loop_end\n") );	
	}
}

#endif
*/


///////////////////////////////////////////////////////////////////////////////
//--

void jcscript::DeleteOpList(OP_LIST & op_list)
{
	OP_LIST::iterator it = op_list.begin(), endit = op_list.end();
	for (; it != endit; ++it)
	{
		IAtomOperate * ao = *it;
		JCASSERT(ao);
		ao->Release();
	}
}

void jcscript::InvokeOpList(OP_LIST & op_list)
{
	OP_LIST::iterator it = op_list.begin(), endit = op_list.end();
	for (; it != endit; ++it)
	{
		IAtomOperate * op = *it;
		JCASSERT(op);
		op->Invoke();
	}
}


