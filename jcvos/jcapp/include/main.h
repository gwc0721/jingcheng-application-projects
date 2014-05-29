#pragma once

#include "../stdext/stdext.h"
#include "include/application.h"

#define JCAPP_MAIN_SUPPORT
#define JCAPP_LOGGER_SUPPORT
#define JCAPP_PARAM_SUPPORT

#ifdef JCAPP_LOGGER_SUPPORT
#endif	//JCAPP_LOGGER_SUPPORT

#ifdef JCAPP_MAIN_SUPPORT
int _tmain(int argc, _TCHAR* argv[])
{
	int return_code = 0;
	CJCApp * app = CJCApp::GetApp<CJCApp>();
	JCASSERT(app);
	try
	{
#ifdef JCAPP_LOGGER_SUPPORT
		FILE * config_file = NULL;
		_tfopen_s(&config_file, _T("jclog.cfg"), _T("r"));
		if (config_file)
		{
			CJCLogger::Instance()->Configurate(config_file);
			fclose(config_file);
		}
#endif	//JCAPP_LOGGER_SUPPORT
		app->Initialize( GetCommandLine() );
		app->Run();
	}
	catch (stdext::CJCException & err)
	{
		stdext::jc_printf(_T("Error! %s\n"), err.WhatT());
		stdext::jc_printf(_T("press any key to exit."));
		//getchar();
		return err.GetErrorID();
	}
	catch (std::exception & err)
	{
		printf("Error! %s\n", err.what());
		printf("press any key to exit.");
		getchar();
		return 1;
	}
	app->CleanUp();

	return return_code;
}
	
#endif	//JCAPP_MAIN_SUPPORT