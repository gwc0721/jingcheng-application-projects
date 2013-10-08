#include "stdafx.h"
#include "atom_operates.h"

LOCAL_LOGGER_ENABLE(_T("CScriptOp"), LOGGER_LEVEL_WARNING);

using namespace jcscript;

///////////////////////////////////////////////////////////////////////////////
//-- CScriptOp
LOG_CLASS_SIZE(CScriptOp)

CScriptOp::CScriptOp(void)
{
	LOG_STACK_TRACE();
}

CScriptOp::~CScriptOp(void)
{
	LOG_STACK_TRACE();
	DeleteOpList(m_op_list);
}

void CScriptOp::Create(CScriptOp *&program)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == program);
	program = new CScriptOp;
}

bool CScriptOp::GetResult(jcparam::IValue * & val) 
{
	LOG_STACK_TRACE();
	return true;
}

void CScriptOp::PushBackAo(IAtomOperate * op)
{
	LOG_STACK_TRACE();
	JCASSERT(op);
	op->AddRef();
	m_op_list.push_back(op);
}

bool CScriptOp::Invoke()
{
	LOG_STACK_TRACE();
	InvokeOpList(m_op_list);
	return true;
}

void CScriptOp::Merge(CComboStatement * combo)
{
	LOG_STACK_TRACE();
	JCASSERT(combo);

	m_op_list.splice(m_op_list.end(), combo->m_prepro_operates);
}


#ifdef _DEBUG
void CScriptOp::DebugOutput(LPCTSTR indentation, FILE * outfile)
{
	stdext::jc_fprintf(outfile, indentation);
	stdext::jc_fprintf(outfile, _T("program_begin\n") );


	OP_LIST::iterator it = m_op_list.begin(), endit = m_op_list.end();
	for (; it != endit; ++it)
	{
		IAtomOperate * op = *it;
		JCASSERT(op);
		op->DebugOutput(indentation-1, outfile);
	}	

	stdext::jc_fprintf(outfile, indentation);
	stdext::jc_fprintf(outfile, _T("program_end\n") );
}

#endif