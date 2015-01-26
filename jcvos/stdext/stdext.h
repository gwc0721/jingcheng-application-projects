#pragma once

//#define _COOLCTRL_EXT_CLASS 	__declspec(dllimport)

#include "include/comm_define.h"
#include "include/jclogger_base.h"
#include "include/jcexception.h"
#include "include/autohandle.h"
#include "include/jcinterface.h"
#include "include/jchelptool.h"

#ifdef _DEBUG
#pragma comment (lib, "lib/stdextD.lib")
#else
#pragma comment (lib, "lib/stdext.lib")
#endif
