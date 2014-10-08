#include "MountedDisk.h"

#define LOGGER_LEVEL	LOGGER_LEVEL_DEBUGINFO
#include "jclogk.h"

extern "C"
{
#include "ntdddisk.h"
#include "ntddcdrm.h"
#include "mountmgr.h"
#include "mountdev.h"
}

// dummy irp for internal operation
#define DIRP_DISCONNECT		1

#pragma PAGEDCODE
inline NTSTATUS CompleteIrp( PIRP irp, NTSTATUS status, ULONG info)
{
    irp->IoStatus.Status = status;
    irp->IoStatus.Information = info;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return status;
}

PVOID GetIrpBuffer(PIRP irp)
{
    PVOID systemBuffer = 0;
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(irp);
    if(ioStack->MajorFunction == IRP_MJ_READ || ioStack->MajorFunction == IRP_MJ_WRITE)
        systemBuffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
    else
        systemBuffer = irp->AssociatedIrp.SystemBuffer;
    return systemBuffer;
}

void GetIrpParam(IN PIRP irp, OUT IrpParam * param)
{
	PAGED_CODE();
	//LOG_STACK_TRACE("");

	if ( (UINT32)irp == DIRP_DISCONNECT )
	{
		param->type = DISK_OP_DISCONNECT;
		param->buffer = NULL;
		param->offset = 0;
		param->size = 0;
		return;
	}
	
	PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(irp);

    param->offset = 0;
    param->type = DISK_OP_EMPTY;
    param->buffer = (char*)GetIrpBuffer(irp);

    if (ioStack->MajorFunction == IRP_MJ_READ)
    {
		//KdPrint(("major: IRP_MJ_READ\n"));
        param->type = DISK_OP_READ;
        param->size = ioStack->Parameters.Read.Length;
        param->offset = ioStack->Parameters.Read.ByteOffset.QuadPart; 
    }
	else if(ioStack->MajorFunction == IRP_MJ_WRITE)
    {
		//KdPrint(("major: IRP_MJ_WRITE\n"));
        param->type = DISK_OP_WRITE;
        param->size = ioStack->Parameters.Write.Length;
        param->offset = ioStack->Parameters.Write.ByteOffset.QuadPart; 
    }
    return;    
}

//CMountedDisk::CMountedDisk(PDRIVER_OBJECT	DriverObject, 
//                         CMountManager* mountManager, 
//                         UINT32 devId, 
//                         UINT64 totalLength)
//              : irpQueue_(irpQueueNotEmpty_), 
//                m_last_irp(0), 
//                irpDispatcher_(devId, totalLength, DriverObject, mountManager)
//{
//}

//CMountedDisk::~CMountedDisk()
//{
//    stopEvent_.set();
//    //complete all IRP
//    if(m_last_irp)
//        CompleteLastIrp(STATUS_DEVICE_NOT_READY, 0);
//
//    while(irpQueue_.pop(m_last_irp))
//        CompleteLastIrp(STATUS_DEVICE_NOT_READY, 0);
//}

void CMountedDisk::Initialize(PDRIVER_OBJECT DriverObject, CMountManager* mountManager,
			UINT32 devId, UINT64 total_sec)
{
	m_driver_obj = DriverObject;
	m_mnt_manager = mountManager;
	m_dev_id = devId;
	m_total_sectors = total_sec;		// length in sectors
	m_last_irp = NULL;
	
	RtlFillMemory(m_device_name, sizeof(WCHAR) * MAXIMUM_FILENAME_LENGTH, 0);
	RtlFillMemory(m_symbo_link, sizeof(WCHAR) * MAXIMUM_FILENAME_LENGTH, 0);

	m_irp_queue.Initialize();
}

void CMountedDisk::Release(void)
{
	while (! m_irp_queue.empty() )
	{
		PIRP irp;
		m_irp_queue.pop(irp);
		if ( (UINT32)irp > 0x10000)	CompleteIrp(irp, STATUS_DEVICE_NOT_READY, 0);
	}
	UNICODE_STRING _symbo_;
	RtlInitUnicodeString(&_symbo_, m_symbo_link);
	IoDeleteSymbolicLink(&_symbo_);
	//m_irp_queue.Release();
}

