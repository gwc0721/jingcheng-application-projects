#include "MountedDisk.h"

#define LOGGER_LEVEL	LOGGER_LEVEL_DEBUGINFO
#include "jclogk.h"

//#ifdef _AMD64_
//#define _WIN64
//#endif

extern "C"
{
#include "ntdddisk.h"
#include "ntddcdrm.h"
#include "mountmgr.h"
#include "mountdev.h"
}

#ifdef _AMD64_
#define SCSI_PASS_THROUGH_DIRECT_T SCSI_PASS_THROUGH_DIRECT32
#define SCSI_PASS_THROUGH_T SCSI_PASS_THROUGH32
#else
#define SCSI_PASS_THROUGH_DIRECT_T SCSI_PASS_THROUGH_DIRECT
#define SCSI_PASS_THROUGH_T SCSI_PASS_THROUGH
#endif

///////////////////////////////////////////////////////////////////////////////
// -- CIrpQueue
#pragma LOCKEDCODE

template <typename ELEMENT>
CIrpQueue<ELEMENT>::CIrpQueue(void)
{
	KdPrint(("[TRACE] CIrpQueue::CIrpQueue\n"));
	RtlFillMemory(m_queue, sizeof(ELEMENT) * MAX_IRP_QUEUE, 0);
	m_head = 0, m_tail = 0;
	m_size = 0;

	KeInitializeEvent(&m_event, SynchronizationEvent, FALSE);
	KeInitializeSpinLock(&m_size_lock);
	ExInitializeFastMutex(&m_mutex);
}

template <typename ELEMENT>
void CIrpQueue<ELEMENT>::Initialize(void)
{
	LOG_STACK_TRACE("");
	RtlFillMemory(m_queue, sizeof(ELEMENT) * MAX_IRP_QUEUE, 0);
	m_head = 0, m_tail = 0;
	m_size = 0;

	KeInitializeEvent(&m_event, SynchronizationEvent, FALSE);
	KeInitializeSpinLock(&m_size_lock);
	ExInitializeFastMutex(&m_mutex);
}

template <typename ELEMENT>
CIrpQueue<ELEMENT>::~CIrpQueue(void)
{
}

template <typename ELEMENT>
bool CIrpQueue<ELEMENT>::IncreaseIfNotFull(void)
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

template <typename ELEMENT>
bool CIrpQueue<ELEMENT>::DecreaseIfNotEmpty(void)
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

template <typename ELEMENT>
bool CIrpQueue<ELEMENT>::empty(void)
{
	KIRQL old_irq;
	bool _empty = true;
	KeAcquireSpinLock(&m_size_lock, &old_irq);
	_empty = (m_size == 0);
	KeReleaseSpinLock(&m_size_lock, old_irq);
	return _empty;
}

template <typename ELEMENT>
bool CIrpQueue<ELEMENT>::push(IN ELEMENT irp)
{
	while ( IncreaseIfNotFull() )
	{
		KdPrint(("queue::push, queue is full\n"));
		KeWaitForSingleObject(&m_event, Executive, KernelMode, FALSE, NULL);
	}

	ExAcquireFastMutex(&m_mutex);
	m_queue[m_tail] = irp;
	m_tail ++;
	if (m_tail >= MAX_IRP_QUEUE) m_tail = 0;

	ExReleaseFastMutex(&m_mutex);
	KeSetEvent(&m_event, 0, FALSE);
	return true;
}

