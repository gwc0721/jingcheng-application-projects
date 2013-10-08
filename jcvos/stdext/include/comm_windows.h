#pragma once
/*
 * comm_windows.h
 *
 *  Created on: Jun 7, 2012
 *      Author: jingcheng
 */

#ifndef COMM_WINDOWS_H_
#define COMM_WINDOWS_H_

#include <windows.h>
//#include <winnt.h>
#include <tchar.h>

#define		LockedIncrement(ii)		InterlockedIncrement(&ii)
#define 	LockedDecrement(ii)		InterlockedDecrement(&ii)

#define	 jcbreak	__debugbreak()

#endif /* COMM_WINDOWS_H_ */
