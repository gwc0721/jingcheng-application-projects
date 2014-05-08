#include "stdafx.h"
#include "atom_operates.h"

LOCAL_LOGGER_ENABLE(_T("script.operates"), LOGGER_LEVEL_WARNING);

using namespace jcscript;
/*
///////////////////////////////////////////////////////////////////////////////
LOG_CLASS_SIZE(CHelpProxy)

void CHelpProxy::Create(IHelpMessage * help, CHelpProxy * & proxy)
{
	JCASSERT(NULL == proxy);
	proxy = new CHelpProxy(help);
}

CHelpProxy::CHelpProxy(IHelpMessage * help)
{
	m_help = help;
	if (m_help) m_help->AddRef();
}

CHelpProxy::~CHelpProxy(void)
{
	if (m_help) m_help->Release();
}

bool CHelpProxy::Invoke()
{
	LOG_STACK_TRACE();
	if (m_help)
	{
		m_help->HelpMessage(stdout);
	}
	return true;
}
*/

//const TCHAR CHelpProxy::name[] = _T("help");