NTSTATUS CMountedDisk::DispatchIrp(IN PIRP irp)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");

    IrpParam irp_param;
	GetIrpParam(irp, &irp_param);

	//KdPrint(("operation type: %d\n", irp_param.type));

	NTSTATUS status;
    if(irp_param.type == DISK_OP_EMPTY)
    {
		LocalDispatch(irp);
        status = irp->IoStatus.Status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
	else
	{
#if DBG
		if (irp_param.type == DISK_OP_READ)	KdPrint(("[IRP] disk <- IRP_MJ_READ\n"));
		else if (irp_param.type == DISK_OP_WRITE)	KdPrint(("[IRP] disk <- IRP_MJ_WRITE\n"));
#endif
		IoMarkIrpPending( irp );
		KdPrint(("push irp to queue\n"));
		m_irp_queue.push(irp);
		status = STATUS_PENDING;
	}
	return status;
}

/*
void CMountedDisk::GetIrpParam(IN PIRP irp, OUT IrpParam & param)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");
	
	PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(irp);

    param.offset = 0;
    param.type = DISK_OP_EMPTY;
    param.buffer = (char*)GetIrpBuffer(irp);

    if (ioStack->MajorFunction == IRP_MJ_READ)
    {
		KdPrint(("major: IRP_MJ_READ\n"));
        param.type = DISK_OP_READ;
        param.size = ioStack->Parameters.Read.Length;
        param.offset = ioStack->Parameters.Read.ByteOffset.QuadPart; 
    }
	else if(ioStack->MajorFunction == IRP_MJ_WRITE)
    {
		KdPrint(("major: IRP_MJ_WRITE\n"));
        param.type = DISK_OP_WRITE;
        param.size = ioStack->Parameters.Write.Length;
        param.offset = ioStack->Parameters.Write.ByteOffset.QuadPart; 
    }
    return;    
}
*/

