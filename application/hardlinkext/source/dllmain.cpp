// dllmain.cpp : DllMain 的实现。

#include "stdafx.h"
#include "../resource.h"
#include "../hardlinkext_i.h"
#include "dllmain.h"

#include <stdext.h>
LOCAL_LOGGER_ENABLE(_T("hlchk"), LOGGER_LEVEL_ERROR);

ChardlinkextModule _AtlModule;

// DLL 入口点
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	TCHAR str[MAX_PATH];
	//wmemset(str, 0, MAX_PATH);
	//GetCurrentDirectory(MAX_PATH, str);
	//LOG_DEBUG(_T("current dir: '%s'"), str);

	wmemset(str, 0, MAX_PATH);
	GetModuleFileNameW(_AtlBaseModule.GetModuleInstance(), str, MAX_PATH);
	LPTSTR sfile = _tcsrchr(str, _T('\\') );
	if (sfile != 0) _tcscpy_s(sfile +1, (MAX_PATH - (sfile - str) -1), _T("jclog.cfg") );

	LOG_DEBUG( _T("loading config %s"), str )
	LOGGER_CONFIG(str);
	
	LOG_RELEASE(_T("starting..."));

	//LOG_STACK_TRACE();
	hInstance;
	return _AtlModule.DllMain(dwReason, lpReserved); 
}
