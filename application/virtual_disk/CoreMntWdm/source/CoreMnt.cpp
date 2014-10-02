
#include "CoreMnt.h"

#define LOGGER_LEVEL	LOGGER_LEVEL_DEBUGINFO
#include "jclogk.h"


#ifndef _WIN32_WINNT            // 指定要求的最低平台是 Windows Vista。
#define _WIN32_WINNT 0x0600     // 将此值更改为相应的值，以适用于 Windows 的其他版本。
#endif

#include "../../Comm/virtual_disk.h"

#include "MountedDisk.h"
#include "MountManager.h"


//The name of current driver
UNICODE_STRING gDeviceName;
//The symbolic link of driver location
UNICODE_STRING gSymbolicLinkName;


///////////////////////////////////////////////////////////////////////////////
// -- Function declaration
NTSTATUS CoreMntAddDevice(IN PDRIVER_OBJECT driver_obj, IN PDEVICE_OBJECT PhysicalDeviceObject);
NTSTATUS IrpHandler( IN PDEVICE_OBJECT fdo, IN PIRP pIrp );
void CoreMntUnload(IN PDRIVER_OBJECT driver_obj);
NTSTATUS CoreMntPnp(IN PDEVICE_OBJECT fdo, IN PIRP Irp);

inline NTSTATUS CompleteIrp( PIRP Irp, NTSTATUS status, ULONG info)
{
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp,IO_NO_INCREMENT);
    return status;
}

PDEVICE_OBJECT g_mount_device = NULL;


// local function defination
NTSTATUS ControlDeviceIrpHandler( IN MountManager * mm,  IN PDEVICE_OBJECT fdo, IN PIRP pIrp );

//#define COREMNT_PNP_SUPPORT


/************************************************************************
* 函数名称:DriverEntry
* 功能描述:初始化驱动程序，定位和申请硬件资源，创建内核对象
* 参数列表:
      driver_obj:从I/O管理器中传进来的驱动对象
      pRegistryPath:驱动程序在注册表的中的路径
* 返回 值:返回初始化驱动状态
*************************************************************************/
#pragma INITCODE 
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT driver_obj,
								IN PUNICODE_STRING pRegistryPath)
{
	LOG_STACK_TRACE("");

#ifndef COREMNT_PNP_SUPPORT
    /* Start Driver initialization */
    RtlInitUnicodeString(&gDeviceName,       DEVICE_NAME);
    RtlInitUnicodeString(&gSymbolicLinkName, SYMBO_LINK_NAME);


	KdPrint(("Create mount device\n"));
	// create a device for mount manage
    NTSTATUS status;
	PDEVICE_OBJECT fdo=NULL;
    status = IoCreateDevice(driver_obj,     // pointer on driver_obj
                            sizeof(MountManager),                // additional size of memory, for device extension
                            &gDeviceName,     // pointer to UNICODE_STRING
                            FILE_DEVICE_NULL, // Device type
                            0,                // Device characteristic
                            FALSE,            // "Exclusive" device
                            &fdo);  // pointer do device object
    if (status != STATUS_SUCCESS)	return STATUS_FAILED_DRIVER_ENTRY;
	g_mount_device = fdo;
	MountManager * pdx = (MountManager *)(fdo->DeviceExtension);
	pdx->Initialize(driver_obj);

	pdx->fdo = fdo;
	pdx->NextStackDevice = NULL;
	pdx->ustrDeviceName = gDeviceName;
	pdx->ustrSymLinkName = gSymbolicLinkName;

    status = IoCreateSymbolicLink(&gSymbolicLinkName, &gDeviceName);
    if (status != STATUS_SUCCESS)	return STATUS_FAILED_DRIVER_ENTRY;
#endif

    // Register IRP handlers
    PDRIVER_DISPATCH *mj_func;
    mj_func = driver_obj->MajorFunction;

	for (size_t i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; ++i) 
        driver_obj->MajorFunction[i] = IrpHandler;

	driver_obj->DriverExtension->AddDevice = CoreMntAddDevice;
	driver_obj->MajorFunction[IRP_MJ_PNP] = CoreMntPnp;
	driver_obj->DriverUnload = CoreMntUnload;
	return STATUS_SUCCESS;
}

/************************************************************************
* 函数名称:CoreMntAddDevice
* 功能描述:添加新设备
* 参数列表:
      driver_obj:从I/O管理器中传进来的驱动对象
      PhysicalDeviceObject:从I/O管理器中传进来的物理设备对象
* 返回 值:返回添加新设备状态
*************************************************************************/
#pragma PAGEDCODE


