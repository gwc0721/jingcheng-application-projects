#pragma once

#ifndef _WIN32_WINNT		//                
#define _WIN32_WINNT 0x0501	//
#endif		

#define LOGGER_LEVEL LOGGER_LEVEL_DEBUGINFO
#define LOG_OUT_CLASS_SIZE

#include <stdio.h>
#include <tchar.h>

// configurable defines

// 行缓存大小
#ifndef MAX_LINE_BUF
	#define MAX_LINE_BUF (1024)
#endif	//MAX_LINE_BUF

// 预处理脚本文件名
#ifndef DEFAULT_SCRIPT
	#define DEFAULT_SCRIPT (_T("preload.scp"));
#endif	//DEFAULT_SCRIPT

#ifndef SRC_READ_SIZE
	#define SRC_READ_SIZE	(4096)
	#define	SRC_BACK_SIZE	(1024)
#endif	//SRC_READ_SIZE

#ifndef SECTOR_SIZE
#define SECTOR_SIZE		(512)
#endif // SECTOR_SIZE