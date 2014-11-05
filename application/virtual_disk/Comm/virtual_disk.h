#pragma once

#include "config.h"

#include <ntddscsi.h>

#define CDB6GENERIC_LENGTH	6
#define CDB10GENERIC_LENGTH	16

#define IRP_MJ_NOP			0xFF
#define IRP_MJ_DISCONNECT	0xFE

enum READ_WRITE
{
	NO_READ_WRITE = 0,
	READ = 0x01,
	WRITE = 0x02,
	READ_AND_WRITE = 3,
};

struct CORE_MNT_EXCHANGE_REQUEST
{
    ULONG32 dev_id;
	UCHAR	m_major_func;
	UCHAR	m_read_write;
	ULONG	m_minor_code;

    //ULONG32 lastType; 
    ULONG32 lastStatus; 
    ULONG32 lastSize; 
    UCHAR *  data;
    ULONG32 dataSize;
};

struct CORE_MNT_EXCHANGE_RESPONSE
{
	UCHAR	m_major_func;
	UCHAR	m_read_write;
	ULONG	m_minor_code;

    //ULONG32 type;
    ULONG32 size; 
    ULONG64 offset; 
};

struct CORE_MNT_MOUNT_REQUEST
{
    IN	ULONG64	total_sec;	// in sectors
	OUT UINT32	dev_id;
	IN	WCHAR	symbo_link[1];
};

// comm for request and response
struct CORE_MNT_COMM
{
	UINT32 dev_id;
};

//#define CORE_MNT_MOUNT_RESPONSE		CORE_MNT_COMM
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