template <typename ELEMENT>
bool CIrpQueue<ELEMENT>::pop(OUT ELEMENT &irp)
{
	while (DecreaseIfNotEmpty() )
	{
		KdPrint(("queue::pop, queue is empty, waiting for irp\n"));
		KeWaitForSingleObject(&m_event, Executive, KernelMode, FALSE, NULL);
	}
	ExAcquireFastMutex(&m_mutex);
	irp = m_queue[m_head];
	m_head ++;
	if (m_head >= MAX_IRP_QUEUE)	m_head = 0;

	ExReleaseFastMutex(&m_mutex);
	KeSetEvent(&m_event, 0, FALSE);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// -- CMountedDisk

// dummy irp for internal operation
#define DIRP_DISCONNECT		1

#pragma PAGEDCODE

void LogProcessName(void)
{
	PEPROCESS ps = PsGetCurrentProcess();
	char * ps_name = (char *)((ULONG)ps + 0x16C);
	KdPrint(("[PROC] %s\n", ps_name));
}

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
	CompleteLastIrp(m_last_irp);
	m_last_irp = NULL;

	while (! m_irp_queue.empty() )
	{
		IRP_EXCHANGE_REQUEST *ier;
		m_irp_queue.pop(ier);
		CompleteLastIrp(ier);
	}
	UNICODE_STRING _symbo_;
	RtlInitUnicodeString(&_symbo_, m_symbo_link);
	IoDeleteSymbolicLink(&_symbo_);
}

void CMountedDisk::CompleteLastIrp(IRP_EXCHANGE_REQUEST * ier)
{
	LOG_STACK_TRACE("");
	if (!ier) return;
	if (ier->m_complete)
	{	// 由exchange负责完成IRP
		KdPrint(("complete irp\n"));
		if ( ier->m_irp )	CompleteIrp(ier->m_irp, ier->m_status, ier->m_data_len);
		ExFreePool(ier);
	}
	else
	{	// 由dispatch负责完成IRP
		KdPrint(("set event of irp\n"));
		KeSetEvent(&ier->m_event, 0, FALSE);
	}
}

void CMountedDisk::SynchExchangeInitBuf(IN PIRP irp, IN UCHAR mj, IN ULONG mi, IN READ_WRITE rw, IN ULONG buf_len,
			IN ULONG64 offset, OUT IRP_EXCHANGE_REQUEST & ier)
{
	ier.m_irp = irp;
	ier.m_major_func = mj;
	ier.m_minor_code = mi;
	ier.m_read_write = rw;
	ier.m_data_len = buf_len;
	ier.m_offset = offset;
	ier.m_complete = false;
	ier.m_kernel_buf = (UCHAR*)ExAllocatePoolWithTag(PagedPool, ier.m_data_len, 0);
	KeInitializeEvent(&(ier.m_event), SynchronizationEvent, FALSE);
}

NTSTATUS CMountedDisk::SynchExchange(IN IRP_EXCHANGE_REQUEST & ier)
{
	LOG_STACK_TRACE("");
	m_irp_queue.push(&ier);
	KeWaitForSingleObject(&(ier.m_event), Executive, KernelMode, FALSE, NULL);
	return ier.m_status;
}

void CMountedDisk::SynchExchangeClean(IN IRP_EXCHANGE_REQUEST & ier, IN ULONG data_len)
{
	LOG_STACK_TRACE("");
	ExFreePool(ier.m_kernel_buf);
	ier.m_irp->IoStatus.Status = ier.m_status;
	ier.m_irp->IoStatus.Information = data_len;
}

void CMountedDisk::AsyncExchange(IN PIRP irp, IN UCHAR mj, IN ULONG mi, IN READ_WRITE rw, IN ULONG buf_size,
			IN ULONG64 offset, IN UCHAR* buf)
{
	IRP_EXCHANGE_REQUEST *ier = (IRP_EXCHANGE_REQUEST *)ExAllocatePool(PagedPool, sizeof(IRP_EXCHANGE_REQUEST));
	ier->m_irp = irp;
	ier->m_major_func = mj;
	ier->m_minor_code = mi;
	ier->m_read_write = rw;
	ier->m_kernel_buf = buf;
	ier->m_data_len = buf_size;
	ier->m_offset = offset;
	ier->m_complete = true;
	if (irp)	IoMarkIrpPending( irp );
	m_irp_queue.push(ier);
}

