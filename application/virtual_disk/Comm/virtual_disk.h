#pragma once

#include "config.h"

#include <ntddscsi.h>

#define SPT_SENSE_LENGTH	32
#define SPTWB_DATA_LENGTH	4*512
#define CDB6GENERIC_LENGTH	6
#define CDB10GENERIC_LENGTH	16

typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER 
{
    SCSI_PASS_THROUGH_DIRECT sptd;
    ULONG             Filler;      // realign buffer to double word boundary
    UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;

typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS 
{
    SCSI_PASS_THROUGH spt;
    ULONG             Filler;      // realign buffers to double word boundary
    UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
    UCHAR             ucDataBuf[SPTWB_DATA_LENGTH];
} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;



//enum DISK_OPERATION_TYPE
//{
//    DISK_OP_EMPTY = 0,
//    DISK_OP_READ = 1,
//    DISK_OP_WRITE = 2,
//	DISK_OP_DISCONNECT = 3,
//    DISK_OP_MAX = DISK_OP_DISCONNECT,
//};

#define IRP_MJ_NOP			0xFF
#define IRP_MJ_DISCONNECT	0xFE

enum READ_WRITE
{
	READ = 0x01,
	WRITE = 0x02,
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


