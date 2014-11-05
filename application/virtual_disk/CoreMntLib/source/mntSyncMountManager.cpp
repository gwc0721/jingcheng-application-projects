#include "stdafx.h"
LOCAL_LOGGER_ENABLE(_T("SyncMntManager"), LOGGER_LEVEL_DEBUGINFO);

#include <stdext.h>


#include "../include/mntSyncMountmanager.h"
#include "process.h"
#include <sstream>
#include "boost\scope_exit.hpp"
#include <iostream>

#include "../../Comm/virtual_disk.h"


///////////////////////////////////////////////////////////////////////////////
// -- 

UINT CSyncMountManager::CreateDevice(ULONG64 total_sec, const CJCStringT & symbo_link)
{
	CDriverControl * drv_ctrl = new CDriverControl(total_sec, symbo_link);	// length in sectors
	UINT dev_id = drv_ctrl->GetDeviceId();
	m_driver_map.insert(DRIVER_MAP_PAIR(dev_id, drv_ctrl) );
	return dev_id;
}

void CSyncMountManager::Connect(UINT dev_id, IImage * image)
{
	DRIVER_MAP_IT it = m_driver_map.find(dev_id);
	if (it == m_driver_map.end() ) THROW_ERROR(ERR_PARAMETER, _T("dev id %d do not exist"), dev_id);
	CDriverControl * drv_ctrl = it->second;
	drv_ctrl->Connect(image);
}

void CSyncMountManager::MountDriver(UINT dev_id, TCHAR mount_point)
{
	DRIVER_MAP_IT it = m_driver_map.find(dev_id);
	if (it == m_driver_map.end() ) THROW_ERROR(ERR_PARAMETER, _T("dev id %d do not exist"), dev_id);
	CDriverControl * drv_ctrl = it->second;
	drv_ctrl->Mount(mount_point);
}

void CSyncMountManager::Disconnect(UINT dev_id)
{
	DRIVER_MAP_IT it = m_driver_map.find(dev_id);
	if (it == m_driver_map.end() ) THROW_ERROR(ERR_PARAMETER, _T("dev id %d do not exist"), dev_id);
	CDriverControl * drv_ctrl = it->second;
	drv_ctrl->Disconnect();
}

void CSyncMountManager::UnmountImage(UINT dev_id)
{
	DRIVER_MAP_IT it = m_driver_map.find(dev_id);
	if (it == m_driver_map.end() ) THROW_ERROR(ERR_PARAMETER, _T("dev id %d do not exist"), dev_id);
	CDriverControl * drv_ctrl = it->second;
	m_driver_map.erase(it->first);

	drv_ctrl->Unmount();
	delete drv_ctrl;
}
