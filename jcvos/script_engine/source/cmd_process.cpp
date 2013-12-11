#include "stdafx.h"
#include "atom_operates.h"

LOCAL_LOGGER_ENABLE(_T("CHelpProxy"), LOGGER_LEVEL_WARNING);

using namespace jcscript;

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
	//if (m_plugin)	m_plugin->Release();
	//if (m_proxy)	m_proxy->Release();
}

//void CHelpProxy::SetProxy(IProxy * proxy)
//{
//	JCASSERT(NULL == m_proxy);
//	JCASSERT(proxy);
//	m_proxy = proxy;
//	if (m_proxy) m_proxy->AddRef();
//}
//	
//void CHelpProxy::SetPlugin(IPlugin * plugin)
//{
//	JCASSERT(NULL == m_plugin);
//	JCASSERT(plugin);
//	m_plugin = plugin;
//	if (m_plugin) m_plugin->AddRef();
//}

bool CHelpProxy::Invoke()
{
	LOG_STACK_TRACE();
	if (m_help)
	{
		m_help->HelpMessage(stdout);
	}
	//if (m_proxy)
	//{
	//	// show help for this function.
	//	m_proxy->HelpMessage(stdout);
	//}
	//else if (m_plugin)
	//{
	//	// list out all functions in this plugin
	//	_tprintf(_T("List out functions for module %s.\n"), m_plugin->name() );
	//	m_plugin->ShowFunctionList(stdout);
	//}
	//else
	//{
	//	// show help for syntax
	//}
	return true;
}
