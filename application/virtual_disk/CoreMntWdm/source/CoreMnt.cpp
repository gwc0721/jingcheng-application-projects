
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


PDEVICE_OBJECT g_mount_device = NULL;


// local function defination
NTSTATUS ControlDeviceIrpHandler( IN CMountManager * mm,  IN PDEVICE_OBJECT fdo, IN PIRP pIrp );

#define COREMNT_PNP_SUPPORT

// {54659E9C-B407-4269-99F2-9A20D89E3575}
static const GUID COREMNT_GUID = COREMNT_CLASS_GUID;
//{ 0x54659e9c, 0xb407, 0x4269, { 0x99, 0xf2, 0x9a, 0x20, 0xd8, 0x9e, 0x35, 0x75 } };



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
    RtlInitUnicodeString(&gDeviceName,       COREMNT_DEV_NAME);
    RtlInitUnicodeString(&gSymbolicLinkName, COREMNT_SYMBOLINK);


	KdPrint(("Create mount device\n"));
	// create a device for mount manage
    NTSTATUS status;
	PDEVICE_OBJECT fdo=NULL;
    status = IoCreateDevice(driver_obj,     // pointer on driver_obj
                            sizeof(CMountManager),                // additional size of memory, for device extension
                            &gDeviceName,     // pointer to UNICODE_STRING
                            FILE_DEVICE_NULL, // Device type
                            0,                // Device characteristic
                            FALSE,            // "Exclusive" device
                            &fdo);  // pointer do device object
    if (status != STATUS_SUCCESS)	return STATUS_FAILED_DRIVER_ENTRY;
	g_mount_device = fdo;
	CMountManager * pdx = (CMountManager *)(fdo->DeviceExtension);
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

// comm log information
	KdPrint(("size: CORE_MNT_COMM = %d\n", sizeof(CORE_MNT_COMM) ));
	KdPrint(("size: CORE_MNT_MOUNT_REQUEST =%d\n",sizeof(CORE_MNT_MOUNT_REQUEST) ));
	KdPrint(("size: CORE_MNT_EXCHANGE_REQUEST = %d\n", sizeof(CORE_MNT_EXCHANGE_REQUEST) ));
	KdPrint(("size: CORE_MNT_EXCHANGE_RESPONSE = %d\n", sizeof(CORE_MNT_EXCHANGE_RESPONSE) ));




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
                           IN PDEVICE_OBJECT pdo)
{ 
	PAGED_CODE();
	LOG_STACK_TRACE("v3");

    RtlInitUnicodeString(&gDeviceName,       COREMNT_DEV_NAME);
    RtlInitUnicodeString(&gSymbolicLinkName, COREMNT_SYMBOLINK);

	NTSTATUS status;
	PDEVICE_OBJECT fdo = NULL;
	//UNICODE_STRING devName;
    status = IoCreateDevice(driver_obj,     // pointer on driver_obj
				sizeof(CMountManager),                // additional size of memory, for device extension
                NULL,     // pointer to UNICODE_STRING
                FILE_DEVICE_NULL, // Device type
                0,                // Device characteristic
                FALSE,            // "Exclusive" device
                &fdo);  // pointer do device object

	if( !NT_SUCCESS(status) )
	{
		KdPrint(("failure on createing pnp device"));
		return status;
	}
	g_mount_device = fdo;
	CMountManager * pdx = (CMountManager *)(fdo->DeviceExtension);

	pdx->Initialize(driver_obj);
	pdx->fdo = fdo;
	pdx->NextStackDevice = IoAttachDeviceToDeviceStack(fdo, pdo);
	pdx->ustrDeviceName = gDeviceName;
	//pdx->ustrSymLinkName = gSymbolicLinkName;

    //status = IoCreateSymbolicLink(&gSymbolicLinkName, &gDeviceName);

	status = IoRegisterDeviceInterface(pdo, &COREMNT_GUID, NULL, &pdx->ustrSymLinkName);
    if (status != STATUS_SUCCESS)
	{
		KdPrint(("failure on registing interface"));
		return status;
	}
	KdPrint(("symbo_link:%wZ", &pdx->ustrSymLinkName));

	status = IoSetDeviceInterfaceState(&pdx->ustrSymLinkName, TRUE);
	if (status != STATUS_SUCCESS)
	{
		KdPrint(("failure on enable interface."));
		return status;
	}

	//fdo->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
	//fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}

#endif


/************************************************************************
*************************************************************************/ 
#pragma PAGEDCODE

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
		//KdPrint(("irp handler for g_mount_device\n"));
		CMountManager * mm = reinterpret_cast<CMountManager *>(fdo->DeviceExtension);		ASSERT(mm);
		status = ControlDeviceIrpHandler(mm, fdo, pIrp);
		return status;
	}
	else
	{
		//KdPrint(("irp handler for drive device\n"));
		CMountedDisk * mnt_disk = reinterpret_cast<CMountedDisk*>(fdo->DeviceExtension);
		status = mnt_disk->DispatchIrp(pIrp);
		return status;
	}
}

