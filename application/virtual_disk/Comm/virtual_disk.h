#pragma once

#include "config.h"

enum DISK_OPERATION_TYPE
{
    directOperationSuccess = 0,
    directOperationEmpty = 0,
    directOperationRead = 1,
    directOperationWrite = 2,
    directOperationFail = 3,
    directOperationMax = directOperationFail,
};

struct CORE_MNT_EXCHANGE_REQUEST
{
    ULONG32 deviceId;
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
    ULONG64 totalLength;
    WCHAR   mountPojnt;
};

struct CORE_MNT_MOUNT_RESPONSE
{
    ULONG32 deviceId;
};

struct CORE_MNT_UNMOUNT_REQUEST
{
    ULONG32 deviceId;
};

#define SECTOR_SIZE		512

#define TOC_DATA_TRACK          0x04

#define CORE_MNT_DISPATCHER        0x8001

#define CORE_MNT_MOUNT_IOCTL \
    CTL_CODE(CORE_MNT_DISPATCHER, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define CORE_MNT_EXCHANGE_IOCTL \
    CTL_CODE(CORE_MNT_DISPATCHER, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define CORE_MNT_UNMOUNT_IOCTL \
    CTL_CODE(CORE_MNT_DISPATCHER, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)