//The object associated with the driver
PDEVICE_OBJECT gDeviceObject = NULL;

#ifndef COREMNT_PNP_SUPPORT

NTSTATUS CoreMntAddDevice(IN PDRIVER_OBJECT driver_obj,
                           IN PDEVICE_OBJECT PhysicalDeviceObject)
{ 
	PAGED_CODE();
	LOG_STACK_TRACE("");

	KdPrint( ("kdprint test ansi %s\n", "hello") );
	//KdPrint( ("kdprint test ansi S %S\n", "hello") );		!!WRONG
	//KdPrint( ("kdprint test unicode %s\n", L"hello") );	!!WRONG
	KdPrint( ("kdprint test unicode S %S\n", L"hello") );

	NTSTATUS status;
	PDEVICE_OBJECT fdo = NULL;
	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName,L"\\Device\\MyWDMDevice");
	status = IoCreateDevice(
		driver_obj,
		sizeof(DEVICE_EXTENSION),
		&(UNICODE_STRING)devName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&fdo);
	if( !NT_SUCCESS(status))		return status;
	gDeviceObject = fdo;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)(fdo->DeviceExtension);
	pdx->fdo = fdo;
	pdx->NextStackDevice = IoAttachDeviceToDeviceStack(fdo, PhysicalDeviceObject);
	UNICODE_STRING symLinkName;
	RtlInitUnicodeString(&symLinkName,L"\\DosDevices\\HelloWDM");

	pdx->ustrDeviceName = devName;
	pdx->ustrSymLinkName = symLinkName;
	status = IoCreateSymbolicLink(&(UNICODE_STRING)symLinkName,&(UNICODE_STRING)devName);

	if( !NT_SUCCESS(status))
	{
		IoDeleteSymbolicLink(&pdx->ustrSymLinkName);
		status = IoCreateSymbolicLink(&symLinkName,&devName);
		if( !NT_SUCCESS(status))
		{
			return status;
		}
	}

	fdo->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
	fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}

#else