// 处理简单的irp，在驱动内处理完并且Complete
NTSTATUS CMountedDisk::DispatchIrp(IN PIRP irp)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");

    PIO_STACK_LOCATION io_stack = IoGetCurrentIrpStackLocation(irp);

	// default result;
    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;

    switch(io_stack->MajorFunction)
    {
    case IRP_MJ_CREATE:
		KdPrint( ("[IRP] disk <- IRP_MJ_CREATE\n") );
		break;

    case IRP_MJ_CLOSE:
		KdPrint( ("[IRP] disk <- IRP_MJ_CLOSE\n") );
        break;

	case IRP_MJ_CLEANUP:
		KdPrint( ("[IRP] disk <- IRP_MJ_CLEANUP\n") );
        break;

	case IRP_MJ_READ: {
		KdPrint(("[IRP] disk <- IRP_MJ_READ\n"));
		AsyncExchange(irp, IRP_MJ_READ, 0, READ, io_stack->Parameters.Read.Length, 
			io_stack->Parameters.Read.ByteOffset.QuadPart, 
			(UCHAR*)MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority));
		return STATUS_PENDING;
		break;}

	case IRP_MJ_WRITE:
		KdPrint(("[IRP] disk <- IRP_MJ_WRITE\n"));
		AsyncExchange(irp, IRP_MJ_WRITE, 0, WRITE, io_stack->Parameters.Write.Length, 
			io_stack->Parameters.Write.ByteOffset.QuadPart, 
			(UCHAR*)MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority));
		return STATUS_PENDING;
		break;

	case IRP_MJ_FLUSH_BUFFERS:
		KdPrint(("[IRP] disk <- IRP_MJ_FLUSH_BUFFERS\n"));
		break;

    case IRP_MJ_QUERY_VOLUME_INFORMATION:
        KdPrint( ("[IRP] disk <- IRP_MJ_QUERY_VOLUME_INFORMATION\n") );
        irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        irp->IoStatus.Information = 0;
        break;

	case IRP_MJ_PNP:	{
		switch (io_stack->MinorFunction)
		{
		case IRP_MN_QUERY_DEVICE_RELATIONS: {
			DEVICE_RELATION_TYPE type = io_stack->Parameters.QueryDeviceRelations.Type;
			KdPrint(("DEVICE_RELATION_TYPE : %d\n", type));
			break;							}
		}
		break;			}
		

	case IRP_MJ_DEVICE_CONTROL: 
		LocalDispatchIoCtrl(irp);
		break;

    default:
		KdPrint( ("[IRP] disk <- UNKNOW_MAJOR_FUNC(0x%08X)\n", io_stack->MajorFunction));
	}
	NTSTATUS status = irp->IoStatus.Status;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}


