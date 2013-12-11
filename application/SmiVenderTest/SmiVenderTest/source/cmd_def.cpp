#include "stdafx.h"

#include <stdext.h>

#include "cmd_def.h"

LOCAL_LOGGER_ENABLE(_T("CmdDef"), LOGGER_LEVEL_DEBUGINFO);

///////////////////////////////////////////////////////////////////////////////
//-- CProxyBase
CProxyBase::CProxyBase(const CCmdDefBase * def) 
	: m_def(def)
	, m_inout(NULL) 
	, m_src_op(NULL)
{
}
	
CProxyBase::~CProxyBase(void) 
{
	if (m_inout)		m_inout->Release();
	if (m_src_op)	m_src_op->Release();
}

//void CProxyBase::SetInput(jcparam::IValue * input)
//{
//	if (m_variable) m_inout->Release();
//	m_inout = input;
//	if (m_inout) m_inout->AddRef();
//}

void CProxyBase::SetSource(UINT src_id, jcscript::IAtomOperate * op)
{
	if (op)
	{
		m_src_op = op;
		m_src_op->AddRef();
	}
}

bool CProxyBase::GetResult(jcparam::IValue * & result)
{
	JCASSERT(NULL == result);
	result = m_inout;
	if (result) result->AddRef();
	return true;
}

bool CProxyBase::PushParameter(const CJCStringT & var_name, jcparam::IValue * val)
{
	m_argus.AddValue(var_name, val);
	return true;
}

bool CProxyBase::CheckAbbrev(TCHAR abbrev, CJCStringT & param_name) const
{
	JCASSERT(m_def);
	const jcparam::CParameterDefinition & param_def = m_def->GetParamDefine();
	const jcparam::CArguDesc * argu_desc = param_def.GetParamDef(abbrev);
	if (! argu_desc) return false;
	param_name = argu_desc->mName;
	return true;
}

void CProxyBase::HelpMessage(FILE * output)
{
	JCASSERT(m_def);
	LPCTSTR desc = m_def->Description();
	if (!desc) desc = _T("");
	_ftprintf(output, _T("%s : %s\n"), 
		m_def->GetCommandName().c_str(), desc);

	m_def->GetParamDefine().OutputHelpString(output);
}

//void CProxyBase::AddParameter(const CJCStringT & param_name, jcparam::IValue * value)
//{
//	m_argus.AddValue(param_name, value);
//}


//bool CProxyBase::CheckParameterName(const CJCStringT & param_name) const
//{
//	JCASSERT(m_def);
//	const jcparam::CParameterDefinition & param_def = m_def->GetParamDefine();
//	return param_def.CheckParameterName(param_name);
//}

//bool CProxyBase::GetParameterName(TCHAR abbrev, CJCStringT & param_name) const
//{
//	JCASSERT(m_def);
//	const jcparam::CParameterDefinition & param_def = m_def->GetParamDefine();
//	const jcparam::CArguDesc * argu_desc = param_def.GetParamDef(abbrev);
//	if (! argu_desc) return false;
//	param_name = argu_desc->mName;
//	return true;
//}


#ifdef _DEBUG
void CProxyBase::DebugOutput(LPCTSTR indentation, FILE * outfile)
{
	//stdext::jc_fprintf(outfile, indentation);
	//stdext::jc_fprintf(outfile, _T("%s, [%08X], <%08X>\n"),
	//	m_def->name(), 
	//	(UINT)(static_cast<jcscript::IAtomOperate*>(this)), 
	//	(UINT)(m_src_op) );
}
#endif