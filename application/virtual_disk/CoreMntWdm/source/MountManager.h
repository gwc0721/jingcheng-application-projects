#pragma once

#include "../../Comm/virtual_disk.h"



typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT fdo;
    PDEVICE_OBJECT NextStackDevice;
	UNICODE_STRING ustrDeviceName;	// 设备名
	UNICODE_STRING ustrSymLinkName;	// 符号链接名
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


class CMountManager : public DEVICE_EXTENSION
{
public:
    //CMountManager(PDRIVER_OBJECT driverObject);
	void Initialize(IN PDRIVER_OBJECT driverObject);
	void Release(void);

	// return device id
    NTSTATUS Mount(IN UINT64 totalLength, OUT UINT32 & dev_id);
    NTSTATUS Unmount(IN UINT32 devId);
    NTSTATUS DispatchIrp(IN UINT32 devId, IN PIRP irp);
    NTSTATUS RequestExchange(IN CORE_MNT_EXCHANGE_REQUEST * request, OUT CORE_MNT_EXCHANGE_RESPONSE * response);
	NTSTATUS DisconnectDisk(IN UINT32 dev_id);

	NTSTATUS DispatchDeviceControl(IN PIRP irp, IN PIO_STACK_LOCATION irp_stack);

	bool CanBeRemoved(void)	{return m_disk_count == 0;}

private:
    PDRIVER_OBJECT m_driver_obj;
    int m_disk_count;

	bool DiskId2Ptr(IN UINT32 dev_id, OUT CMountedDisk * &disk);

	PDEVICE_OBJECT m_disk_map[MAX_MOUNTED_DISK];
};
