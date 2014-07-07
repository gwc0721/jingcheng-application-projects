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

LOCAL_LOGGER_ENABLE(_T("script.operates"), LOGGER_LEVEL_WARNING);

using namespace jcscript;

///////////////////////////////////////////////////////////////////////////////
//-- CLoopVarOp
LOG_CLASS_SIZE(CLoopVarOp)

const TCHAR CLoopVarOp::m_name[] = _T("table");


CLoopVarOp::CLoopVarOp(void)
	: m_table(NULL)
	, m_table_size(0)
	, m_cur_index(0)
{
	LOG_STACK_TRACE();
}

CLoopVarOp::~CLoopVarOp(void)
{
	LOG_STACK_TRACE();
	if (m_table)	m_table->Release();
}

bool CLoopVarOp::GetResult(jcparam::IValue * & val)
{
	return false;
}

bool CLoopVarOp::Invoke(void)
{
	LOG_STACK_TRACE();
	JCASSERT(m_outport);

	if (!m_table) Init();

	
	if (m_cur_index >= m_table_size) return false;	//返回false表示循环结束
	LOG_SCRIPT(_T("push %d / %d"), m_cur_index, m_table_size );
	jcparam::IValue * row = NULL;
	m_table->GetRow(m_cur_index, row);
	JCASSERT(row);
	m_outport->PushResult(row);
	row->Release();
	m_cur_index ++;
	return (m_cur_index < m_table_size);
}

void CLoopVarOp::GetProgress(JCSIZE &cur_prog, JCSIZE &total_prog) const
{
	cur_prog = m_cur_index;
	total_prog = m_table_size;
}

void CLoopVarOp::Init(void)
{
	LOG_STACK_TRACE();
	JCASSERT(m_src[0]);

	// 第一次运行时获取表格
	JCASSERT(NULL == m_table);		// 不能重复调用
	jcparam::IValue * val = NULL;
	m_src[0]->GetResult(val);
	m_table = dynamic_cast<jcparam::IVector*>(val);
	if (!m_table) 
	{
		if (val) val->Release();
		THROW_ERROR(ERR_PARAMETER, _T("loop variable must be a table."));
	}
	m_table_size = m_table->GetRowSize();
	m_cur_index = 0;
	LOG_SCRIPT(_T("count=%d"), m_table_size );
}

void CLoopVarOp::DebugInfo(FILE * outfile)
{
}

///////////////////////////////////////////////////////////////////////////////
//-- CCollectOp
LOG_CLASS_SIZE(CCollectOp)
const TCHAR CCollectOp::m_name[] = _T("collector");

CCollectOp::CCollectOp(void)
	: m_table(NULL)
{
	//strcpy(_class_name, "CollOp");
}

CCollectOp::~CCollectOp(void)
{
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
	LOG_SCRIPT(_T(""));
	JCASSERT(m_src[0]);

	jcparam::IValue * pre_out = NULL;
	m_src[0]->PopupResult(pre_out);

	if (pre_out)
	{
		if ( !m_table )
		{
			jcparam::ITableRow * row = NULL;
			if ( row = dynamic_cast<jcparam::ITableRow*>(pre_out) )
			{
				jcparam::ITable * table = NULL;
				row->CreateTable(table);
				m_table = static_cast<jcparam::IVector*>(table);
			}
			else
			{
				m_table = static_cast<jcparam::IVector*>(new jcparam::CVector);
			}
		}
		JCASSERT(m_table);
		m_table->PushBack(pre_out);
		pre_out->Release();
	}
	// 对于任何输入，只需执行一次
	return false;
}

void CCollectOp::DebugInfo(FILE * outfile)
{

}

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


