#pragma once

#include "../../Comm/virtual_disk.h"


typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT fdo;
    PDEVICE_OBJECT NextStackDevice;
	UNICODE_STRING ustrDeviceName;	// 设备名
	UNICODE_STRING ustrSymLinkName;	// 符号链接名
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


class MountManager : public DEVICE_EXTENSION
{
public:
    //MountManager(PDRIVER_OBJECT driverObject);
	void Initialize(IN PDRIVER_OBJECT driverObject);

	// return device id
    NTSTATUS Mount(IN UINT64 totalLength, OUT int & dev_id);
    NTSTATUS Unmount(IN int devId);
    NTSTATUS DispatchIrp(IN int devId, IN PIRP irp);
    NTSTATUS RequestExchange(IN CORE_MNT_EXCHANGE_REQUEST * request, OUT CORE_MNT_EXCHANGE_RESPONSE * response);

private:
    PDRIVER_OBJECT m_driver_obj;
    int m_disk_count;

	PDEVICE_OBJECT m_disk_map[MAX_MOUNTED_DISK];
};
