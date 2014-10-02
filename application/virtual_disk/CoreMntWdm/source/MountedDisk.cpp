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
	LOG_STACK_TRACE("");
	
	PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(irp);

    param->offset = 0;
    param->type = directOperationEmpty;
    param->buffer = (char*)GetIrpBuffer(irp);

    if (ioStack->MajorFunction == IRP_MJ_READ)
    {
		KdPrint(("major: IRP_MJ_READ\n"));
        param->type = directOperationRead;
        param->size = ioStack->Parameters.Read.Length;
        param->offset = ioStack->Parameters.Read.ByteOffset.QuadPart; 
    }
	else if(ioStack->MajorFunction == IRP_MJ_WRITE)
    {
		KdPrint(("major: IRP_MJ_WRITE\n"));
        param->type = directOperationWrite;
        param->size = ioStack->Parameters.Write.Length;
        param->offset = ioStack->Parameters.Write.ByteOffset.QuadPart; 
    }
    return;    
}

//MountedDisk::MountedDisk(PDRIVER_OBJECT	DriverObject, 
//                         MountManager* mountManager, 
//                         int devId, 
//                         UINT64 totalLength)
//              : irpQueue_(irpQueueNotEmpty_), 
//                m_last_irp(0), 
//                irpDispatcher_(devId, totalLength, DriverObject, mountManager)
//{
//}

//MountedDisk::~MountedDisk()
//{
//    stopEvent_.set();
//    //complete all IRP
//    if(m_last_irp)
//        CompleteLastIrp(STATUS_DEVICE_NOT_READY, 0);
//
//    while(irpQueue_.pop(m_last_irp))
//        CompleteLastIrp(STATUS_DEVICE_NOT_READY, 0);
//}

void MountedDisk::Initialize(PDRIVER_OBJECT DriverObject, MountManager* mountManager,
			int devId, UINT64 totalLength)
{
	m_driver_obj = DriverObject;
	m_mnt_manager = mountManager;
	m_dev_id = devId;
	m_total_length = totalLength;
	m_last_irp = NULL;
	
	m_irp_queue.Initialize();
}

void MountedDisk::Release(void)
{

	//while (m_last_irp)
	//{
	//	CompleteLastIrp(STATUS_DEVICE_NOT_READY, 0);
	//	m_irp_queue.pop(m_last_irp);
	//}
}

NTSTATUS MountedDisk::DispatchIrp(IN PIRP irp)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");

    IrpParam irp_param;
	GetIrpParam(irp, &irp_param);

	KdPrint(("operation type: %d\n", irp_param.type));

	NTSTATUS status;
    if(irp_param.type == directOperationEmpty)
    {
		LocalDispatch(irp);
        status = irp->IoStatus.Status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
	else
	{
		IoMarkIrpPending( irp );
		KdPrint(("push irp to queue\n"));
		m_irp_queue.push(irp);
		status = STATUS_PENDING;
	}
	return status;
}

/*
void MountedDisk::GetIrpParam(IN PIRP irp, OUT IrpParam & param)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");
	
	PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(irp);

    param.offset = 0;
    param.type = directOperationEmpty;
    param.buffer = (char*)GetIrpBuffer(irp);

    if (ioStack->MajorFunction == IRP_MJ_READ)
    {
		KdPrint(("major: IRP_MJ_READ\n"));
        param.type = directOperationRead;
        param.size = ioStack->Parameters.Read.Length;
        param.offset = ioStack->Parameters.Read.ByteOffset.QuadPart; 
    }
	else if(ioStack->MajorFunction == IRP_MJ_WRITE)
    {
		KdPrint(("major: IRP_MJ_WRITE\n"));
        param.type = directOperationWrite;
        param.size = ioStack->Parameters.Write.Length;
        param.offset = ioStack->Parameters.Write.ByteOffset.QuadPart; 
    }
    return;    
}
*/

