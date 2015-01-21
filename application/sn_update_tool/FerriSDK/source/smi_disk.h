#pragma once

#include "scsi_defs.h"
#define INQUIRY_BUFFER_LENGTH 	0x24

enum SMI_TESTER_TYPE
{
	NON_SMI_TESTER,
	TESTER_SM333,
	TESTER_SM334,
};

//=======================================================================================================================//
//=======================================================================================================================//
//=== Global Functions ===// ===>>>
BOOL SCSICommandReadTester(HANDLE hDisk,UCHAR *Buffer,int BufferSize ,UCHAR *Command);
BOOL Send_Inquiry_Tester(HANDLE device, UCHAR *pDataBuffer, DWORD buf_len, SMI_TESTER_TYPE & tester);
ULONG SMIGetSize(HANDLE hDevice);
bool TestOpen(CHAR Letter,HANDLE *hDisk);
BOOL TestClose(HANDLE hDisk);
BOOL SMISCSIReadTester(HANDLE hDevice, PUCHAR DataBuffer,ULONG BufferSize,UCHAR *Arg);
bool Send_Inquiry_XP_SM333(HANDLE device, UCHAR *buf, DWORD buf_len);
bool Send_Initial_Card_Command(HANDLE hDisk,BOOL enable);
bool SMITesterPowerOff(HANDLE device, SMI_TESTER_TYPE tester);
bool SMITesterPowerOn(HANDLE device);


