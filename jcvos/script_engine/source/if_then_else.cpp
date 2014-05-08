#include "stdafx.h"
#include "flow_ctrl_op.h"

LOCAL_LOGGER_ENABLE(_T("script.operates"), LOGGER_LEVEL_WARNING);
using namespace jcscript;

///////////////////////////////////////////////////////////////////////////////
// -- 一个操作序列
LOG_CLASS_SIZE(CSequenceOp)
LOG_CLASS_SIZE_T(CSequenceOp::OP_LIST, 3)

CSequenceOp::CSequenceOp(CSequenceOp * super)
	: m_super_seq(super)
	, m_dependency(0)
	, m_result_op(NULL)
	, m_ref(1)
{
	// m_super_seq不能进行引用计数，引用计数会死锁
	if (m_super_seq)		m_dependency = m_super_seq->GetDependency() +1;
}

CSequenceOp::~CSequenceOp(void)
{
	OP_LIST::iterator it = m_op_list.begin(), endit = m_op_list.end();
	for (; it != endit; ++it)
	{
		IAtomOperate * ao = *it;
		JCASSERT(ao);
		ao->Release();
	}
}

void CSequenceOp::AddOp(IAtomOperate * op)
{
	LOG_STACK_TRACE();
	JCASSERT(op);

	m_op_list.push_back(op);
	op->AddRef();
	m_result_op = op;
}

bool CSequenceOp::InvokeOpList(void)
{
	bool running = true;
	OP_LIST::iterator it = m_op_list.begin(), endit = m_op_list.end();
	for (; it != endit; ++it)
	{
		IAtomOperate * op = *it;
		JCASSERT(op);
		if ( !op->Invoke() ) 
		{
			running = false;
			break;
		}
	}
	return running;
}

CSequenceOp * CSequenceOp::GetSuper(void)
{
	return m_super_seq;
}

bool CSequenceOp::GetResult(jcparam::IValue * & val) 
{
	if (m_result_op) return m_result_op->GetResult(val);
	else return false; 
}


void CSequenceOp::DebugOutput(LPCTSTR indentation, FILE * outfile)
{
	OP_LIST::iterator it = m_op_list.begin(), endit = m_op_list.end();
	for (; it != endit; ++it)
	{
		IAtomOperate * op = *it;
		JCASSERT(op);
		op->DebugOutput(indentation, outfile);
	}	
}

///////////////////////////////////////////////////////////////////////////////
// -- combo
LOG_CLASS_SIZE(CComboSt)

const TCHAR CComboSt::m_name[] = _T("combo_st");

CComboSt::CComboSt(CSequenceOp * super)
	: CSequenceOp(super)
{
}

CComboSt::~CComboSt(void)
{
}

void CComboSt::AddOp(IAtomOperate * op)
{
	JCASSERT(op);
	JCASSERT(m_super_seq);
	// 判断是否有依赖于循环变量，如果不依赖，则放入上一层中
	UINT prop = op->GetProperty();
	if ( prop & OPP_LOOP_SOURCE )
	{
		m_op_list.push_back(op);
		op->AddRef();
	}
	else if (op->GetDependency() < m_dependency) 
	{
		m_super_seq->AddOp(op);
	}
	else
	{
		m_op_list.push_back(op);
		op->AddRef();
	}
	m_result_op = op;
}

bool CComboSt::Invoke(void)
{
	LOG_SCRIPT(_T("") );

	while (1)
	{
		bool running = false;
		OP_LIST::iterator it = m_op_list.begin(), endit = m_op_list.end();
		for (; it != endit; ++it)
		{
			IAtomOperate * op = *it;
			JCASSERT(op);
			// single_st返回false表示其inport为空。当所有的single_st都返回false后，停止运行。
			bool br = op->Invoke();
			running |= br;
		}
		if (!running) break;
	}
	return true;
}

void CComboSt::DebugOutput(LPCTSTR indentation, FILE * outfile)
{
	stdext::jc_fprintf(outfile, _T("%s%s [%08X], line=%d, res=<%08X>\n"), 
		indentation, m_name, (UINT)(static_cast<IAtomOperate*>(this)), m_line, 
		(UINT)(static_cast<IAtomOperate*>(m_result_op)) );

	__super::DebugOutput(indentation-1, outfile);
	stdext::jc_fprintf(outfile, _T("%s%s end\n") , indentation, m_name);

}

