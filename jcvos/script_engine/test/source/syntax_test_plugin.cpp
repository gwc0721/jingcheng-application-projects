#include "stdafx.h"

#include <stdext.h>

#include "./syntax_test_plugin.h"
#include "op_bus_doctor.h"

LOCAL_LOGGER_ENABLE(_T("syntax_test_plugin"), LOGGER_LEVEL_DEBUGINFO);

static CPluginSyntest g_dummy_plugin;

extern FILE * outfile;

///////////////////////////////////////////////////////////////////////////////
//-- CPorxySyntest
LOG_CLASS_SIZE(CProxySyntest)
CProxySyntest::CProxySyntest(const CJCStringT & name)
	: m_source(NULL)
	, m_name(name)
	, m_count(100)
{
}

CProxySyntest::~CProxySyntest(void)
{
	if (m_source) m_source->Release();
}

bool CProxySyntest::Invoke(void)
{
	LOG_STACK_TRACE();
	LOG_DEBUG(_T("{+} proxy is invoked"));
	return true;
}

bool CProxySyntest::PushParameter(const CJCStringT & var_name, jcparam::IValue * val)
{
	LOG_STACK_TRACE();
	LOG_DEBUG(_T("{+} push parameter %s"), var_name.c_str());
	return true;
}

bool CProxySyntest::CheckAbbrev(TCHAR param_name, CJCStringT & var_name) const
{
	LOG_STACK_TRACE();
	var_name = _T("abbre_");
	var_name += param_name;
	return true;
}

void CProxySyntest::SetSource(UINT src_id, IAtomOperate * op)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == m_source);

	m_source = op;
	m_source->AddRef();
}

bool CProxySyntest::GetResult(jcparam::IValue * & result) 
{
	return true;
}

#ifdef _DEBUG
void CProxySyntest::DebugOutput(LPCTSTR indentation, FILE * outfile)
{
	stdext::jc_fprintf(outfile, indentation);
	stdext::jc_fprintf(outfile, _T("%s, [%08X], <%08X>\n"),
		m_name.c_str(),
		(UINT)(static_cast<IAtomOperate*>(this)), (UINT)(m_source) );
}
#endif

///////////////////////////////////////////////////////////////////////////////
//-- CPluginSyntest
LOG_CLASS_SIZE(CPluginSyntest)

bool CPluginSyntest::Reset(void)
{
	LOG_STACK_TRACE();
	return true;
}

void CPluginSyntest::GetFeature(const CJCStringT & cmd_name, IFeature * & pr)
{
	LOG_STACK_TRACE();
	LOG_DEBUG(_T("{*} function name = %s"), cmd_name.c_str());
	//_ftprintf(outfile, _T("GET PROXY: %s\n"), cmd_name.c_str());
	pr = static_cast<IFeature *>(new CProxySyntest(cmd_name) );
	//pr->AddRef();
}

void CPluginSyntest::ShowFunctionList(FILE * output) const
{
	LOG_STACK_TRACE();
}

LPCTSTR CPluginSyntest::name() const
{
	return _T("dummy proxy");
}

void CPluginSyntest::Release()
{
	LOG_SIMPLE_TRACE(_T(""));
	__super::Release();
}


///////////////////////////////////////////////////////////////////////////////
//-- CContainerSyntest
LOG_CLASS_SIZE(CContainerSyntest)

CContainerSyntest::CContainerSyntest(void)
	: m_var_op(NULL)
	, m_variable(NULL)
{
	LOG_STACK_TRACE();
}

CContainerSyntest::~CContainerSyntest(void)
{
	LOG_STACK_TRACE();
	if (m_var_op) m_var_op->Release();
	if (m_variable) m_variable->Release();
}

void CContainerSyntest::Delete(void)
{
	LOG_STACK_TRACE();
	if (m_var_op) 
	{
		m_var_op->Release();
		m_var_op = NULL;
	}

	if (m_variable)
	{
		m_variable->Release();
		m_variable = NULL;
	}
}