// 处理简单的irp，在驱动内处理完并且Complete
void MountedDisk::LocalDispatch(IN PIRP irp)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");

    PIO_STACK_LOCATION io_stack = IoGetCurrentIrpStackLocation(irp);
    switch(io_stack->MajorFunction)
    {
    case IRP_MJ_CREATE:
    case IRP_MJ_CLOSE:
		KdPrint( ("major: IRP_MJ_CREATE / IRP_MJ_CLOSE\n") );
        irp->IoStatus.Status = STATUS_SUCCESS;
        irp->IoStatus.Information = 0;
        break;

    case IRP_MJ_QUERY_VOLUME_INFORMATION:
        KdPrint( ("major: IRP_MJ_QUERY_VOLUME_INFORMATION\n") );
        irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        irp->IoStatus.Information = 0;
        break;

	case IRP_MJ_DEVICE_CONTROL: {
		ULONG code = io_stack->Parameters.DeviceIoControl.IoControlCode;
		KdPrint( ("major: IRP_MJ_DEVICE_CONTROL, minor: 0x%X\n", code) );
		switch (code)
		{
		case IOCTL_DISK_GET_DRIVE_LAYOUT:
			KdPrint( ("minor: IOCTL_DISK_GET_DRIVE_LAYOUT\n") );
			if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof (DRIVE_LAYOUT_INFORMATION))
			{
				irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
				irp->IoStatus.Information = 0;
			}
			else
			{
				PDRIVE_LAYOUT_INFORMATION outputBuffer = (PDRIVE_LAYOUT_INFORMATION)
					irp->AssociatedIrp.SystemBuffer;

				outputBuffer->PartitionCount = 1;
				outputBuffer->Signature = 0;

				outputBuffer->PartitionEntry->PartitionType = PARTITION_ENTRY_UNUSED;
				outputBuffer->PartitionEntry->BootIndicator = FALSE;
				outputBuffer->PartitionEntry->RecognizedPartition = TRUE;
				outputBuffer->PartitionEntry->RewritePartition = FALSE;
				outputBuffer->PartitionEntry->StartingOffset = RtlConvertUlongToLargeInteger (0);
				outputBuffer->PartitionEntry->PartitionLength.QuadPart= m_total_length;
				outputBuffer->PartitionEntry->HiddenSectors = 1L;

				irp->IoStatus.Status = STATUS_SUCCESS;
				irp->IoStatus.Information = sizeof (PARTITION_INFORMATION);
			}
			break;

		case IOCTL_DISK_CHECK_VERIFY:
		case IOCTL_CDROM_CHECK_VERIFY:
		case IOCTL_STORAGE_CHECK_VERIFY:
		case IOCTL_STORAGE_CHECK_VERIFY2:	{
			KdPrint( ("minor: IOCTL_xxx_CHECK_VERIFY\n") );
			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = 0;
			break;							}

		case IOCTL_DISK_GET_DRIVE_GEOMETRY:
		case IOCTL_CDROM_GET_DRIVE_GEOMETRY:		{
			KdPrint( ("minor: IOCTL_xxx_GET_DRIVE_GEOMETRY\n") );

			PDISK_GEOMETRY  disk_geometry;
			ULONGLONG       length;

			if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(DISK_GEOMETRY))
			{
				irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
				irp->IoStatus.Information = 0;
				break;
			}
			disk_geometry = (PDISK_GEOMETRY) irp->AssociatedIrp.SystemBuffer;
			length = m_total_length;
			disk_geometry->Cylinders.QuadPart = length / SECTOR_SIZE / 0x20 / 0x80;
			disk_geometry->MediaType = FixedMedia;
			disk_geometry->TracksPerCylinder = 0x80;
			disk_geometry->SectorsPerTrack = 0x20;
			disk_geometry->BytesPerSector = SECTOR_SIZE;
			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
			break;								}

		case IOCTL_DISK_GET_LENGTH_INFO:		{
			KdPrint( ("minor: IOCTL_DISK_GET_LENGTH_INFO\n") );
			PGET_LENGTH_INFORMATION get_length_information;
			if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(GET_LENGTH_INFORMATION))
			{
				irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
				irp->IoStatus.Information = 0;
				break;
			}
			get_length_information = (PGET_LENGTH_INFORMATION) irp->AssociatedIrp.SystemBuffer;
			get_length_information->Length.QuadPart = m_total_length;
			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = sizeof(GET_LENGTH_INFORMATION);
			break;								}

		case IOCTL_DISK_GET_PARTITION_INFO:		{
			KdPrint( ("minor: IOCTL_DISK_GET_PARTITION_INFO\n") );

			PPARTITION_INFORMATION  partition_information;
			ULONGLONG               length;
			if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(PARTITION_INFORMATION))
			{
				irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
				irp->IoStatus.Information = 0;
				break;
			}
			partition_information = (PPARTITION_INFORMATION) irp->AssociatedIrp.SystemBuffer;
			length = m_total_length;
			partition_information->StartingOffset.QuadPart = 0;
			partition_information->PartitionLength.QuadPart = length;
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
			KdPrint( ("minor: IOCTL_DISK_GET_PARTITION_INFO_EX\n") );

			PPARTITION_INFORMATION_EX   partition_information_ex;
			ULONGLONG                   length;
			if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(PARTITION_INFORMATION_EX))
			{
				irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
				irp->IoStatus.Information = 0;
				break;
			}
			partition_information_ex = (PPARTITION_INFORMATION_EX) irp->AssociatedIrp.SystemBuffer;
			length = m_total_length;
			partition_information_ex->PartitionStyle = PARTITION_STYLE_MBR;
			partition_information_ex->StartingOffset.QuadPart = 0;
			partition_information_ex->PartitionLength.QuadPart = length;
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
			KdPrint( ("minor: IOCTL_DISK_IS_WRITABLE\n") );
			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = 0;
			break;								}

		case IOCTL_DISK_MEDIA_REMOVAL:
		case IOCTL_STORAGE_MEDIA_REMOVAL:		{
			KdPrint( ("minor: IOCTL_xxx_MEDIA_REMOVAL\n") );
			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = 0;
			break;								}

		case IOCTL_CDROM_READ_TOC:				{
			KdPrint( ("minor: IOCTL_CDROM_READ_TOC\n") );
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
			KdPrint( ("minor: IOCTL_DISK_SET_PARTITION_INFO\n") );
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
			KdPrint( ("minor: IOCTL_DISK_VERIFY\n") );
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
			KdPrint( ("minor: IOCTL_MOUNTDEV_QUERY_DEVICE_NAME\n") );
			if(io_stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof (MOUNTDEV_NAME))
			{
				irp->IoStatus.Information = sizeof (MOUNTDEV_NAME);
				irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
			}
			else
			{
				PMOUNTDEV_NAME devName = (PMOUNTDEV_NAME)irp->AssociatedIrp.SystemBuffer;

				//WCHAR device_name_buffer[MAXIMUM_FILENAME_LENGTH];
				//swprintf(device_name_buffer, DIRECT_DISK_PREFIX L"%u", devId_);

				UNICODE_STRING deviceName;
				RtlInitUnicodeString(&deviceName, m_device_name);

				devName->NameLength = deviceName.Length;
				int outLength = sizeof(USHORT) + deviceName.Length;
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
			KdPrint( ("minor: IOCTL_MOUNTDEV_QUERY_UNIQUE_ID\n") );
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
				//WCHAR unique_id_buffer[MAXIMUM_FILENAME_LENGTH];
				//swprintf(unique_id_buffer, DIRECT_DISK_PREFIX L"%u", devId_);
	            
				UNICODE_STRING uniqueId;
				RtlInitUnicodeString(&uniqueId, m_device_name);
				unique_id_length = uniqueId.Length;
	            
				mountDevId->UniqueIdLength = uniqueId.Length;
				int outLength = sizeof(USHORT) + uniqueId.Length;
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
			KdPrint( ("minor: IOCTL_MOUNTDEV_QUERY_STABLE_GUID\n") );
			irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
			irp->IoStatus.Information = 0;
			break;	}


		case IOCTL_STORAGE_GET_HOTPLUG_INFO:			{
			KdPrint( ("minor: IOCTL_STORAGE_GET_HOTPLUG_INFO\n") );
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
			KdPrint( ("code: Unknown PNP minor function= 0x%x\n", io_stack->MinorFunction));
		}
		break; }

    default:
		KdPrint( ("major: Unknown MJ fnc = 0x%x\n", io_stack->MajorFunction));
     }
}