bool CMountedDisk::LocalDispatchIoCtrl(IN PIRP irp)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");

    PIO_STACK_LOCATION io_stack = IoGetCurrentIrpStackLocation(irp);
	ULONG code = io_stack->Parameters.DeviceIoControl.IoControlCode;
	switch (code)
	{
	case IOCTL_SCSI_PASS_THROUGH_DIRECT: {
		KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_SCSI_PASS_THROUGH_DIRECT\n") );
		KdPrint(("sizeof(SCSI_PASS_THROUGH_DIRECT)=%d\n",sizeof(SCSI_PASS_THROUGH_DIRECT_T)));
		SCSI_PASS_THROUGH_DIRECT_T * sptd = reinterpret_cast<SCSI_PASS_THROUGH_DIRECT_T*>(
				irp->AssociatedIrp.SystemBuffer);

		ULONG32 spt_len = sizeof(SCSI_PASS_THROUGH_DIRECT_T);
		UCHAR sense_len = sptd->SenseInfoLength;
		ULONG sense_offset = sptd->SenseInfoOffset;

		ULONG data_len = sptd->DataTransferLength;
		PVOID data_buf = sptd->DataBuffer;
		KdPrint(("data_buf=0x%016I64X\n",data_buf));

		IRP_EXCHANGE_REQUEST ier;
		SynchExchangeInitBuf(irp, IRP_MJ_DEVICE_CONTROL, IOCTL_SCSI_PASS_THROUGH_DIRECT, READ_AND_WRITE, 
			spt_len + sense_len + data_len, 0, ier);

		UCHAR * buf = ier.m_kernel_buf;
		// copy header
		RtlCopyMemory(buf, sptd, spt_len);	buf+= spt_len;
		// copy sense info
		RtlCopyMemory(buf, sptd + sense_offset, sense_len);	buf+=sense_len;
		if (sptd->DataIn == SCSI_IOCTL_DATA_OUT) 
		{
			KdPrint(("copy data for write, len = %d\n", data_len));
			// copy input data
			RtlCopyMemory(buf, data_buf, data_len);
		}
		NTSTATUS st = SynchExchange(ier);
		ULONG out_len = 0;
		if (st == STATUS_SUCCESS)
		{
			// 返回data
			buf = ier.m_kernel_buf;
			RtlCopyMemory(irp->AssociatedIrp.SystemBuffer, buf, spt_len); buf+= spt_len;
			RtlCopyMemory(sptd + sense_offset, buf, sense_len);	buf += sense_len;
			out_len = spt_len + sense_len;

			if (sptd->DataIn == SCSI_IOCTL_DATA_IN)	// read
			{
				KdPrint(("copy data for read, len = %d\n", data_len));
				RtlCopyMemory(data_buf, buf, data_len);
			}
		}
		SynchExchangeClean(ier, out_len);
		break;									 }

	case IOCTL_SCSI_PASS_THROUGH: {
		KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_SCSI_PASS_THROUGH\n") );
		KdPrint(("sizeof(SCSI_PASS_THROUGH)=%d\n",sizeof(SCSI_PASS_THROUGH_DIRECT_T)));
		SCSI_PASS_THROUGH_T * sptd = reinterpret_cast<SCSI_PASS_THROUGH_T*>(
				irp->AssociatedIrp.SystemBuffer);

		ULONG32 spt_len = sizeof(SCSI_PASS_THROUGH_T);
		UCHAR sense_len = sptd->SenseInfoLength;
		ULONG sense_offset = sptd->SenseInfoOffset;

		ULONG data_len = sptd->DataTransferLength;
		ULONG32 data_offset = sptd->DataBufferOffset;
		ULONG32 buf_size = max(io_stack->Parameters.DeviceIoControl.InputBufferLength,
			io_stack->Parameters.DeviceIoControl.OutputBufferLength);

		IRP_EXCHANGE_REQUEST ier;
		SynchExchangeInitBuf(irp, IRP_MJ_DEVICE_CONTROL, IOCTL_SCSI_PASS_THROUGH, READ_AND_WRITE, 
			buf_size, 0, ier);

		UCHAR * buf = ier.m_kernel_buf;
		RtlCopyMemory(buf, sptd, io_stack->Parameters.DeviceIoControl.InputBufferLength);
		NTSTATUS st = SynchExchange(ier);
		ULONG out_len = 0;
		if (st == STATUS_SUCCESS)
		{	// 返回data
			buf = ier.m_kernel_buf;
			out_len = io_stack->Parameters.DeviceIoControl.OutputBufferLength;
			RtlCopyMemory(irp->AssociatedIrp.SystemBuffer, buf, out_len);
		}
		SynchExchangeClean(ier, out_len);
		break;									 }

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
		outputBuffer->PartitionEntry->PartitionLength.QuadPart= SECTOR_TO_BYTE(m_total_sectors);
		outputBuffer->PartitionEntry->HiddenSectors = 1L;

		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = sizeof (PARTITION_INFORMATION);
		break;		}

	case IOCTL_DISK_CHECK_VERIFY:
	case IOCTL_CDROM_CHECK_VERIFY:
	case IOCTL_STORAGE_CHECK_VERIFY:
	case IOCTL_STORAGE_CHECK_VERIFY2:	{
		KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_xxx_CHECK_VERIFY\n") );
		break;							}

	case IOCTL_DISK_GET_DRIVE_GEOMETRY:
	case IOCTL_CDROM_GET_DRIVE_GEOMETRY:		{
		KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_xxx_GET_DRIVE_GEOMETRY\n") );
		PDISK_GEOMETRY  disk_geometry;
		if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
			sizeof(DISK_GEOMETRY))
		{
			irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			irp->IoStatus.Information = 0;
			break;
		}
		disk_geometry = (PDISK_GEOMETRY) irp->AssociatedIrp.SystemBuffer;
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
		get_length_information->Length.QuadPart = SECTOR_TO_BYTE(m_total_sectors);	// length in sectors
		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = sizeof(GET_LENGTH_INFORMATION);
		break;								}

	case IOCTL_DISK_GET_PARTITION_INFO:		{
		KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_DISK_GET_PARTITION_INFO\n") );
		PPARTITION_INFORMATION  partition_information;
		if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
			sizeof(PARTITION_INFORMATION))
		{
			irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			irp->IoStatus.Information = 0;
			break;
		}
		partition_information = (PPARTITION_INFORMATION) irp->AssociatedIrp.SystemBuffer;
		partition_information->StartingOffset.QuadPart = 0;
		partition_information->PartitionLength.QuadPart = SECTOR_TO_BYTE(m_total_sectors);
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
		if (io_stack->Parameters.DeviceIoControl.OutputBufferLength <
			sizeof(PARTITION_INFORMATION_EX))
		{
			irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			irp->IoStatus.Information = 0;
			break;
		}
		partition_information_ex = (PPARTITION_INFORMATION_EX) irp->AssociatedIrp.SystemBuffer;
		partition_information_ex->PartitionStyle = PARTITION_STYLE_MBR;
		partition_information_ex->StartingOffset.QuadPart = 0;
		partition_information_ex->PartitionLength.QuadPart = SECTOR_TO_BYTE(m_total_sectors);
		partition_information_ex->PartitionNumber = 0;
		partition_information_ex->RewritePartition = FALSE;
		partition_information_ex->Mbr.PartitionType = 0;
		partition_information_ex->Mbr.BootIndicator = FALSE;
		partition_information_ex->Mbr.RecognizedPartition = TRUE;
		partition_information_ex->Mbr.HiddenSectors = 0;
		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = sizeof(PARTITION_INFORMATION_EX);
		break;								}

	case IOCTL_DISK_IS_WRITABLE:
		KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_DISK_IS_WRITABLE\n") );
		break;

	case IOCTL_DISK_MEDIA_REMOVAL:
	case IOCTL_STORAGE_MEDIA_REMOVAL:
		KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::IOCTL_xxx_MEDIA_REMOVAL\n") );
		break;

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
		KdPrint( ("[IRP] disk <- IRP_MJ_DEVICE_CONTROL::UNKNOW_MINOR_FUNC(0x%08X)\n", code));
	}
	return true;
}