bool CContainerSyntest::GetPlugin(const CJCStringT & name, IPlugin * & plugin)
{
	LOG_STACK_TRACE();
	LOG_DEBUG(_T("{*} module name = %s"), name.c_str()); 
	//_ftprintf(outfile, _T("GET PLUGIN: %s\n"), name.c_str());
	plugin = static_cast<IPlugin *>(&g_dummy_plugin);
	return true;
}

void CContainerSyntest::GetVarOp(IAtomOperate * & op)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == op);

	if (!m_var_op)
	{
		JCASSERT(NULL == m_variable);
		m_variable = new CValueSyntest;
		//m_var_op = new CConstantOp(m_variable);
		CSyntaxParser::CreateConstantOp(m_variable, m_var_op);
	}

	LOG_DEBUG(_T("var_op=0x%08X"), (UINT)(m_var_op));
	JCASSERT(m_var_op);
	op = static_cast<IAtomOperate *>(m_var_op);
	op->AddRef();
}

bool CContainerSyntest::ReadFileOp(LPCTSTR type, const CJCStringT & filename, IAtomOperate *& op)
{
	LOG_STACK_TRACE();
	LOG_DEBUG(_T("create file op: type=%s, fn=%s"), type, filename.c_str());
	//jcparam::IValue * fn = static_cast<jcparam::IValue*>(
	//	new CValueSyntest);
	//CSyntaxParser::CreateConstantOp(fn, op);
	//fn->Release();

	CBusDoctorOp * bd_op = new CBusDoctorOp(filename);
	op = static_cast<IAtomOperate*>(bd_op);
	
	return true;
}



///////////////////////////////////////////////////////////////////////////////
//-- CRowSyntest
LOG_CLASS_SIZE(CRowSyntest)
CRowSyntest::CRowSyntest(CValueSyntest * tab)
	: m_table(tab)
{
}

void CRowSyntest::GetColumnData(int field, jcparam::IValue * & val)	const
{
	JCASSERT(field < 5);
	JCASSERT(NULL == val);
	const jcparam::IValue * _val = static_cast<const jcparam::IValue*>(this);
	val = const_cast<jcparam::IValue *>(_val);
	val->AddRef();
}

//void CRowSyntest::GetColumnData(LPCTSTR field_name, jcparam::IValue * &val) const
//{
//	JCASSERT(NULL == val);
//	const jcparam::IValue * _val = static_cast<const jcparam::IValue*>(this);
//	val = const_cast<jcparam::IValue *>(_val);
//	val->AddRef();
//}

void CRowSyntest::AddRef(void)
{
	LOG_STACK_TRACE();
	__super::AddRef();
	LOG_DEBUG(_T("ref=%d"), m_ref);
}

void CRowSyntest::Release(void)
{
	LOG_STACK_TRACE();
	LOG_DEBUG(_T("ref=%d"), m_ref);
	__super::Release();
}

bool CRowSyntest::CreateTable(jcparam::ITable * & tab)
{
	tab = static_cast<CValueSyntest*>(m_table);
	tab->AddRef();
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//-- CValueSyntest
LOG_CLASS_SIZE(CValueSyntest)

CValueSyntest::CValueSyntest(void)
{
	m_row = new CRowSyntest(this);
}

CValueSyntest::~CValueSyntest(void)
{
	m_row->Release();
}

void CValueSyntest::GetSubValue(LPCTSTR name, jcparam::IValue * & val)
{
	LOG_SIMPLE_TRACE(_T("{+} variable = %s"), name);
	_ftprintf(outfile, _T("GET SUB VAR: %s\n"), name);
	val = static_cast<jcparam::IValue *>(this);
	val->AddRef();
}

void CValueSyntest::AddRef(void)
{
	LOG_STACK_TRACE();
	__super::AddRef();
	LOG_DEBUG(_T("ref=%d"), m_ref);
}

void CValueSyntest::Release(void)
{
	LOG_STACK_TRACE();
	LOG_DEBUG(_T("ref=%d"), m_ref);
	__super::Release();
}

void CValueSyntest::GetRow(JCSIZE index, IValue * & row)
{ 
	JCASSERT(index < 10);
	JCASSERT(NULL == row);
	m_row->AddRef(); 
	row = static_cast<IValue*>(m_row);
}