///////////////////////////////////////////////////////////////////////////////
// -- if-then-else
//LOG_CLASS_SIZE(CStIf) 
//const TCHAR CStIf::m_name[] = _T("if");
//
//CStIf::CStIf(void)
//	 : m_true_seq(NULL), m_false_seq(NULL)
//{
//}
//
//CStIf::~CStIf(void)
//{
//	if (m_true_seq)		m_true_seq->Release();
//	if (m_false_seq)	m_false_seq->Release();
//}
//
//bool CStIf::Invoke(void)
//{
//	bool * r = (bool*)(m_src[0]->GetValue() );
//	if ( (*r) && m_true_seq)	m_true_seq->Invoke();
//	else if (m_false_seq)		m_false_seq->Invoke();
//	return true;
//}
//
//CSequenceOp * CStIf::GetSubSequenc(bool id)
//{
//	return NULL;
//}

///////////////////////////////////////////////////////////////////////////////
// single statement
LOG_CLASS_SIZE(CSingleSt)
LOG_CLASS_SIZE_T(CSingleSt::ROW_LIST, 2)

const TCHAR CSingleSt::m_name[] = _T("single_st");

CSingleSt::CSingleSt(CSequenceOp * super)
	: CSequenceOp(super)
	, m_inport(NULL)
	, m_property(0)
	, m_state_op(NULL)
	, m_state_op_run_more(false)
{
}

CSingleSt::~CSingleSt(void)
{
	// m_inport is managed by op list
	if (m_state_op)	m_state_op->Release();
}

void CSingleSt::AddOp(IAtomOperate * op)
{
	JCASSERT(op);

	if ( op->GetProperty() & OOP_STATE )
	{
		JCASSERT(NULL == m_state_op);
		m_state_op = op;
		m_state_op->AddRef();
	}
	else if (op->GetDependency() < m_dependency)
	{
		m_super_seq->AddOp(op);
	}
	else
	{
		m_op_list.push_back(op);
		op->AddRef();
	}
}


void CSingleSt::SetSource(UINT src_id, IAtomOperate * src)
{
	// 1, confirm current source
	if (m_inport) THROW_ERROR(ERR_USER, _T("source has already been set") );
	// 2, create inoprt,
	m_inport = new CInPort;
	// 3, set inport"s sorce to source
	m_inport->SetSource(0, src);
	// 4, update dependency
	m_dependency = m_inport->GetDependency();
	m_op_list.push_back(m_inport);
}


bool CSingleSt::Invoke(void)
{
	LOG_SCRIPT(_T("run_more=%d"), m_state_op_run_more );
	// 考虑到输出量>输入量的情况下，后续命令应该优先处理。
	// 处理原则：(1) 包含一个或0个inport，用于连接前一个single statement
	//			 (2) 必须包含一个filter(状态op)，且位于single statement的最后
	//			 (3) 包含一个或多个非状态op，位于inport和状态op之间。
	//			 (4) filter的invoke函数被设计成可重入的，
	//			 (5) 如果invoke返回true，则表示还需要继续来处理当前的inport，下次执行时以NULL作为inport
	//			 (6) 如果invoke返回false则表示不需要继续处理。下次执行时，以下一个inport作为输入
	if (!m_state_op_run_more)
	{
		if (! InvokeOpList() )
		{
			LOG_SCRIPT_OUT(_T("failure on non-state op"));
			return false;
		}
	}	
	while (1)
	{
		JCASSERT(m_state_op);
		if ( ! m_outport.empty() )
		{
			LOG_SCRIPT_OUT(_T("outport not empty, more=%d"), m_state_op_run_more);
			return true;
		}
		m_state_op_run_more = m_state_op->Invoke();
		if (!m_state_op_run_more)
		{
			LOG_SCRIPT_OUT(_T("state op no more"));
			return false;
		}
	}
	return true;
}

bool CSingleSt::PopupResult(jcparam::IValue * & val)
{
	JCASSERT(NULL == val);
	if (m_outport.empty() ) return false;
	val = m_outport.front();
	m_outport.pop_front();
	return true;
}


bool CSingleSt::PushResult(jcparam::IValue * row)
{	
	JCASSERT(row);
	m_outport.push_back(row);
	row->AddRef();
	return false;
}

void CSingleSt::DebugOutput(LPCTSTR indentation, FILE * outfile)
{
	stdext::jc_fprintf(outfile, _T("%s%s [%08X]\n"), 
		indentation, m_name, (UINT)(static_cast<IAtomOperate*>(this)) );
	__super::DebugOutput(indentation-1, outfile);
	if (m_state_op)	m_state_op->DebugOutput(indentation-1, outfile);
	stdext::jc_fprintf(outfile, _T("%s%s end\n") , indentation, m_name);
}
