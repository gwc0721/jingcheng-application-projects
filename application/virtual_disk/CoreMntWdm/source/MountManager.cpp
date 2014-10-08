﻿


#include "CoreMnt.h"
#include "MountedDisk.h"
#include "Mountmanager.h"

#define LOGGER_LEVEL	LOGGER_LEVEL_DEBUGINFO
#include "jclogk.h"

//#include "new"
//#include "exception"
//#include "memory"

//CMountManager::CMountManager(PDRIVER_OBJECT driverObject):
//        m_driver_obj(driverObject), 
//        m_disk_count(0)

#pragma PAGEDCODE
void CMountManager::Initialize(IN PDRIVER_OBJECT driverObject)
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

void CMountManager::Release(void)
{
	//<TODO> need mutex
	for (UINT32 ii =0; ii < MAX_MOUNTED_DISK; ++ii)
	{
		PDEVICE_OBJECT fdo = m_disk_map[ii];
		m_disk_map[ii] = NULL;
		if (fdo)
		{	// unmount
			CMountedDisk * mnt_disk = reinterpret_cast<CMountedDisk*>(fdo->DeviceExtension);
			mnt_disk->Release();
			IoDeleteDevice(fdo);
		}
	}
	//end mutex
}

bool CMountManager::DiskId2Ptr(IN UINT32 dev_id, OUT CMountedDisk * &disk)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");
	ASSERT(disk == NULL);

	// illeagle parameter
	if (dev_id >= MAX_MOUNTED_DISK)
	{
		KdPrint(("dev_id (%d) is illeagle", dev_id));
		return false;
	}
	//<TODO> need mutex
	PDEVICE_OBJECT fdo = m_disk_map[dev_id];
	// end mutex
	if (fdo == NULL)
	{
		KdPrint(("dev_id (%d) is not exist", dev_id));
		return false;
	}

	disk = reinterpret_cast<CMountedDisk*>(fdo->DeviceExtension);
	return true;
}


NTSTATUS CMountManager::Unmount(IN UINT32 dev_id)
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

	CMountedDisk * mnt_disk = reinterpret_cast<CMountedDisk*>(fdo->DeviceExtension);
	mnt_disk->Release();
	IoDeleteDevice(fdo);

	return STATUS_SUCCESS;
}

// length in sectors
NTSTATUS CMountManager::Mount(IN UINT64 totalLength, OUT UINT32 & dev_id)
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

	UINT32 ii = 0;
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

	UNICODE_STRING str_device_name;
	RtlInitUnicodeString(&str_device_name, device_name_buffer);

    NTSTATUS status;

    //create device
	PDEVICE_OBJECT fdo = NULL;
    status = IoCreateDevice(m_driver_obj, sizeof(CMountedDisk),
        &str_device_name, FILE_DEVICE_DISK,
        0, FALSE, &fdo);

    if (!NT_SUCCESS(status))
	{
		KdPrint( ("failure on creating disk device\n") );
		return status;
	}


    WCHAR symbo_link_buf[MAXIMUM_FILENAME_LENGTH];
	jj = 0;
	while ( SYMBO_DIRECT_DISK[jj] !=0 )
	{
		symbo_link_buf[jj] = SYMBO_DIRECT_DISK[jj];
		++jj;
	}
	symbo_link_buf[jj++] = dev_id + L'0';
	symbo_link_buf[jj] = 0;
	KdPrint( ("symbo link:%S\n", symbo_link_buf) );

	UNICODE_STRING str_symbo_link;
	RtlInitUnicodeString(&str_symbo_link, symbo_link_buf);

	status = IoCreateSymbolicLink(&str_symbo_link, &str_device_name);
    if (!NT_SUCCESS(status))
	{
		KdPrint( ("failure on creating symbolink\n") );
		return status;
	}

	CMountedDisk * mnt_disk = reinterpret_cast<CMountedDisk*>(fdo->DeviceExtension);
	mnt_disk->Initialize(m_driver_obj, this, dev_id, totalLength);	// length in sectors
	// set device name
	mnt_disk->SetDeviceName(&str_device_name, &str_symbo_link); 

    fdo->Flags |= DO_DIRECT_IO;
    fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	//<TODO> need mutex
	m_disk_map[dev_id] = fdo;
	// end of mutex

    return STATUS_SUCCESS;
}


NTSTATUS CMountManager::RequestExchange(IN CORE_MNT_EXCHANGE_REQUEST * request, OUT CORE_MNT_EXCHANGE_RESPONSE * response)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");

	CMountedDisk *mnt_disk = NULL;
	if ( !DiskId2Ptr(request->dev_id, mnt_disk) ) return STATUS_UNSUCCESSFUL;

	KdPrint(("exchange:dev_id=%d, last_type=%d, last_status=%d, last_size=%d\n",
		request->dev_id, request->lastType, request->lastStatus, request->lastSize));

	NTSTATUS status = mnt_disk->RequestExchange(request, response);

	KdPrint(("exchange:dev_id=%d, next_type=%d, next_size=%d\n",
		request->dev_id, response->type, response->size));

	return status;
}

