


#include "MountedDisk.h"
#include "Mountmanager.h"

#define LOGGER_LEVEL	LOGGER_LEVEL_DEBUGINFO
#include "jclogk.h"

//#include "new"
//#include "exception"
//#include "memory"

//MountManager::MountManager(PDRIVER_OBJECT driverObject):
//        m_driver_obj(driverObject), 
//        m_disk_count(0)

void MountManager::Initialize(IN PDRIVER_OBJECT driverObject)
{
	LOG_STACK_TRACE("");
	m_driver_obj = driverObject;
	ASSERT(m_driver_obj);
	m_disk_count = 0;


	NTSTATUS          status;
    UNICODE_STRING    device_dir_name;
    OBJECT_ATTRIBUTES object_attributes;

    RtlInitUnicodeString(&device_dir_name, ROOT_DIR_NAME);

    InitializeObjectAttributes(&object_attributes, &device_dir_name, 
				OBJ_OPENIF, NULL, NULL);

    HANDLE dir_handle;
    status = ZwCreateDirectoryObject(&dir_handle, DIRECTORY_ALL_ACCESS, 
				&object_attributes);

	// 初始化disk map
	RtlFillMemory(m_disk_map, sizeof(PDEVICE_OBJECT) * MAX_MOUNTED_DISK, 0);
    if (!NT_SUCCESS(status)) ASSERT(false);
}

NTSTATUS MountManager::Unmount(IN int dev_id)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");

	// illeagle parameter
	if (dev_id >= MAX_MOUNTED_DISK)
	{
		KdPrint(("dev_id (%d) is illeagle", dev_id));
		return STATUS_UNSUCCESSFUL;
	}
	//<TODO> need mutex
	PDEVICE_OBJECT fdo = m_disk_map[dev_id];
	if (fdo == NULL)
	{
		KdPrint(("dev_id (%d) is not exist", dev_id));
		return STATUS_UNSUCCESSFUL;
	}

	m_disk_map[dev_id] = NULL;
	m_disk_count --;
	// end mutex

	MountedDisk * mnt_disk = reinterpret_cast<MountedDisk*>(fdo->DeviceExtension);
	mnt_disk->Release();
	IoDeleteDevice(fdo);

    //AutoMutex guard(diskMapLock_);
	return STATUS_SUCCESS;
}

NTSTATUS MountManager::Mount(IN UINT64 totalLength, OUT int & dev_id)
{
	PAGED_CODE();
    // generate id
	LOG_STACK_TRACE("");

    dev_id = -1;
	//<TODO> need mutex
	// 找到一个空余的map位置
	if (m_disk_count >= MAX_MOUNTED_DISK)
	{
		KdPrint(("no empty disk (m_disk_count=%d)", m_disk_count));
		return STATUS_UNSUCCESSFUL;
	}

	int ii = 0;
	for ( ; ii<MAX_MOUNTED_DISK; ii++)
	{
		if (m_disk_map[ii] == NULL)
		{	dev_id = ii;	break;	}
	}
	ASSERT(ii < MAX_MOUNTED_DISK);
	KdPrint( ("found disk id:%d\n", dev_id) );
	// 找到空位置
	m_disk_count ++;
	m_disk_map[dev_id] = (PDEVICE_OBJECT)0xFFFFFFFF;
	// end of mutex

	// 创建设备
	// Make device name
    WCHAR device_name_buffer[MAXIMUM_FILENAME_LENGTH];
	int jj=0;
	while ( DIRECT_DISK_PREFIX[jj] !=0 )
	{
		device_name_buffer[jj] = DIRECT_DISK_PREFIX[jj];
		++jj;
	}
	device_name_buffer[jj++] = dev_id + L'0';
	device_name_buffer[jj] = 0;
	KdPrint( ("disk name:%S\n", device_name_buffer) );

	UNICODE_STRING str_prefix;
	RtlInitUnicodeString(&str_prefix, device_name_buffer);

    NTSTATUS status;

    //create device
	PDEVICE_OBJECT fdo = NULL;
    status = IoCreateDevice(m_driver_obj, sizeof(MountedDisk),
        &str_prefix, FILE_DEVICE_DISK,
        0, FALSE, &fdo);

    if (!NT_SUCCESS(status))
	{
		KdPrint( ("failure on creating disk device\n") );
		return status;
	}

	MountedDisk * mnt_disk = reinterpret_cast<MountedDisk*>(fdo->DeviceExtension);
	mnt_disk->Initialize(m_driver_obj, this, dev_id, totalLength);
	// set device name
	mnt_disk->SetDeviceName(device_name_buffer); 

    fdo->Flags |= DO_DIRECT_IO;
    fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	//<TODO> need mutex
	m_disk_map[dev_id] = fdo;
	// end of mutex

    return STATUS_SUCCESS;
}


NTSTATUS MountManager::RequestExchange(IN CORE_MNT_EXCHANGE_REQUEST * request, OUT CORE_MNT_EXCHANGE_RESPONSE * response)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");

	if (request->deviceId >= MAX_MOUNTED_DISK)
	{
		KdPrint(("Illeagle device id %d\n", request->deviceId));
		return STATUS_UNSUCCESSFUL;
	}

	//<TODO> need mutex
	PDEVICE_OBJECT fdo = m_disk_map[request->deviceId];
	// end mutex
	if (fdo == NULL)
	{
		KdPrint(("device id %d is not exist.\n", request->deviceId));
		return STATUS_UNSUCCESSFUL;
	}

	KdPrint(("exchange:dev_id=%d, last_type=%d, last_status=%d, last_size=%d\n",
		request->deviceId, request->lastType, request->lastStatus, request->lastSize));

	MountedDisk * mnt_disk = reinterpret_cast<MountedDisk*>(fdo->DeviceExtension);
	NTSTATUS status = mnt_disk->RequestExchange(request, response);

	KdPrint(("exchange:dev_id=%d, next_type=%d, next_size=%d\n",
		request->deviceId, response->type, response->size));

	return status;
}


