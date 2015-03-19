#include "stdext.h"

#include "../include/device_base.h"

LPCTSTR CSmiDeviceBase::PROP_WPRO  =		_T("WPRO");				// UINT
LPCTSTR CSmiDeviceBase::PROP_CACHE_NUM =	_T("CACHE_BLOCK_NUM");	// UINT
LPCTSTR CSmiDeviceBase::PROP_INFO_BLOCK =	_T("INFO_BLOCK");		// UINT, hi word: info 1, lo word: info 2
LPCTSTR CSmiDeviceBase::PROP_ISP_BLOCK =	_T("ISP_BLOCK");		// UINT, hi word: isp 1, lo word: isp 2
LPCTSTR CSmiDeviceBase::PROP_INFO_PAGE =	_T("INFO_PAGE");		// UINT
LPCTSTR CSmiDeviceBase::PROP_ORG_BAD_INFO = _T("ORG_BAD_INFO");		// UINT
LPCTSTR CSmiDeviceBase::PROP_ISP_MODE =		_T("ISP_MODE");			// UINT, hi word: info_valid; lo word: isp_mode
LPCTSTR CSmiDeviceBase::PROP_HBLOCK_NUM =	_T("HBLOCK_NUM");			// UINT, hi word: info_valid; lo word: isp_mode


