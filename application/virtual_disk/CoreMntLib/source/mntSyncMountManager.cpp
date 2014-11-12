#include "stdafx.h"
LOCAL_LOGGER_ENABLE(_T("SyncMntManager"), LOGGER_LEVEL_DEBUGINFO);

#include "../include/mntSyncMountmanager.h"
#include "process.h"
#include <sstream>
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

void CSyncMountManager::InstallDriver(const CJCStringT & driver_fn)
{
	LOG_STACK_TRACE();
	LOG_DEBUG(_T("driver = %s"), driver_fn.c_str());
	SC_HANDLE hdl = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hdl == NULL) THROW_WIN32_ERROR(_T(" openning scm failed!"));

	SC_HANDLE srv = CreateService(hdl, _T("CoreMnt"), _T("CoreMnt"), SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START, SERVICE_ERROR_CRITICAL, driver_fn.c_str(), NULL, NULL, NULL, NULL, NULL);
	if (srv == NULL)
	{
		DWORD ir = GetLastError();
		LOG_DEBUG(_T("create service failed, code=%d"), ir);
		if ( (ir == ERROR_IO_PENDING) || (ir == ERROR_SERVICE_EXISTS) )
		{
			LOG_DEBUG(_T("service is existing, retry for open"));
			srv = OpenService(hdl, _T("CoreMnt"), SERVICE_ALL_ACCESS);
		}
		if (srv == NULL)	THROW_WIN32_ERROR(_T("creating service failed!"));
	}

	BOOL br = StartService(srv, NULL, NULL);
	if (!br)
	{
		LOG_DEBUG(_T("start service failed"));
		DWORD ir = GetLastError();
		if (ir == ERROR_SERVICE_ALREADY_RUNNING)
		{
			LOG_DEBUG(_T("service is running."))
		}
		else THROW_WIN32_ERROR(_T("start service failed"))
	}

	//SC_HANDLE driver = OpenService(hdl, _T("CoreMnt"), 
	//ControlService(srv, SERVICE_CONTROL_STOP, NULL);

	CloseServiceHandle(srv);
	CloseServiceHandle(hdl);
}