NTSTATUS CMountedDisk::RequestExchange(IN CORE_MNT_EXCHANGE_REQUEST * request, OUT CORE_MNT_EXCHANGE_RESPONSE * response)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");
	LogProcessName();

	//!! m_last_irp对于同一个CMountedDisk对象是线程不安全的。
	//!! 这里User mode utility (LIB)保证对于同一个device，以单线程调用RequestExchange

	// 返回上次处理结果
	if (request->m_major_func != IRP_MJ_NOP)
    {
        ASSERT(m_last_irp);
		KdPrint(("response last irp, func:%d, code:0x%08X\n", m_last_irp->m_major_func, m_last_irp->m_minor_code));
        m_last_irp->m_status = request->lastStatus;
        if( request->m_read_write & READ )	
        {	// 需要返回 data
			KdPrint(("return data to irp, len=%d\n", request->proc_size));
			ASSERT(m_last_irp->m_kernel_buf);
			if (m_last_irp->m_kernel_buf)		RtlCopyMemory(
				(m_last_irp->m_kernel_buf + m_last_irp->m_processed), (PVOID)(request->m_buf), request->proc_size);
        }
		m_last_irp->m_processed += request->proc_size;
		if ( (m_last_irp->m_processed >= m_last_irp->m_data_len)
			|| (m_last_irp->m_read_write == (WRITE | READ) )
			|| (m_last_irp->m_status != STATUS_SUCCESS) )
		{
			CompleteLastIrp(m_last_irp);
			m_last_irp = NULL;
		}
    }

	// 处理本次请求
    //ASSERT(m_last_irp == NULL);
	if ( !m_last_irp )
	{	// 上次的irp已经全部完成，从队列中获取新的irp
		m_irp_queue.pop(m_last_irp);
		ASSERT(m_last_irp);
		m_last_irp->m_processed = 0;
	}

	KdPrint(("send irp, func:%d, code:0x%08X\n", m_last_irp->m_major_func, m_last_irp->m_minor_code));

	response->m_major_func = m_last_irp->m_major_func;
	response->m_minor_code = m_last_irp->m_minor_code;
	response->m_read_write = m_last_irp->m_read_write;

	KdPrint(("user mode buffer size: %dKB\n", request->buf_size / 1024));

	ASSERT(m_last_irp->m_data_len >  m_last_irp->m_processed);
	if ( (m_last_irp->m_read_write == (WRITE | READ) ) && (m_last_irp->m_data_len > request->buf_size) )
	{	// read/write 请求不支持分批处理
		KdPrint(("user mode buffer too small: request:0x%X\n", m_last_irp->m_data_len));
		response->size = m_last_irp->m_data_len;
		// 结束m_last_irp
		m_last_irp->m_status = STATUS_NO_MEMORY;
		CompleteLastIrp(m_last_irp);
		m_last_irp = NULL;
		return STATUS_NO_MEMORY;
	}

	ULONG32 transfer_size = m_last_irp->m_data_len - m_last_irp->m_processed;
	if (transfer_size > request->buf_size)	transfer_size = request->buf_size;
	response->size = transfer_size;

	//response->size = m_last_irp->m_data_len;
	//response->offset = m_last_irp->m_offset;

	response->offset = m_last_irp->m_offset + m_last_irp->m_processed;

	if (m_last_irp->m_read_write & WRITE)
	{
		KdPrint(("copy data, len=%d\n", transfer_size));
		ASSERT(m_last_irp->m_kernel_buf);
		if (m_last_irp->m_kernel_buf)	RtlCopyMemory(
			(PVOID)(request->m_buf), (m_last_irp->m_kernel_buf + m_last_irp->m_processed), transfer_size);
		//m_last_irp->m_processed += transfer_size;
	}
	return STATUS_SUCCESS;
}

NTSTATUS CMountedDisk::Disconnect(void)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");
	LogProcessName();

	AsyncExchange(NULL, IRP_MJ_DISCONNECT, 0, NO_READ_WRITE, 0, 0, NULL);
	return STATUS_SUCCESS;
}

void CMountedDisk::SetDeviceName(PUNICODE_STRING dev_name, PUNICODE_STRING symbo)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");

	UNICODE_STRING _dev_name;
	RtlInitUnicodeString(&_dev_name, m_device_name);
	_dev_name.MaximumLength = MAXIMUM_FILENAME_LENGTH -1;
	RtlCopyUnicodeString(&_dev_name, dev_name);
	KdPrint(("device name saved:%S\n", m_device_name));

	UNICODE_STRING _symbo_;
	RtlInitUnicodeString(&_symbo_, m_symbo_link);
	_symbo_.MaximumLength = MAXIMUM_FILENAME_LENGTH -1;
	RtlCopyUnicodeString(&_symbo_, symbo);
	KdPrint(("symbo link set:%S\n", m_symbo_link));
}