NTSTATUS ControlDeviceIrpHandler( IN CMountManager * mm, IN PDEVICE_OBJECT fdo, IN PIRP pIrp )
{
	PAGED_CODE();
	LOG_STACK_TRACE("");

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(pIrp);
	//KdPrint( ("major func: %X\n", irpStack->MajorFunction) );
    switch(irpStack->MajorFunction)
    {
    case IRP_MJ_CREATE:
		KdPrint( ("[IRP] mnt <- IRP_MJ_CREATE\n") );
		return CompleteIrp(pIrp, STATUS_SUCCESS,0);
    
	case IRP_MJ_CLOSE:
		KdPrint( ("[IRP] mnt <- IRP_MJ_CLOSE\n") );
		return CompleteIrp(pIrp, STATUS_SUCCESS,0);
    
	case IRP_MJ_CLEANUP:
		KdPrint( ("[IRP] mnt <- IRP_MJ_CLEANUP\n") );
        return CompleteIrp(pIrp, STATUS_SUCCESS,0);

    case IRP_MJ_DEVICE_CONTROL:	        {
		NTSTATUS status = mm->DispatchDeviceControl(pIrp, irpStack);
		return status;					}
	default:
		KdPrint(("[IRP] mnt <- UNKNOW_MAJOR_FUNC(0x%08X)", irpStack->MajorFunction));

    }
    __asm int 3;
    return CompleteIrp(pIrp, STATUS_UNSUCCESSFUL, 0);
}


#pragma LOCKEDCODE
NTSTATUS OnPnpIrpComplete(PDEVICE_OBJECT fdo, PIRP irp, PKEVENT evt)
{
	KdPrint(("pnp irp completed.\n"));
	KeSetEvent(evt, 0, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
}

#pragma PAGEDCODE
NTSTATUS ForwardAndWait(CMountManager * mm, PIRP irp)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");
	
	KEVENT evt;
	KeInitializeEvent(&evt, NotificationEvent, FALSE);
	IoCopyCurrentIrpStackLocationToNext(irp);
	IoSetCompletionRoutine(irp, (PIO_COMPLETION_ROUTINE)(OnPnpIrpComplete), (PVOID)(&evt), TRUE, TRUE, TRUE);
	IoCallDriver(mm->NextStackDevice, irp);
	KdPrint(("waiting for irp complete\n"));
	KeWaitForSingleObject(&evt, Executive, KernelMode, FALSE, NULL);
	return irp->IoStatus.Status;
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
NTSTATUS DefaultPnpHandler(CMountManager * pdx, PIRP irp)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");
	IoSkipCurrentIrpStackLocation(irp);
	return IoCallDriver(pdx->NextStackDevice, irp);
	//return ForwardAndWait(pdx, irp);
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
NTSTATUS HandleRemoveDevice(CMountManager * pdx, PIRP Irp)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");

	Irp->IoStatus.Status = STATUS_SUCCESS;
	NTSTATUS status = DefaultPnpHandler(pdx, Irp);
	//IoDeleteSymbolicLink(&(UNICODE_STRING)pdx->ustrSymLinkName);

	IoSetDeviceInterfaceState(&pdx->ustrSymLinkName, FALSE);
	RtlFreeUnicodeString(&pdx->ustrSymLinkName);

    //调用IoDetachDevice()把fdo从设备栈中脱开：
    if (pdx->NextStackDevice) IoDetachDevice(pdx->NextStackDevice);

	pdx->Release();
	
    //删除fdo：
    IoDeleteDevice(pdx->fdo);
	return status;
}

NTSTATUS CoreMntQueryRemoveDevice(CMountManager * pdx, PIRP irp)
{
	PAGED_CODE();
	LOG_STACK_TRACE("");
	bool removable = pdx->CanBeRemoved();
	KdPrint(("core mnt is %s\n", removable?"removable":"un-removable"));
	
	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
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
	ULONG fcn = stack->MinorFunction;


#if DBG
	char * str_dev =NULL;
	char * str_func = NULL;
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
	if (fdo == gDeviceObject)			str_dev = "dev";
	else if (fdo == g_mount_device)		str_dev = "mount";
	else								str_dev = "disk";
	if (fcn < arraysize(fcnname) )		str_func = fcnname[fcn];
	else								str_func = "UNKNOWN_PNP_REQUEST";
	KdPrint(("[IRP] %s <- PNP::%s\n", str_dev, str_func));
#endif




#ifndef COREMNT_PNP_SUPPORT
	if (fdo != gDeviceObject)
	{
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return status;
	}
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
#else
	if (fdo != g_mount_device)
	{
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return status;
	}
	CMountManager * pdx = reinterpret_cast<CMountManager *>(fdo->DeviceExtension);
#endif

	static NTSTATUS (*fcntab[])(CMountManager * pdx, PIRP Irp) = 
	{
		DefaultPnpHandler,		// IRP_MN_START_DEVICE
		CoreMntQueryRemoveDevice,		// IRP_MN_QUERY_REMOVE_DEVICE
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

	if (fcn >= arraysize(fcntab))
	{						// unknown function
		status = DefaultPnpHandler(pdx, Irp); // some function we don't know about
		return status;
	}						// unknown function

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

#ifndef COREMNT_PNP_SUPPORT
	ASSERT(g_mount_device);
	CMountManager * mm = reinterpret_cast<CMountManager *>(g_mount_device->DeviceExtension);		
	ASSERT(mm);
	mm->Release();

	IoDeleteSymbolicLink(&gSymbolicLinkName);
    IoDeleteDevice(g_mount_device);
#endif
}