// 处理简单的irp，在驱动内处理完并且Complete
void CMountedDisk::LocalDispatch(IN PIRP irp)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");

    PIO_STACK_LOCATION io_stack = IoGetCurrentIrpStackLocation(irp);
    switch(io_stack->MajorFunction)
    {
    case IRP_MJ_CREATE:
    case IRP_MJ_CLOSE:
		KdPrint( ("[IRP] disk <- IRP_MJ_CREATE / IRP_MJ_CLOSE\n") );
        irp->IoStatus.Status = STATUS_SUCCESS;
        irp->IoStatus.Information = 0;
        break;

    case IRP_MJ_QUERY_VOLUME_INFORMATION:
        KdPrint( ("[IRP] disk <- IRP_MJ_QUERY_VOLUME_INFORMATION\n") );
        irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        irp->IoStatus.Information = 0;
        break;

	case IRP_MJ_DEVICE_CONTROL: {
		ULONG code = io_stack->Parameters.DeviceIoControl.IoControlCode;
		//KdPrint(("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::"));
		switch (code)
		{
		case IOCTL_DISK_GET_DRIVE_LAYOUT:		{
			KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_DISK_GET_DRIVE_LAYOUT\n") );
			if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof (DRIVE_LAYOUT_INFORMATION))
			{
				irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
				irp->IoStatus.Information = 0;
				break;
			}
			PDRIVE_LAYOUT_INFORMATION outputBuffer = (PDRIVE_LAYOUT_INFORMATION)
					irp->AssociatedIrp.SystemBuffer;

			outputBuffer->PartitionCount = 1;
			outputBuffer->Signature = 0;

			outputBuffer->PartitionEntry->PartitionType = PARTITION_ENTRY_UNUSED;
			outputBuffer->PartitionEntry->BootIndicator = FALSE;
			outputBuffer->PartitionEntry->RecognizedPartition = TRUE;
			outputBuffer->PartitionEntry->RewritePartition = FALSE;
			outputBuffer->PartitionEntry->StartingOffset = RtlConvertUlongToLargeInteger (0);
			// length in sectors
			outputBuffer->PartitionEntry->PartitionLength.QuadPart= m_total_sectors * SECTOR_SIZE;
			outputBuffer->PartitionEntry->HiddenSectors = 1L;

			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = sizeof (PARTITION_INFORMATION);
			break;		}

		case IOCTL_DISK_CHECK_VERIFY:
		case IOCTL_CDROM_CHECK_VERIFY:
		case IOCTL_STORAGE_CHECK_VERIFY:
		case IOCTL_STORAGE_CHECK_VERIFY2:	{
			KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_xxx_CHECK_VERIFY\n") );
			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = 0;
			break;							}

		case IOCTL_DISK_GET_DRIVE_GEOMETRY:
		case IOCTL_CDROM_GET_DRIVE_GEOMETRY:		{
			KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_xxx_GET_DRIVE_GEOMETRY\n") );

			PDISK_GEOMETRY  disk_geometry;
			//ULONGLONG       length;

			if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(DISK_GEOMETRY))
			{
				irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
				irp->IoStatus.Information = 0;
				break;
			}
			disk_geometry = (PDISK_GEOMETRY) irp->AssociatedIrp.SystemBuffer;
			//length = m_total_length;
			//disk_geometry->Cylinders.QuadPart = length / SECTOR_SIZE / 0x20 / 0x80;
			disk_geometry->Cylinders.QuadPart = m_total_sectors / 0x20 / 0x80;			// length in sectors
			disk_geometry->MediaType = FixedMedia;
			disk_geometry->TracksPerCylinder = 0x80;
			disk_geometry->SectorsPerTrack = 0x20;
			disk_geometry->BytesPerSector = SECTOR_SIZE;
			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
			break;								}

		case IOCTL_DISK_GET_LENGTH_INFO:		{
			KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_DISK_GET_LENGTH_INFO\n") );
			PGET_LENGTH_INFORMATION get_length_information;
			if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(GET_LENGTH_INFORMATION))
			{
				irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
				irp->IoStatus.Information = 0;
				break;
			}
			get_length_information = (PGET_LENGTH_INFORMATION) irp->AssociatedIrp.SystemBuffer;
			get_length_information->Length.QuadPart = m_total_sectors * SECTOR_SIZE;		// length in sectors
			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = sizeof(GET_LENGTH_INFORMATION);
			break;								}

		case IOCTL_DISK_GET_PARTITION_INFO:		{
			KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_DISK_GET_PARTITION_INFO\n") );

			PPARTITION_INFORMATION  partition_information;
			//ULONGLONG               length;
			if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(PARTITION_INFORMATION))
			{
				irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
				irp->IoStatus.Information = 0;
				break;
			}
			partition_information = (PPARTITION_INFORMATION) irp->AssociatedIrp.SystemBuffer;
			//length = m_total_length;
			partition_information->StartingOffset.QuadPart = 0;
			partition_information->PartitionLength.QuadPart = m_total_sectors * SECTOR_SIZE;
			partition_information->HiddenSectors = 0;
			partition_information->PartitionNumber = 0;
			partition_information->PartitionType = 0;
			partition_information->BootIndicator = FALSE;
			partition_information->RecognizedPartition = TRUE;
			partition_information->RewritePartition = FALSE;
			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);
			break;								}

		case IOCTL_DISK_GET_PARTITION_INFO_EX:	{
			KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_DISK_GET_PARTITION_INFO_EX\n") );

			PPARTITION_INFORMATION_EX   partition_information_ex;
			//ULONGLONG                   length;
			if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(PARTITION_INFORMATION_EX))
			{
				irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
				irp->IoStatus.Information = 0;
				break;
			}
			partition_information_ex = (PPARTITION_INFORMATION_EX) irp->AssociatedIrp.SystemBuffer;
			//length = m_total_length;
			partition_information_ex->PartitionStyle = PARTITION_STYLE_MBR;
			partition_information_ex->StartingOffset.QuadPart = 0;
			partition_information_ex->PartitionLength.QuadPart = m_total_sectors * SECTOR_SIZE;
			partition_information_ex->PartitionNumber = 0;
			partition_information_ex->RewritePartition = FALSE;
			partition_information_ex->Mbr.PartitionType = 0;
			partition_information_ex->Mbr.BootIndicator = FALSE;
			partition_information_ex->Mbr.RecognizedPartition = TRUE;
			partition_information_ex->Mbr.HiddenSectors = 0;
			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = sizeof(PARTITION_INFORMATION_EX);
			break;								}

		case IOCTL_DISK_IS_WRITABLE:			{
			KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_DISK_IS_WRITABLE\n") );
			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = 0;
			break;								}

		case IOCTL_DISK_MEDIA_REMOVAL:
		case IOCTL_STORAGE_MEDIA_REMOVAL:		{
			KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_xxx_MEDIA_REMOVAL\n") );
			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = 0;
			break;								}

		case IOCTL_CDROM_READ_TOC:				{
			KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_CDROM_READ_TOC\n") );
			PCDROM_TOC cdrom_toc;
			if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(CDROM_TOC))
			{
				irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
				irp->IoStatus.Information = 0;
				break;
			}
			cdrom_toc = (PCDROM_TOC) irp->AssociatedIrp.SystemBuffer;
			RtlZeroMemory(cdrom_toc, sizeof(CDROM_TOC));
			cdrom_toc->FirstTrack = 1;
			cdrom_toc->LastTrack = 1;
			cdrom_toc->TrackData[0].Control = TOC_DATA_TRACK;
			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = sizeof(CDROM_TOC);
			break;								}

		case IOCTL_DISK_SET_PARTITION_INFO:		{
			KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_DISK_SET_PARTITION_INFO\n") );
			if (io_stack->Parameters.DeviceIoControl.InputBufferLength <
				sizeof(SET_PARTITION_INFORMATION))
			{
				irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
				irp->IoStatus.Information = 0;
				break;
			}
			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = 0;
			break;							}

		case IOCTL_DISK_VERIFY:					{
			KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_DISK_VERIFY\n") );
			PVERIFY_INFORMATION verify_information;
			if (io_stack->Parameters.DeviceIoControl.InputBufferLength <
				sizeof(VERIFY_INFORMATION))
			{
				irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
				irp->IoStatus.Information = 0;
				break;
			}
			verify_information = (PVERIFY_INFORMATION) irp->AssociatedIrp.SystemBuffer;
			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = verify_information->Length;
			break;								}	

		case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:	{
			KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_MOUNTDEV_QUERY_DEVICE_NAME\n") );
			if(io_stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof (MOUNTDEV_NAME))
			{
				irp->IoStatus.Information = sizeof (MOUNTDEV_NAME);
				irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
			}
			else
			{
				PMOUNTDEV_NAME devName = (PMOUNTDEV_NAME)irp->AssociatedIrp.SystemBuffer;
				UNICODE_STRING deviceName;
				RtlInitUnicodeString(&deviceName, m_device_name);

				devName->NameLength = deviceName.Length;
				ULONG outLength = sizeof(USHORT) + deviceName.Length;
				if(io_stack->Parameters.DeviceIoControl.OutputBufferLength < outLength)
				{
					irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
					irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
					break;
				}

				RtlCopyMemory(devName->Name, deviceName.Buffer, deviceName.Length);

				irp->IoStatus.Status = STATUS_SUCCESS;
				irp->IoStatus.Information = outLength;
			}
			break;								}

		case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:	{
			KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_MOUNTDEV_QUERY_UNIQUE_ID\n") );
			if(io_stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof (MOUNTDEV_UNIQUE_ID))
			{
				irp->IoStatus.Information = sizeof (MOUNTDEV_UNIQUE_ID);
				irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
			}
			else
			{
				#define UNIQUE_ID_PREFIX L"coreMntMountedDrive"
				PMOUNTDEV_UNIQUE_ID mountDevId = (PMOUNTDEV_UNIQUE_ID)irp->AssociatedIrp.SystemBuffer;
	            
				USHORT unique_id_length;
				UNICODE_STRING uniqueId;
				RtlInitUnicodeString(&uniqueId, m_device_name);
				unique_id_length = uniqueId.Length;
	            
				mountDevId->UniqueIdLength = uniqueId.Length;
				ULONG outLength = sizeof(USHORT) + uniqueId.Length;
				if(io_stack->Parameters.DeviceIoControl.OutputBufferLength < outLength)
				{
					irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
					irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
					break;
				}
	            
				RtlCopyMemory(mountDevId->UniqueId, uniqueId.Buffer, uniqueId.Length);
	            
				irp->IoStatus.Status = STATUS_SUCCESS;
				irp->IoStatus.Information = outLength;
			}
			break;								}

		case IOCTL_MOUNTDEV_QUERY_STABLE_GUID:	{
			KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_MOUNTDEV_QUERY_STABLE_GUID\n") );
			irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
			irp->IoStatus.Information = 0;
			break;	}


		case IOCTL_STORAGE_GET_HOTPLUG_INFO:			{
			KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_STORAGE_GET_HOTPLUG_INFO\n") );
			if (io_stack->Parameters.DeviceIoControl.OutputBufferLength < 
				sizeof(STORAGE_HOTPLUG_INFO)) 
			{
				irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			PSTORAGE_HOTPLUG_INFO hotplug = 
				(PSTORAGE_HOTPLUG_INFO)irp->AssociatedIrp.SystemBuffer;

			RtlZeroMemory(hotplug, sizeof(STORAGE_HOTPLUG_INFO));

			hotplug->Size = sizeof(STORAGE_HOTPLUG_INFO);
			hotplug->MediaRemovable = 1;

			irp->IoStatus.Information = sizeof(STORAGE_HOTPLUG_INFO);
			irp->IoStatus.Status = STATUS_SUCCESS;
			break;										}

		/*
		case IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY:	{
			irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
			irp->IoStatus.Information = 0;
			break;										}
		case GUID_IO_VOLUME_UNIQUE_ID_CHANGE:	break;
		case IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_READWRITE:
			irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
			irp->IoStatus.Information = 0;
			break;
		*/

		default:
			irp->IoStatus.Information = 0;
			irp->IoStatus.Status = STATUS_SUCCESS;
			KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::UNKNOW_MINOR_FUNC(0x%08X)\n", io_stack->MinorFunction));
		}
		break; }

    default:
		KdPrint( ("[IRP] disk <- UNKNOW_MAJOR_FUNC(0x%08X)\n", io_stack->MajorFunction));
     }
}

