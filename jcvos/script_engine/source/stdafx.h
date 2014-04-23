#pragma once

#ifndef _WIN32_WINNT		//                
#define _WIN32_WINNT 0x0501	//
#endif		

#define LOGGER_LEVEL LOGGER_LEVEL_DEBUGINFO

#ifdef _DEBUG
#define LOG_OUT_CLASS_SIZE
#endif

// configurable defines

// 一次从文件读入内存的大小
#ifndef SRC_READ_SIZE
#define SRC_READ_SIZE	(4096)
#endif

// 内存后的预留大小
#ifndef SRC_BACK_SIZE
#define	SRC_BACK_SIZE	(1024)
#endif