NTSTATUS CoreMntAddDevice(IN PDRIVER_OBJECT driver_obj,
                           IN PDEVICE_OBJECT PhysicalDeviceObject)
{ 
	PAGED_CODE();
	LOG_STACK_TRACE("");

    RtlInitUnicodeString(&gDeviceName,       DEVICE_NAME);
    RtlInitUnicodeString(&gSymbolicLinkName, SYMBO_LINK_NAME);

	NTSTATUS status;
	PDEVICE_OBJECT fdo = NULL;
	UNICODE_STRING devName;
    status = IoCreateDevice(driver_obj,     // pointer on driver_obj
                            sizeof(MountManager),                // additional size of memory, for device extension
                            &gDeviceName,     // pointer to UNICODE_STRING
                            FILE_DEVICE_NULL, // Device type
                            0,                // Device characteristic
                            FALSE,            // "Exclusive" device
                            &fdo);  // pointer do device object

	if( !NT_SUCCESS(status))		return status;
	g_mount_device = fdo;
	MountManager * pdx = (MountManager *)(fdo->DeviceExtension);

	pdx->Initialize(driver_obj);
	pdx->fdo = fdo;
	pdx->NextStackDevice = IoAttachDeviceToDeviceStack(fdo, PhysicalDeviceObject);
	pdx->ustrDeviceName = gDeviceName;
	pdx->ustrSymLinkName = gSymbolicLinkName;

    status = IoCreateSymbolicLink(&gSymbolicLinkName, &gDeviceName);
    if (status != STATUS_SUCCESS)
	{
		return STATUS_FAILED_DRIVER_ENTRY;
	}

	//fdo->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
	//fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}

#endif


/************************************************************************
*************************************************************************/ 
#pragma PAGECODE

NTSTATUS IrpHandler(IN PDEVICE_OBJECT fdo, IN PIRP pIrp )
{
	PAGED_CODE();

	LOG_STACK_TRACE("");
	NTSTATUS status;
    if(fdo == gDeviceObject)
    {
		KdPrint(("irp handler for gDeviceObject\n"));
        return CompleteIrp(pIrp, STATUS_SUCCESS,0);
    }
	else if (fdo == g_mount_device)
	{
		KdPrint(("irp handler for g_mount_device\n"));
		MountManager * mm = reinterpret_cast<MountManager *>(fdo->DeviceExtension);		ASSERT(mm);
		status = ControlDeviceIrpHandler(mm, fdo, pIrp);
		return status;
	}
	else
	{
		KdPrint(("irp handler for drive device\n"));
		MountedDisk * mnt_disk = reinterpret_cast<MountedDisk*>(fdo->DeviceExtension);
		status = mnt_disk->DispatchIrp(pIrp);
		return status;
	}
}

NTSTATUS ControlDeviceIrpHandler( IN MountManager * mm, IN PDEVICE_OBJECT fdo, IN PIRP pIrp )
{
	PAGED_CODE();
	LOG_STACK_TRACE("");

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(pIrp);
	KdPrint( ("major func: %X\n", irpStack->MajorFunction) );
    switch(irpStack->MajorFunction)
    {
    case IRP_MJ_CREATE:
			KdPrint( ("major func: IRP_MJ_CREATE\n") );
			return CompleteIrp(pIrp, STATUS_SUCCESS,0);
    case IRP_MJ_CLOSE:
			KdPrint( ("major func: IRP_MJ_CLOSE\n") );
			return CompleteIrp(pIrp, STATUS_SUCCESS,0);
    case IRP_MJ_CLEANUP:
			KdPrint( ("major func: IRP_MJ_CLEANUP\n") );
        return CompleteIrp(pIrp, STATUS_SUCCESS,0);

    case IRP_MJ_DEVICE_CONTROL:	        {
			KdPrint( ("major func: IRP_MJ_DEVICE_CONTROL\n") );
			ASSERT(mm);
            ULONG code = irpStack->Parameters.DeviceIoControl.IoControlCode;
            PVOID buffer = pIrp->AssociatedIrp.SystemBuffer;
            ULONG outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;    
            ULONG inputBufferLength = irpStack->Parameters.DeviceIoControl.InputBufferLength; 
            NTSTATUS status = STATUS_SUCCESS;
            switch (code)
            {
            case CORE_MNT_MOUNT_IOCTL:	{
				KdPrint( ("ioctrl: CORE_MNT_MOUNT_IOCTL\n"));
				KdPrint( ("request size=%d, response size=%d\n",sizeof(CORE_MNT_MOUNT_REQUEST), sizeof(CORE_MNT_MOUNT_RESPONSE) ) );
				KdPrint( ("input buffer len=%d, output buf len=%d\n", inputBufferLength, outputBufferLength) );


				//if(inputBufferLength < sizeof(CORE_MNT_MOUNT_REQUEST) || 
				//	outputBufferLength < sizeof(CORE_MNT_MOUNT_RESPONSE) )
				if(inputBufferLength <= 0 || outputBufferLength <= 0 )
				{
					KdPrint( ("input or output buffer size mismatch\n") );
					status = STATUS_UNSUCCESSFUL;
					break;
				}

				CORE_MNT_MOUNT_REQUEST * request = (CORE_MNT_MOUNT_REQUEST *)buffer;
				UINT64 totalLength = request->totalLength;
				KdPrint( ("disk size=%I64d, mount point=%lc\n", totalLength, request->mountPojnt) );

				int dev_id = -1;
				status = mm->Mount(totalLength, dev_id);
				if ( !NT_SUCCESS(status) )		{	KdPrint( ("disk map is full\n") );		}

				CORE_MNT_MOUNT_RESPONSE * response = (CORE_MNT_MOUNT_RESPONSE *)buffer;
				response->deviceId = dev_id;
				break;					}

			case CORE_MNT_EXCHANGE_IOCTL:		{
				KdPrint( ("ioctrl CORE_MNT_EXCHANGE_IOCTL\n"));
				KdPrint( ("request size=%d, response size=%d\n",sizeof(CORE_MNT_EXCHANGE_REQUEST), sizeof(CORE_MNT_EXCHANGE_RESPONSE) ) );
				KdPrint( ("input buffer len=%d, output buf len=%d\n", inputBufferLength, outputBufferLength) );
				//if(	inputBufferLength < sizeof(CORE_MNT_EXCHANGE_REQUEST) || 
				//	outputBufferLength < sizeof(CORE_MNT_EXCHANGE_RESPONSE) )
				if(inputBufferLength <= 0 || outputBufferLength <= 0 )
				{
					KdPrint( ("input or output buffer size mismatch\n") );
					status = STATUS_UNSUCCESSFUL;
					break;
				}
				CORE_MNT_EXCHANGE_REQUEST * request = (CORE_MNT_EXCHANGE_REQUEST *)buffer;
				CORE_MNT_EXCHANGE_RESPONSE response = {0};
				//mm->RequestExchange(request->deviceId,
				//		request->lastType, request->lastStatus, request->lastSize, 
				//		request->data, request->dataSize, 
				//		&response.type, &response.size, &response.offset);
				mm->RequestExchange(request, &response);
				KdPrint(("response: type=%d, size=%d\n", response.type, response.size));
				RtlCopyMemory(buffer, &response, sizeof(CORE_MNT_EXCHANGE_RESPONSE) );
				status = STATUS_SUCCESS;
				//return STATUS_SUCCESS;
				break;								}

			case CORE_MNT_UNMOUNT_IOCTL:		{
				KdPrint( ("ioctrl CORE_MNT_UNMOUNT_IOCTL\n"));
				KdPrint( ("request size=%d\n",sizeof(CORE_MNT_UNMOUNT_REQUEST) ) );
				KdPrint( ("input buffer len=%d\n", inputBufferLength) );
		        //if(inputBufferLength < sizeof(CORE_MNT_UNMOUNT_REQUEST))
 		        if(inputBufferLength <= 0)
				{
					KdPrint( ("input buffer size mismatch\n") );
					status = STATUS_UNSUCCESSFUL;
					break;
				}
				CORE_MNT_UNMOUNT_REQUEST * request = (CORE_MNT_UNMOUNT_REQUEST *)buffer;
				status = mm->Unmount(request->deviceId);
				break;		}
            }
            return CompleteIrp(pIrp,status,outputBufferLength);
        }
    }
    __asm int 3;
    return CompleteIrp(pIrp, STATUS_UNSUCCESSFUL, 0);
}


/************************************************************************
* 函数名称:DefaultPnpHandler
* 功能描述:对PNP IRP进行缺省处理
* 参数列表:
      pdx:设备对象的扩展
      Irp:从IO请求包
* 返回 值:返回状态
*************************************************************************/ 
#pragma PAGEDCODE
NTSTATUS DefaultPnpHandler(PDEVICE_EXTENSION pdx, PIRP Irp)
{
	PAGED_CODE();
	KdPrint(("Enter DefaultPnpHandler\n"));
	IoSkipCurrentIrpStackLocation(Irp);
	KdPrint(("Leave DefaultPnpHandler\n"));
	return IoCallDriver(pdx->NextStackDevice, Irp);
}

/************************************************************************
* 函数名称:HandleRemoveDevice
* 功能描述:对IRP_MN_REMOVE_DEVICE IRP进行处理
* 参数列表:
      fdo:功能设备对象
      Irp:从IO请求包
* 返回 值:返回状态
*************************************************************************/
#pragma PAGEDCODE
NTSTATUS HandleRemoveDevice(PDEVICE_EXTENSION pdx, PIRP Irp)
{
	PAGED_CODE();
	KdPrint(("Enter HandleRemoveDevice\n"));

	Irp->IoStatus.Status = STATUS_SUCCESS;
	NTSTATUS status = DefaultPnpHandler(pdx, Irp);
	IoDeleteSymbolicLink(&(UNICODE_STRING)pdx->ustrSymLinkName);

    //调用IoDetachDevice()把fdo从设备栈中脱开：
    if (pdx->NextStackDevice)
        IoDetachDevice(pdx->NextStackDevice);
	
    //删除fdo：
    IoDeleteDevice(pdx->fdo);
	KdPrint(("Leave HandleRemoveDevice\n"));
	return status;
}

/************************************************************************
* 函数名称:CoreMntPnp
* 功能描述:对即插即用IRP进行处理
* 参数列表:
      fdo:功能设备对象
      Irp:从IO请求包
* 返回 值:返回状态
*************************************************************************/
#pragma PAGEDCODE
NTSTATUS CoreMntPnp(IN PDEVICE_OBJECT fdo,
                        IN PIRP Irp)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");

	NTSTATUS status = STATUS_SUCCESS;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

#ifndef COREMNT_PNP_SUPPORT
	if (fdo != gDeviceObject)
	{
		if (fdo == g_mount_device)	KdPrint(("pnp handler for mount device\n"));
		else						KdPrint(("pnp handler for disk device\n"));
		KdPrint(("minor:%d\n", stack->MinorFunction));
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return status;
	}
#else
	if (fdo != g_mount_device)
	{
		KdPrint(("pnp handler for disk device\n"));
		KdPrint(("minor:%d\n", stack->MinorFunction));
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return status;
	}
#endif

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	static NTSTATUS (*fcntab[])(PDEVICE_EXTENSION pdx, PIRP Irp) = 
	{
		DefaultPnpHandler,		// IRP_MN_START_DEVICE
		DefaultPnpHandler,		// IRP_MN_QUERY_REMOVE_DEVICE
		HandleRemoveDevice,		// IRP_MN_REMOVE_DEVICE
		DefaultPnpHandler,		// IRP_MN_CANCEL_REMOVE_DEVICE
		DefaultPnpHandler,		// IRP_MN_STOP_DEVICE
		DefaultPnpHandler,		// IRP_MN_QUERY_STOP_DEVICE
		DefaultPnpHandler,		// IRP_MN_CANCEL_STOP_DEVICE
		DefaultPnpHandler,		// IRP_MN_QUERY_DEVICE_RELATIONS
		DefaultPnpHandler,		// IRP_MN_QUERY_INTERFACE
		DefaultPnpHandler,		// IRP_MN_QUERY_CAPABILITIES
		DefaultPnpHandler,		// IRP_MN_QUERY_RESOURCES
		DefaultPnpHandler,		// IRP_MN_QUERY_RESOURCE_REQUIREMENTS
		DefaultPnpHandler,		// IRP_MN_QUERY_DEVICE_TEXT
		DefaultPnpHandler,		// IRP_MN_FILTER_RESOURCE_REQUIREMENTS
		DefaultPnpHandler,		// 
		DefaultPnpHandler,		// IRP_MN_READ_CONFIG
		DefaultPnpHandler,		// IRP_MN_WRITE_CONFIG
		DefaultPnpHandler,		// IRP_MN_EJECT
		DefaultPnpHandler,		// IRP_MN_SET_LOCK
		DefaultPnpHandler,		// IRP_MN_QUERY_ID
		DefaultPnpHandler,		// IRP_MN_QUERY_PNP_DEVICE_STATE
		DefaultPnpHandler,		// IRP_MN_QUERY_BUS_INFORMATION
		DefaultPnpHandler,		// IRP_MN_DEVICE_USAGE_NOTIFICATION
		DefaultPnpHandler,		// IRP_MN_SURPRISE_REMOVAL
	};

	ULONG fcn = stack->MinorFunction;
	if (fcn >= arraysize(fcntab))
	{						// unknown function
		status = DefaultPnpHandler(pdx, Irp); // some function we don't know about
		return status;
	}						// unknown function

#if DBG
	static char* fcnname[] = 
	{
		"IRP_MN_START_DEVICE",
		"IRP_MN_QUERY_REMOVE_DEVICE",
		"IRP_MN_REMOVE_DEVICE",
		"IRP_MN_CANCEL_REMOVE_DEVICE",
		"IRP_MN_STOP_DEVICE",
		"IRP_MN_QUERY_STOP_DEVICE",
		"IRP_MN_CANCEL_STOP_DEVICE",
		"IRP_MN_QUERY_DEVICE_RELATIONS",
		"IRP_MN_QUERY_INTERFACE",
		"IRP_MN_QUERY_CAPABILITIES",
		"IRP_MN_QUERY_RESOURCES",
		"IRP_MN_QUERY_RESOURCE_REQUIREMENTS",
		"IRP_MN_QUERY_DEVICE_TEXT",
		"IRP_MN_FILTER_RESOURCE_REQUIREMENTS",
		"",
		"IRP_MN_READ_CONFIG",
		"IRP_MN_WRITE_CONFIG",
		"IRP_MN_EJECT",
		"IRP_MN_SET_LOCK",
		"IRP_MN_QUERY_ID",
		"IRP_MN_QUERY_PNP_DEVICE_STATE",
		"IRP_MN_QUERY_BUS_INFORMATION",
		"IRP_MN_DEVICE_USAGE_NOTIFICATION",
		"IRP_MN_SURPRISE_REMOVAL",
	};

	KdPrint(("PNP Request (%s)\n", fcnname[fcn]));
#endif // DBG

	status = (*fcntab[fcn])(pdx, Irp);
	return status;
}

/************************************************************************
* 函数名称:CoreMntUnload
* 功能描述:负责驱动程序的卸载操作
* 参数列表:
      driver_obj:驱动对象
* 返回 值:返回状态
*************************************************************************/
#pragma PAGEDCODE
void CoreMntUnload(IN PDRIVER_OBJECT driver_obj)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");
	IoDeleteSymbolicLink(&gSymbolicLinkName);
    IoDeleteDevice(g_mount_device);
}


