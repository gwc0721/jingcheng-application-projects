#pragma once

//#define COREMNT_PNP_SUPPORT

#define COREMNT_NAME		L"CoreMnt"
#define COREMNT_DEV_NAME	L"\\Device\\" COREMNT_NAME
#define COREMNT_SYMBOLINK	L"\\DosDevices\\" COREMNT_NAME
#define COREMNT_USER_NAME	L"\\\\.\\" COREMNT_NAME

#define COREMNT_INTERFACE	_T("\\\\.\\Root#UNKNOWN#0000#{54659e9c-b407-4269-99f2-9a20d89e3575}")
//#define COREMNT_USER_NAME	COREMNT_INTERFACE

#define ROOT_DIR_NAME       L"\\Device\\CoreMntDevDir"
#define DIRECT_DISK_PREFIX	ROOT_DIR_NAME L"\\disk"
#define SYMBO_DIRECT_DISK	L"\\DosDevices\\CoreMntDisk"

#define MAX_MOUNTED_DISK	(5)
#define MAX_IRP_QUEUE		(32)

#define WDM_FN				_T("CoreMntWdm")
// 32MB (64K sec, max of lba48)
#define EXCHANGE_BUFFER_SIZE	(32 * 1024 * 1024)
//#define EXCHANGE_BUFFER_SIZE	(2 * 512)

#define COREMNT_CLASS_GUID { 0x54659e9c, 0xb407, 0x4269, { 0x99, 0xf2, 0x9a, 0x20, 0xd8, 0x9e, 0x35, 0x75 } }