NTSTATUS CMountManager::DisconnectDisk(IN UINT32 dev_id)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");
	CMountedDisk *mnt_disk = NULL;
	if ( !DiskId2Ptr(dev_id, mnt_disk) ) return STATUS_UNSUCCESSFUL;
	return mnt_disk->Disconnect();
}


NTSTATUS CMountManager::DispatchDeviceControl(IN PIRP irp, IN PIO_STACK_LOCATION irp_stack)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");

	ULONG code = irp_stack->Parameters.DeviceIoControl.IoControlCode;

    PVOID buffer = irp->AssociatedIrp.SystemBuffer;
    ULONG out_buf_len = irp_stack->Parameters.DeviceIoControl.OutputBufferLength;    
    ULONG in__buf_len = irp_stack->Parameters.DeviceIoControl.InputBufferLength; 
	KdPrint( ("input buffer len=%d, output buf len=%d\n", in__buf_len, out_buf_len) );

    NTSTATUS status = STATUS_SUCCESS;
    switch (code)
    {
    case CORE_MNT_MOUNT_IOCTL:	{
		KdPrint(("[IRP] mnt <- IRP_MJ_DEVICE_CONTROL::CORE_MNT_MOUNT_IOCTL\n"));

		if(in__buf_len < sizeof(CORE_MNT_MOUNT_REQUEST) || 
			out_buf_len < sizeof(CORE_MNT_MOUNT_RESPONSE) )
		{
			KdPrint( ("input or output buffer size mismatch\n") );
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		CORE_MNT_MOUNT_REQUEST * request = (CORE_MNT_MOUNT_REQUEST *)buffer;
		UINT64 total_sec = request->total_sec;		// length in sectors
		KdPrint( ("disk size=%I64d sectors\n", total_sec) );

		UINT32 dev_id = -1;
		status = Mount(total_sec, dev_id);
		if ( !NT_SUCCESS(status) )		{	KdPrint( ("disk map is full\n") );		}

		CORE_MNT_MOUNT_RESPONSE * response = (CORE_MNT_MOUNT_RESPONSE *)buffer;
		response->dev_id = dev_id;
		break;					}

	case CORE_MNT_EXCHANGE_IOCTL:		{
		KdPrint( ("[IRP] mnt <- IRP_MJ_DEVICE_CONTROL::CORE_MNT_EXCHANGE_IOCTL\n"));
		if(	in__buf_len < sizeof(CORE_MNT_EXCHANGE_REQUEST) || 
			out_buf_len < sizeof(CORE_MNT_EXCHANGE_RESPONSE) )
		{
			KdPrint( ("input or output buffer size mismatch\n") );
			status = STATUS_UNSUCCESSFUL;
			break;
		}
		CORE_MNT_EXCHANGE_REQUEST * request = (CORE_MNT_EXCHANGE_REQUEST *)buffer;
		CORE_MNT_EXCHANGE_RESPONSE response = {0};
		RequestExchange(request, &response);
		KdPrint(("response: type=%d, size=%d\n", response.type, response.size));
		RtlCopyMemory(buffer, &response, sizeof(CORE_MNT_EXCHANGE_RESPONSE) );
		status = STATUS_SUCCESS;
		break;								}

	case CORE_MNT_UNMOUNT_IOCTL:		{
		KdPrint( ("[IRP] mnt <- IRP_MJ_DEVICE_CONTROL::CORE_MNT_UNMOUNT_IOCTL\n"));
		if(in__buf_len < sizeof(CORE_MNT_UNMOUNT_REQUEST))
		{
			KdPrint( ("input buffer size mismatch\n") );
			status = STATUS_UNSUCCESSFUL;
			break;
		}
		CORE_MNT_UNMOUNT_REQUEST * request = (CORE_MNT_UNMOUNT_REQUEST *)buffer;
		status = Unmount(request->dev_id);
		break;		}

	case CORE_MNT_DISCONNECT_IOCTL:		{
		KdPrint( ("[IRP] mnt <- IRP_MJ_DEVICE_CONTROL::CORE_MNT_DISCONNECT_IOCTL\n"));
		if(in__buf_len < sizeof(CORE_MNT_COMM))
		{
			KdPrint( ("input buffer size mismatch\n") );
			status = STATUS_UNSUCCESSFUL;
			break;
		}
		CORE_MNT_COMM * request = (CORE_MNT_COMM *)buffer;
		status = DisconnectDisk(request->dev_id);
		break;		}

	default:
		KdPrint(("[IRP] mnt <- IRP_MJ_DEVICE_CONTROL::UNKNOW_MINOR_FUNC(0x%08X)\n", code));
		status = STATUS_NOT_IMPLEMENTED;
		break;
    }
    return CompleteIrp(irp, status, out_buf_len);
}

