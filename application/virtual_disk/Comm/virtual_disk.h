#pragma once

#include "config.h"

enum DISK_OPERATION_TYPE
{
    //directOperationSuccess = 0,
    DISK_OP_EMPTY = 0,
    DISK_OP_READ = 1,
    DISK_OP_WRITE = 2,
	DISK_OP_DISCONNECT = 3,
    //directOperationFail = 4,
    DISK_OP_MAX = DISK_OP_DISCONNECT,
};

struct CORE_MNT_EXCHANGE_REQUEST
{
    ULONG32 dev_id;
    ULONG32 lastType; 
    ULONG32 lastStatus; 
    ULONG32 lastSize; 
    char *  data;
    ULONG32 dataSize;
};

struct CORE_MNT_EXCHANGE_RESPONSE
{
    ULONG32 type;
    ULONG32 size; 
    ULONG64 offset; 
};

struct CORE_MNT_MOUNT_REQUEST
{
    ULONG64 total_sec;	// in sectors
    //WCHAR   mountPojnt;
};

// comm for request and response
struct CORE_MNT_COMM
{
	UINT32 dev_id;
};

#define CORE_MNT_MOUNT_RESPONSE		CORE_MNT_COMM
#define CORE_MNT_UNMOUNT_REQUEST	CORE_MNT_COMM

#define SECTOR_SIZE		512

#define TOC_DATA_TRACK          0x04

#define CORE_MNT_DISPATCHER        0x8001

#define CORE_MNT_MOUNT_IOCTL \
    CTL_CODE(CORE_MNT_DISPATCHER, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define CORE_MNT_EXCHANGE_IOCTL \
    CTL_CODE(CORE_MNT_DISPATCHER, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define CORE_MNT_UNMOUNT_IOCTL \
    CTL_CODE(CORE_MNT_DISPATCHER, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define CORE_MNT_DISCONNECT_IOCTL \
    CTL_CODE(CORE_MNT_DISPATCHER, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)