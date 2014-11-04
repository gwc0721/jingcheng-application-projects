#pragma once

#include "CoreMnt.h"

#include "../../Comm/virtual_disk.h"

inline NTSTATUS CompleteIrp( PIRP Irp, NTSTATUS status, ULONG info);

// 用于dispatch与exchange之间通信
struct IRP_EXCHANGE_REQUEST
{
	UCHAR m_major_func;
	UCHAR m_read_write;
	ULONG m_minor_code;
	UCHAR * m_kernel_buf;
	ULONG m_data_len;
	ULONG64	m_offset;
	NTSTATUS m_status;
	// 如果true,则由exchange完成irp，exchange需要复制缓存，并且删除此对象
	// 如果false，则有dispatch完成irp，exchange需要复制缓存，并且set event
	bool	m_complete;
	KEVENT	m_event;
	PIRP	m_irp;

};

template <typename ELEMENT>
class CIrpQueue
{
public:
	CIrpQueue(void);
	~CIrpQueue(void);

	void Initialize(void);
	bool push(IN ELEMENT irp);
	bool pop(OUT ELEMENT &irp);
	// return true if queue is empty, no block;
	bool empty(void);

protected:
	bool IncreaseIfNotFull(void);
	bool DecreaseIfNotEmpty(void);

protected:
	ELEMENT	m_queue[MAX_IRP_QUEUE];
	int		m_head, m_tail, m_size;

	KEVENT	m_event;			// 用于当queue的内容改变时，发出通知
	KSPIN_LOCK	m_size_lock;	// 用于m_size锁
	FAST_MUTEX	m_mutex;		// 用于队列操作锁
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

	void SetDeviceName(PUNICODE_STRING dev_name, PUNICODE_STRING symbo);

	void CompleteLastIrp(IRP_EXCHANGE_REQUEST * ier); 

protected:
	// 如果Driver能够处理IRP则处理并返回true，不能处理，则返回false，入队列，给USER处理
	bool LocalDispatch(IN PIRP irp);
	bool LocalDispatchIoCtrl(IN PIRP irp);
    void CompleteLastIrp(NTSTATUS status, ULONG information);
	void AsyncExchange(IN PIRP irp, IN UCHAR mj, IN ULONG mi, IN READ_WRITE rw, IN ULONG buf_size,
			IN ULONG64 offset, IN UCHAR* buf);
	void SynchExchangeInitBuf(IN PIRP irp, IN UCHAR mj, IN ULONG mi, IN READ_WRITE rw, IN ULONG buf_size,
			IN ULONG64 offset, OUT IRP_EXCHANGE_REQUEST & ier);
	void SynchExchange(IN IRP_EXCHANGE_REQUEST & ier);
	void SynchExchangeClean(IN IRP_EXCHANGE_REQUEST & ier, IN ULONG data_len);

protected:
	WCHAR			m_device_name[MAXIMUM_FILENAME_LENGTH];
	WCHAR			m_symbo_link[MAXIMUM_FILENAME_LENGTH];

	CIrpQueue<IRP_EXCHANGE_REQUEST*>		m_irp_queue;
	PDRIVER_OBJECT	m_driver_obj;
	CMountManager	* m_mnt_manager;
	UINT32				m_dev_id;
	UINT64			m_total_sectors;		// length in sectors
	IRP_EXCHANGE_REQUEST *			m_last_irp;
};

