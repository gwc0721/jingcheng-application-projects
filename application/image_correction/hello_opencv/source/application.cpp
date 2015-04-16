#include "stdafx.h"

#include "application.h"
#include <vld.h>
#include <stdext.h>

LOCAL_LOGGER_ENABLE(_T("hello_opencv"), LOGGER_LEVEL_DEBUGINFO);

const TCHAR CHelloOpenCvApp::LOG_CONFIG_FN[] = _T("driver_test.cfg");
typedef jcapp::CJCApp<CHelloOpenCvApp>	CApplication;
#define _class_name_	CApplication
CApplication _app;

BEGIN_ARGU_DEF_TABLE()
	ARGU_DEF_ITEM(_T("dummy"),		_T('p'), CJCStringT,	m_dummy,		_T("load user mode driver.") )
	//ARGU_DEF_ITEM(_T("config"),		_T('g'), CJCStringT,	m_config,		_T("configuration file name for driver.") )
END_ARGU_DEF_TABLE()


///////////////////////////////////////////////////////////////////////////////
//--

CHelloOpenCvApp::CHelloOpenCvApp(void)
{
}

int CHelloOpenCvApp::Initialize(void)
{
	// loop
		// create process
		// add processor to list
	// end loop

	return 1;

}

int CHelloOpenCvApp::Run(void)
{
	// 
	return 0;
}

void CHelloOpenCvApp::CleanUp(void)
{
	//if (m_driver_module) FreeLibrary(m_driver_module);
}


int _tmain(int argc, _TCHAR* argv[])
{
	return jcapp::local_main(argc, argv);
}