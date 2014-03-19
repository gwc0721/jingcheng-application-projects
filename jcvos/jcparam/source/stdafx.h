#pragma once

#ifndef _WIN32_WINNT		//                
#define _WIN32_WINNT 0x0501	//
#endif		

#define LOGGER_LEVEL LOGGER_LEVEL_DEBUGINFO

#ifdef _DEBUG
#define LOG_OUT_CLASS_SIZE
#endif

#include <stdio.h>
#include <tchar.h>