NTSTATUS CMountedDisk::RequestExchange(IN CORE_MNT_EXCHANGE_REQUEST * request, OUT CORE_MNT_EXCHANGE_RESPONSE * response)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");

	DISK_OPERATION_TYPE last_type = (DISK_OPERATION_TYPE)(request->lastType);

	// 返回上次处理结果
    if(last_type != DISK_OP_EMPTY)
    {
		KdPrint(("processing last irp\n"));
        ASSERT(m_last_irp);
        m_last_irp->IoStatus.Status = request->lastStatus;
        if(request->lastStatus == STATUS_SUCCESS && last_type == DISK_OP_READ)
        {
			KdPrint(("last irp is read cmd\n"));
            IrpParam irp_param;
			GetIrpParam(m_last_irp, &irp_param);
            if(irp_param.buffer)	RtlCopyMemory(irp_param.buffer, request->data, request->lastSize);
            else					m_last_irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        CompleteIrp(m_last_irp, m_last_irp->IoStatus.Status, request->lastSize);
		m_last_irp = NULL;
    }

	// 处理本次请求
    ASSERT(m_last_irp == NULL);
	response->type = DISK_OP_EMPTY;
	m_irp_queue.pop(m_last_irp);
	ASSERT(m_last_irp);

    IrpParam irp_param;
	GetIrpParam(m_last_irp, &irp_param);
	KdPrint(("pop irp, type=%d\n", irp_param.type));

    response->type = (UINT32)(irp_param.type);
	response->size = irp_param.size;
    response->offset = irp_param.offset;
    ASSERT(response->type != DISK_OP_EMPTY);

	if (irp_param.type == DISK_OP_DISCONNECT) m_last_irp = NULL;
	else if( irp_param.type == DISK_OP_WRITE )
    {
		KdPrint(("current irp is write cmd\n"));
		if(irp_param.buffer) RtlCopyMemory(request->data, irp_param.buffer, response->size);
        else 
        {
            CompleteIrp(m_last_irp, STATUS_INSUFFICIENT_RESOURCES, 0);
			m_last_irp = NULL;
            response->type = (UINT32)(DISK_OP_EMPTY);
        }
    }

	return STATUS_SUCCESS;
}

