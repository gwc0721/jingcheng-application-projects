#pragma once

#include "CoreMnt.h"

#include "../../Comm/virtual_disk.h"


//#define ENABLE_SYNC

inline NTSTATUS CompleteIrp( PIRP Irp, NTSTATUS status, ULONG info);

class CIrpQueue
{
public:
	CIrpQueue(void);
	~CIrpQueue(void);

	void Initialize(void);
	bool push(IN PIRP irp);
	bool pop(OUT PIRP &irp);
	// return true if queue is empty, no block;
	bool empty(void);

protected:
	bool IncreaseIfNotFull(void);
	bool DecreaseIfNotEmpty(void);

protected:
	PIRP	m_queue[MAX_IRP_QUEUE];
	int		m_head, m_tail, m_size;

	KEVENT	m_event;			// 用于当queue的内容改变时，发出通知
	KSPIN_LOCK	m_size_lock;	// 用于m_size锁
	FAST_MUTEX	m_mutex;		// 用于队列操作锁
};

struct IrpParam
{
    IrpParam(DISK_OPERATION_TYPE in_type, UINT32 in_size, UINT64 in_offset, char* in_buffer)
		: type(in_type), size(in_size), offset(in_offset), buffer(in_buffer){}
	IrpParam() : type(DISK_OP_EMPTY), size(0), offset(0), buffer(0){}

    DISK_OPERATION_TYPE type;
    UINT32 size;
    UINT64 offset;
    char* buffer;
};

class CMountManager;

class CMountedDisk
{
public:
    //CMountedDisk(PDRIVER_OBJECT DriverObject,
    //            CMountManager* mountManager,
    //            int devId,
    //            UINT64 totalLength);
    //~CMountedDisk();

	// length in sectors
	void Initialize(IN PDRIVER_OBJECT DriverObject, IN CMountManager* mountManager,
			IN UINT32 devId, IN UINT64 total_sec);
	void Release(void);

    NTSTATUS DispatchIrp(IN PIRP irp);
    NTSTATUS RequestExchange(IN CORE_MNT_EXCHANGE_REQUEST * request, OUT CORE_MNT_EXCHANGE_RESPONSE * response);
	NTSTATUS Disconnect(void);

	//void SetDeviceName(const WCHAR * dev_name);
	void SetDeviceName(PUNICODE_STRING dev_name, PUNICODE_STRING symbo);

protected:
	void LocalDispatch(IN PIRP irp);
    void CompleteLastIrp(NTSTATUS status, ULONG information);

protected:
	WCHAR			m_device_name[MAXIMUM_FILENAME_LENGTH];
	WCHAR			m_symbo_link[MAXIMUM_FILENAME_LENGTH];
	CIrpQueue		m_irp_queue;
	PDRIVER_OBJECT	m_driver_obj;
	CMountManager	* m_mnt_manager;
	UINT32				m_dev_id;
	UINT64			m_total_sectors;		// length in sectors
	PIRP			m_last_irp;
};