void MountedDisk::CompleteLastIrp(NTSTATUS status, ULONG information)
{
    ASSERT(m_last_irp);
    //irpDispatcher_.onCompleteIrp(m_last_irp, status, information);
    m_last_irp->IoStatus.Status = status;
    m_last_irp->IoStatus.Information = information;
    IoCompleteRequest( m_last_irp, IO_NO_INCREMENT);
    m_last_irp = NULL;
}




NTSTATUS MountedDisk::RequestExchange(IN CORE_MNT_EXCHANGE_REQUEST * request, OUT CORE_MNT_EXCHANGE_RESPONSE * response)
                     //UINT32 * type, UINT32 * length, UINT64 * offset)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");

	DISK_OPERATION_TYPE last_type = (DISK_OPERATION_TYPE)(request->lastType);

	// 返回上次处理结果
    if(last_type != directOperationEmpty)
    {
		KdPrint(("processing last irp\n"));
        ASSERT(m_last_irp);
        m_last_irp->IoStatus.Status = request->lastStatus;
        if(request->lastStatus == STATUS_SUCCESS && last_type == directOperationRead)
        {
			KdPrint(("last irp is read cmd\n"));
            IrpParam irp_param;
			GetIrpParam(m_last_irp, &irp_param);
            if(irp_param.buffer)	RtlCopyMemory(irp_param.buffer, request->data, request->lastSize);
            else					m_last_irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        CompleteLastIrp(m_last_irp->IoStatus.Status, request->lastSize);
    }

	// 处理本次请求
    ASSERT(!m_last_irp);
	response->type = directOperationEmpty;
	
	//PIRP irp;
	m_irp_queue.pop(m_last_irp);
	ASSERT(m_last_irp);

    IrpParam irp_param;
	GetIrpParam(m_last_irp, &irp_param);
	KdPrint(("pop irp, type=%d\n", irp_param.type));

    response->type = (UINT32)(irp_param.type);
	response->size = irp_param.size;
    response->offset = irp_param.offset;
    ASSERT(response->type != directOperationEmpty);

    if((DISK_OPERATION_TYPE)(response->type) == directOperationWrite)
    {
		KdPrint(("current irp is write cmd\n"));
        IrpParam irp_param;
		GetIrpParam(m_last_irp, &irp_param);

		if(irp_param.buffer) RtlCopyMemory(request->data, irp_param.buffer, response->size);
        else 
        {
            CompleteLastIrp(STATUS_INSUFFICIENT_RESOURCES, 0);
            response->type = (UINT32)(directOperationEmpty);
        }
    }

	return STATUS_SUCCESS;
}