NTSTATUS CMountedDisk::Disconnect(void)
{
	m_irp_queue.push( (PIRP) DIRP_DISCONNECT);
	return STATUS_SUCCESS;
}

void CMountedDisk::SetDeviceName(PUNICODE_STRING dev_name, PUNICODE_STRING symbo)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");

	//KdPrint(("source length = %d\n", dev_name->Length));
	//KdPrint(("source max lne = %d\n", dev_name->MaximumLength));

	UNICODE_STRING _dev_name;
	RtlInitUnicodeString(&_dev_name, m_device_name);
	//KdPrint(("unicode length = %d\n", _dev_name.Length));
	//KdPrint(("unicode max len= %d\n", _dev_name.MaximumLength));
	_dev_name.MaximumLength = MAXIMUM_FILENAME_LENGTH -1;
	RtlCopyUnicodeString(&_dev_name, dev_name);
	//KdPrint(("device name input:%wZ\n", dev_name));
	//KdPrint(("device name copy: %wZ\n", _dev_name));
	KdPrint(("device name saved:%S\n", m_device_name));

	UNICODE_STRING _symbo_;
	RtlInitUnicodeString(&_symbo_, m_symbo_link);
	_symbo_.MaximumLength = MAXIMUM_FILENAME_LENGTH -1;
	RtlCopyUnicodeString(&_symbo_, symbo);
	KdPrint(("symbo link set:%S\n", m_symbo_link));
}

