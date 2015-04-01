#pragma once

#include <string>
#include <map>
#include <algorithm>
#include "windows.h"
#include "mntImage.h"
#include "windows.h"
#include <atlsync.h>
#include "mntDriverControl.h"
#include <jcparam.h>

class CSyncMountManager
{
public:
	CSyncMountManager(void);
	~CSyncMountManager(void);
	
	UINT CreateDevice(ULONG64 total_sec, const CJCStringT & symbo_link);		// length in sectors
	void Connect(UINT dev_id, IImage * image);
	void MountDriver(UINT dev_id, TCHAR mount_point);
	void Disconnect(UINT dev_id);
	void UnmountImage(UINT dev_id);

	void InstallDriver(const CJCStringT & driver_fn);
	void UninstallDriver(void);

	//bool LoadUserModeDriver(const CJCStringT & drv_path, const CJCStringT & drv_name, jcparam::IValue * param, IImage *& obj/*, HMODULE & module*/);
	bool LoadUserModeDriver(const CJCStringT & drv_path, const CJCStringT & drv_name, const CJCStringT & config, IImage *& obj/*, HMODULE & module*/);
	void UnloadDriver(void);

protected:
	typedef std::map<UINT, CDriverControl*> DRIVER_MAP;
	typedef DRIVER_MAP::iterator DRIVER_MAP_IT;
	typedef std::pair<UINT, CDriverControl*> DRIVER_MAP_PAIR;
	DRIVER_MAP	m_driver_map;

	HMODULE				m_driver_module;
};