void MountedDisk::SetDeviceName(const WCHAR *dev_name)
{
	int jj=0;
	while ( dev_name[jj] !=0 )
	{
		m_device_name[jj] = dev_name[jj];
		++jj;
	}
	m_device_name[jj] = 0;
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

	//
	//KeInitializeSemaphore(&m_semaph, 0, MAX_IRP_QUEUE);
	//KeInitializeMutex(&m_mutex, 0);
#ifdef ENABLE_SYNC
	KeInitializeEvent(&m_event, SynchronizationEvent, FALSE);
	KeInitializeSpinLock(&m_size_lock);
	ExInitializeFastMutex(&m_mutex);
#endif
}

void CIrpQueue::Initialize(void)
{
	LOG_STACK_TRACE("");
	RtlFillMemory(m_queue, sizeof(PIRP) * MAX_IRP_QUEUE, 0);
	m_head = 0, m_tail = 0;
	m_size = 0;

	//
	//KeInitializeSemaphore(&m_semaph, 0, MAX_IRP_QUEUE);
	//KeInitializeMutex(&m_mutex, 0);
#ifdef ENABLE_SYNC
	KeInitializeEvent(&m_event, SynchronizationEvent, FALSE);
	KeInitializeSpinLock(&m_size_lock);
	ExInitializeFastMutex(&m_mutex);
#endif
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
	bool empty = true;
	KeAcquireSpinLock(&m_size_lock, &old_irq);
	if (m_size > 0)
	{
		m_size --;
		empty = false;
	}
	KeReleaseSpinLock(&m_size_lock, old_irq);
	return empty;
}

bool CIrpQueue::push(IN PIRP irp)
{
#ifdef ENABLE_SYNC
	while ( IncreaseIfNotFull() )
	{
		KdPrint(("queue::push, queue is full\n"));
		KeWaitForSingleObject(&m_event, Executive, KernelMode, FALSE, NULL);
	}

	//<TODO> need mutex
	ExAcquireFastMutex(&m_mutex);
#else
	while (m_size >= MAX_IRP_QUEUE)
	{
		LARGE_INTEGER my_interval;
		my_interval.QuadPart = -10*1000 * 100;
		KeDelayExecutionThread(KernelMode,0,&my_interval);
	}
	//if (m_size >= MAX_IRP_QUEUE) return false;
	m_size ++;
#endif
	m_queue[m_tail] = irp;
	m_tail ++;
	if (m_tail >= MAX_IRP_QUEUE) m_tail = 0;

#ifdef ENABLE_SYNC
	ExReleaseFastMutex(&m_mutex);
	// end mutex
	KeSetEvent(&m_event, 0, FALSE);
#endif

	return true;
}

bool CIrpQueue::pop(OUT PIRP &irp)
{
#ifdef ENABLE_SYNC
	while (DecreaseIfNotEmpty() )
	{
		KdPrint(("queue::pop, queue is empty, waiting for irp\n"));
		KeWaitForSingleObject(&m_event, Executive, KernelMode, FALSE, NULL);
	}
	//<TODO> need mutex
	ExAcquireFastMutex(&m_mutex);
#else
	while (m_size <= 0)
	{
		LARGE_INTEGER my_interval;
		my_interval.QuadPart = -10*1000 * 100;
		KeDelayExecutionThread(KernelMode,0,&my_interval);
	}
	//if (m_size <= 0) return false;
	m_size --;
#endif
	irp = m_queue[m_head];
	m_head ++;
	if (m_head >= MAX_IRP_QUEUE)	m_head = 0;

#ifdef ENABLE_SYNC
	ExReleaseFastMutex(&m_mutex);
	// end mutex
	KeSetEvent(&m_event, 0, FALSE);
#endif
	return true;
}
