#pragma once

#include "CoreMnt.h"

#include "../../Comm/virtual_disk.h"


#define ENABLE_SYNC

class CIrpQueue
{
public:
	CIrpQueue(void);
	~CIrpQueue(void);

	void Initialize(void);
	bool push(IN PIRP irp);
	bool pop(OUT PIRP &irp);

protected:
	bool IncreaseIfNotFull(void);
	bool DecreaseIfNotEmpty(void);

protected:
	PIRP	m_queue[MAX_IRP_QUEUE];
	int		m_head, m_tail, m_size;

	KEVENT	m_event;			// 用于当queue的内容改变时，发出通知
	KSPIN_LOCK	m_size_lock;	// 用于m_size锁
	FAST_MUTEX	m_mutex;		// 用于队列操作锁

	//KSEMAPHORE	m_semaph;	// 用于队列计数
	//KMUTEX	m_mutex;	// 用于队列操作锁

};



struct IrpParam
{
    IrpParam(DISK_OPERATION_TYPE in_type, UINT32 in_size, UINT64 in_offset, char* in_buffer)
		: type(in_type), size(in_size), offset(in_offset), buffer(in_buffer){}
	IrpParam() : type(directOperationEmpty), size(0), offset(0), buffer(0){}

    DISK_OPERATION_TYPE type;
    UINT32 size;
    UINT64 offset;
    char* buffer;
};

class MountManager;

class MountedDisk
{
public:
    //MountedDisk(PDRIVER_OBJECT DriverObject,
    //            MountManager* mountManager,
    //            int devId,
    //            UINT64 totalLength);
    //~MountedDisk();

	void Initialize(IN PDRIVER_OBJECT DriverObject, IN MountManager* mountManager,
			IN int devId, IN UINT64 totalLength);
	void Release(void);

    NTSTATUS DispatchIrp(IN PIRP irp);
    NTSTATUS RequestExchange(IN CORE_MNT_EXCHANGE_REQUEST * request, OUT CORE_MNT_EXCHANGE_RESPONSE * response);
        //UINT32 * type, UINT32 * length, UINT64 * offset);

	void SetDeviceName(const WCHAR * dev_name);

protected:
	//void GetIrpParam(IN PIRP irp, OUT IrpParam & param);
	void LocalDispatch(IN PIRP irp);
    void CompleteLastIrp(NTSTATUS status, ULONG information);

public:

protected:
	WCHAR			m_device_name[MAXIMUM_FILENAME_LENGTH];
	CIrpQueue		m_irp_queue;
	PDRIVER_OBJECT	m_driver_obj;
	MountManager	* m_mnt_manager;
	int				m_dev_id;
	UINT64			m_total_length;
	PIRP			m_last_irp;
};

