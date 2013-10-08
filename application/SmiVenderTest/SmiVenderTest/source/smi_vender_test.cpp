// SmiVenderTest.cpp :

#include "stdafx.h"
#include <vld.h>
#include <vector>

#include <stdext.h>

LOCAL_LOGGER_ENABLE(_T("SmiVenderTest"), LOGGER_LEVEL_DEBUGINFO);

#include <jcparam.h>
#include <SmiDevice.h>
#include "application.h"

static CSvtApplication the_app;

#include "plugin_test.h"
#include "plugin_device.h"
#include "plugin_ata_device.h"
#include "plugin_debug.h"
#include "plugin_isp.h"
#include "plugin_overprgm.h"
#include "plugin_performance.h"
#include "plugin_scsi_device.h"

#include "plugin_simulator.h"


bool RegistInternalPlugin(void)
{
	LOG_STACK_TRACE();
	CPluginManager * manager = CSvtApplication::GetApp()->GetPluginManager();
	JCASSERT(manager);
	CPluginTest::Regist(manager);
	CPluginDevice::Regist(manager);
	CPluginAtaDevice::Regist(manager);
	CPluginIsp::Regist(manager);
	CPluginOverPrgm::Regist(manager);
	CPluginPerformance::Regist(manager);
	CPluginScsiDevice::Regist(manager);

	CPluginSimulator::Regist(manager);

#ifdef _DEBUG
	CPluginDebug::Regist(manager);
#endif
	return true;
}

#include <stdlib.h>
//#include <time.h>
//
//LONGLONG GetThreadTotalTime(void)
//{
//	FILETIME tmp1, tmp2, kernel_time, user_time;
//	LARGE_INTEGER lkt, lut;
//	GetThreadTimes(GetCurrentThread(), &tmp1, &tmp2, &kernel_time, &user_time);
//	lkt.LowPart = kernel_time.dwLowDateTime, lkt.HighPart = kernel_time.dwHighDateTime;
//	lut.LowPart = user_time.dwLowDateTime, lut.HighPart = user_time.dwHighDateTime;
//	return (lkt.QuadPart + lut.QuadPart);
//}

#ifdef _DEBUG
//#if 0
LOGGER_TO_DEBUG(0, 0
		//| CJCLogger::COL_COMPNENT_NAME
		| CJCLogger::COL_TIME_STAMP
		| CJCLogger::COL_FUNCTION_NAME 
		//| CJCLogger::COL_REAL_TIME
		, 0);
#else
LOGGER_TO_FILE(0, _T("SmiVendorTest.log"), 
		//CJCLogger::COL_COMPNENT_NAME | 
		CJCLogger::COL_TIME_STAMP |
		CJCLogger::COL_FUNCTION_NAME | 
		CJCLogger::COL_REAL_TIME, 0);
#endif

void timestamptest(void)
{
	LOG_STACK_TRACE();
}

int _tmain(int argc, _TCHAR* argv[])
{
	LOG_STACK_TRACE();

	timestamptest();

	LOG_DEBUG(_T("T1"));
	LOG_DEBUG(_T("T2"));

	int return_code = 0;
	setvbuf(stdout, NULL, _IONBF, 0);
	stdext::jc_printf(_T("SMI Vender Test\n"));
	stdext::jc_printf(_T("Ver. 1.2\n"));
	stdext::jc_printf(_T("Jingcheng Yuan\n"));

	try
	{
		// Configurate jclogger
		FILE * config_file = NULL;
		_tfopen_s(&config_file, _T("jclog.cfg"), _T("r"));
		if (config_file)
		{
			CJCLogger::Instance()->Configurate(config_file);
			fclose(config_file);
		}

		RegistInternalPlugin();
		the_app.Initialize( GetCommandLine() );
		the_app.Run();
	}
	catch (std::exception & err)
	{
		stdext::CJCException * jcerr = dynamic_cast<stdext::CJCException *>(&err);
		if (jcerr)
		{
		}
		printf("Error! %s\n", err.what());
		getchar();
	}
	catch (jcscript::CExitException &)
	{
	}
	
	the_app.Cleanup();
	return return_code;
}