///////////////////////////////////////////////////////////////////////////////
// -- CIrpQueue
#pragma LOCKEDCODE

CIrpQueue::CIrpQueue(void)
{
	KdPrint(("[TRACE] CIrpQueue::CIrpQueue\n"));
	RtlFillMemory(m_queue, sizeof(PIRP) * MAX_IRP_QUEUE, 0);
	m_head = 0, m_tail = 0;
	m_size = 0;

//#ifdef ENABLE_SYNC
	KeInitializeEvent(&m_event, SynchronizationEvent, FALSE);
	KeInitializeSpinLock(&m_size_lock);
	ExInitializeFastMutex(&m_mutex);
//#endif
}

void CIrpQueue::Initialize(void)
{
	LOG_STACK_TRACE("");
	RtlFillMemory(m_queue, sizeof(PIRP) * MAX_IRP_QUEUE, 0);
	m_head = 0, m_tail = 0;
	m_size = 0;

//#ifdef ENABLE_SYNC
	KeInitializeEvent(&m_event, SynchronizationEvent, FALSE);
	KeInitializeSpinLock(&m_size_lock);
	ExInitializeFastMutex(&m_mutex);
//#endif
}
CIrpQueue::~CIrpQueue(void)
{
}

bool CIrpQueue::IncreaseIfNotFull(void)
{
	KIRQL	old_irq;
	bool full = true;
	KeAcquireSpinLock(&m_size_lock, &old_irq);
	if (m_size < MAX_IRP_QUEUE)
	{
		m_size ++;
		full = false;
	}
	KeReleaseSpinLock(&m_size_lock, old_irq);
	return full;
}

bool CIrpQueue::DecreaseIfNotEmpty(void)
{
	KIRQL old_irq;
	bool _empty = true;
	KeAcquireSpinLock(&m_size_lock, &old_irq);
	if (m_size > 0)
	{
		m_size --;
		_empty = false;
	}
	KeReleaseSpinLock(&m_size_lock, old_irq);
	return _empty;
}

bool CIrpQueue::empty(void)
{
	KIRQL old_irq;
	bool _empty = true;
	KeAcquireSpinLock(&m_size_lock, &old_irq);
	_empty = (m_size == 0);
	KeReleaseSpinLock(&m_size_lock, old_irq);
	return _empty;
}

bool CIrpQueue::push(IN PIRP irp)
{
//#ifdef ENABLE_SYNC
	while ( IncreaseIfNotFull() )
	{
		KdPrint(("queue::push, queue is full\n"));
		KeWaitForSingleObject(&m_event, Executive, KernelMode, FALSE, NULL);
	}

	ExAcquireFastMutex(&m_mutex);
//#else
//	while (m_size >= MAX_IRP_QUEUE)
//	{
//		LARGE_INTEGER my_interval;
//		my_interval.QuadPart = -10*1000 * 100;
//		KeDelayExecutionThread(KernelMode,0,&my_interval);
//	}
//	m_size ++;
//#endif
	m_queue[m_tail] = irp;
	m_tail ++;
	if (m_tail >= MAX_IRP_QUEUE) m_tail = 0;

//#ifdef ENABLE_SYNC
	ExReleaseFastMutex(&m_mutex);
	KeSetEvent(&m_event, 0, FALSE);
//#endif

	return true;
}

bool CIrpQueue::pop(OUT PIRP &irp)
{
//#ifdef ENABLE_SYNC
	while (DecreaseIfNotEmpty() )
	{
		KdPrint(("queue::pop, queue is empty, waiting for irp\n"));
		KeWaitForSingleObject(&m_event, Executive, KernelMode, FALSE, NULL);
	}
	ExAcquireFastMutex(&m_mutex);
//#else
//	while (m_size <= 0)
//	{
//		LARGE_INTEGER my_interval;
//		my_interval.QuadPart = -10*1000 * 100;
//		KeDelayExecutionThread(KernelMode,0,&my_interval);
//	}
//	m_size --;
//#endif
	irp = m_queue[m_head];
	m_head ++;
	if (m_head >= MAX_IRP_QUEUE)	m_head = 0;

//#ifdef ENABLE_SYNC
	ExReleaseFastMutex(&m_mutex);
	KeSetEvent(&m_event, 0, FALSE);
//#endif
	return true;
}
