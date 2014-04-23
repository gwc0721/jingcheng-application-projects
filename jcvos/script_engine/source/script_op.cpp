#include "stdafx.h"
#include "atom_operates.h"
#include "flow_ctrl_op.h"

LOCAL_LOGGER_ENABLE(_T("script.operates"), LOGGER_LEVEL_WARNING);

using namespace jcscript;

///////////////////////////////////////////////////////////////////////////////
//-- CScriptOp
LOG_CLASS_SIZE(CScriptOp)

CScriptOp::CScriptOp(CSequenceOp * super)
	: CSequenceOp(super)
{
	LOG_STACK_TRACE();
}

CScriptOp::~CScriptOp(void)
{
	LOG_STACK_TRACE();
}

//bool CScriptOp::GetResult(jcparam::IValue * & val) 
//{
//	LOG_STACK_TRACE();
//	return true;
//}

bool CScriptOp::Invoke()
{
	LOG_STACK_TRACE();
	InvokeOpList();
	return true;
}

void CScriptOp::DebugOutput(LPCTSTR indentation, FILE * outfile)
{
	stdext::jc_fprintf(outfile, _T("%sprogram [%08X], res=<%08X>\n"), 
		indentation, (UINT)(static_cast<IAtomOperate*>(this)),
		(UINT)(static_cast<IAtomOperate*>(m_result_op)) );
	__super::DebugOutput(indentation-1, outfile);
	stdext::jc_fprintf(outfile, _T("%sprogram_end\n"), indentation);
}
