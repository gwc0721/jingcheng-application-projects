#include "stdafx.h"
#include "smi_disk.h"
#include <stdext.h>

#include "ntddscsi.h"
//#include "Shlwapi.h"

//#include <Setupapi.h>
//#include <cfgmgr32.h>
//#include <winioctl.h>
//#include "sm224testB.h"
//#include "setupapi.h"

#define TIME_OUT			60	

//=======================================================================================================================//
//=======================================================================================================================//
//=======================================================================================================================//
//=== The Local Parameters Definition ===// ===>>>

#define SPT_SENSE_LENGTH	32
#define SPTWB_DATA_LENGTH	4*512
#define CDB6GENERIC_LENGTH	6
#define CDB10GENERIC_LENGTH	10
#define CDB16GENERIC_LENGTH	16

typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS 
{
    SCSI_PASS_THROUGH spt;
    ULONG             Filler;      // realign buffers to double word boundary
    UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
    UCHAR             ucDataBuf[SPTWB_DATA_LENGTH];
} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;

typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER 
{
    SCSI_PASS_THROUGH_DIRECT sptd;
    ULONG             Filler;      // realign buffer to double word boundary
    UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;

bool Send_Initial_Card_Command(HANDLE hDisk, BOOL enable)
{
	BYTE Command[16];
	memset(Command,0,16);
	Command[0x00] = 0xF2;
	Command[0x01] = 0x80;
	Command[0x02] = enable;
	SMISCSIReadTester(hDisk, NULL, 0, Command); 
	Sleep(3000);
	return true;
}


bool Send_Inquiry_XP_SM333(HANDLE device, UCHAR *buf, DWORD buf_len)
{
	JCASSERT(NULL != device && INVALID_HANDLE_VALUE != device);

	SCSI_PASS_THROUGH_WITH_BUFFERS sptwb;
    ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));

    sptwb.spt.Length	= sizeof(SCSI_PASS_THROUGH);
    sptwb.spt.PathId	= 0;
    sptwb.spt.TargetId	= 1;
    sptwb.spt.Lun	= 0;
    sptwb.spt.CdbLength = CDB6GENERIC_LENGTH;
    sptwb.spt.SenseInfoLength	= 0x00;
    sptwb.spt.DataIn	= SCSI_IOCTL_DATA_IN;
    sptwb.spt.DataTransferLength= 0x24;
    sptwb.spt.TimeOutValue	= 2;
    sptwb.spt.DataBufferOffset	= (ULONG)offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf);
    sptwb.spt.SenseInfoOffset	= (ULONG)offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucSenseBuf);
    sptwb.spt.Cdb[0] = 0x12;
    sptwb.spt.Cdb[4] = 0x24;

    ULONG length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) + sptwb.spt.DataTransferLength;
	
	ULONG	returned = 0;
	BOOL	success;
    success = DeviceIoControl(device,
		IOCTL_SCSI_PASS_THROUGH,
		&sptwb, sizeof(SCSI_PASS_THROUGH),
		&sptwb, length,
		&returned, FALSE );
	
	// Vendor Information: Offset - 0x08
	if( !success) return false;
	memcpy( buf, sptwb.ucDataBuf + 0x05, 0x27 );
	//memcpy( pRMB,sptwb.ucDataBuf,5);
   
	if((buf[0] == 's') && (buf[1] == 'm') && (buf[2] == 'i') )		success = TRUE;
	else	success = FALSE;
	return	(success != FALSE);	
}

BOOL Send_Inquiry_Tester(HANDLE device, UCHAR *pDataBuffer, DWORD buf_len, SMI_TESTER_TYPE & tester)
{
	JCASSERT(NULL != device && INVALID_HANDLE_VALUE != device);
	UCHAR   Command[16];
	BOOL	success = FALSE;

	tester = NON_SMI_TESTER;

	// Valid Device Handle
	// Judge whether SMI 333 controller
	DWORD 	dummy;
	DeviceIoControl(device, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &dummy, NULL);

	// SMI333
	memset(Command,0,sizeof(Command));
	Command[0x00] = 0xF0;
	Command[0x01] = 0x83;
	Command[0x06] = 0x01;
	Command[0x0B] = 0x01;
	Command[0x0F] = 0x01;

	success = SMISCSIReadTester(device, pDataBuffer, buf_len, Command); 
			
	if(success)
	{
		if(pDataBuffer[0x1E]==0x55 && pDataBuffer[0x1F]==0xAA) 
		{
			success = TRUE;
			tester = TESTER_SM333;
		}
		else
		{	// SM334
			memset(Command,0,sizeof(Command));
			Command[0x00] = 0xF8;
			Command[0x01] = 0x0;
			Command[0x06] = 0x0;
			Command[0x0B] = 0x01;
			Command[0x0F] = 0x0;

			success=SMISCSIReadTester(device, pDataBuffer, buf_len, Command); 
			if(success)
			{
				success = FALSE;	
				tester = TESTER_SM334;
			}
		}
	}
	else
	{	// SM334
		memset(Command,0,sizeof(Command));
		Command[0x00] = 0xF8;
		Command[0x01] = 0x0;
		Command[0x06] = 0x0;
		Command[0x0B] = 0x01;
		Command[0x0F] = 0x0;

		success=SMISCSIReadTester(device, pDataBuffer, buf_len, Command); 
		if(success)
		{
			success = FALSE;	
			tester = TESTER_SM334;
		}
	}
							
	DeviceIoControl(device, FSCTL_UNLOCK_VOLUME,NULL, 0, NULL, 0, &dummy, NULL);
	return success;	
}

bool TestOpen(CHAR Letter,HANDLE *hDisk)
{
	CHAR DiskName[64];
	int retrycnt=0;
	*hDisk=INVALID_HANDLE_VALUE;

	sprintf_s( DiskName, "\\\\.\\%c:", Letter );
	while(*hDisk==INVALID_HANDLE_VALUE)
	{
		*hDisk = CreateFileA(DiskName, 
							GENERIC_READ|GENERIC_WRITE, 
				  			FILE_SHARE_READ|FILE_SHARE_WRITE, 
							NULL, 
							OPEN_EXISTING, 
							FILE_FLAG_NO_BUFFERING, 
							NULL );
		if(*hDisk ==INVALID_HANDLE_VALUE) 
		{
			Sleep(1500);
			if(retrycnt++>20)	return false;
		}
	}

	if(*hDisk !=INVALID_HANDLE_VALUE)
	{
		DWORD   dummy;
		BOOL    success;
		success=DeviceIoControl(*hDisk,FSCTL_LOCK_VOLUME,NULL, 0, NULL, 0, &dummy, NULL);
		return true;
	}
	else	return true;
}

BOOL TestClose(HANDLE hDisk)
{
	BOOL success = TRUE;
	DWORD dummy;

	if(hDisk != INVALID_HANDLE_VALUE)
	{ 
		success=DeviceIoControl(hDisk, FSCTL_UNLOCK_VOLUME,NULL, 0, NULL, 0, &dummy, NULL);
        CloseHandle(hDisk);
	}
	return success; 
}

ULONG SMIGetSize(HANDLE hDevice)
{
	ULONG TotalSector;        
    UCHAR   Buf[8];
	SCSI_PASS_THROUGH_WITH_BUFFERS sptwb;
    ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));
    sptwb.spt.Length	= sizeof(SCSI_PASS_THROUGH);
    sptwb.spt.PathId	= 0;
    sptwb.spt.TargetId	= 1;
    sptwb.spt.Lun	= 0;
    sptwb.spt.CdbLength = CDB16GENERIC_LENGTH;
    sptwb.spt.SenseInfoLength	= 0x00;
    sptwb.spt.DataIn	= SCSI_IOCTL_DATA_IN;
    sptwb.spt.DataTransferLength= 8;
    sptwb.spt.TimeOutValue	= 5;
    sptwb.spt.DataBufferOffset	= (ULONG)offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf);
    sptwb.spt.SenseInfoOffset	= (ULONG)offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucSenseBuf);
    sptwb.spt.Cdb[0] = 0x25;

    ULONG length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) + sptwb.spt.DataTransferLength;
	ULONG	returned = 0;
    BOOL	success=0;

    if( hDevice != INVALID_HANDLE_VALUE )
	{

     success = DeviceIoControl(	
			   hDevice,
			   IOCTL_SCSI_PASS_THROUGH,
			   &sptwb,
			   sizeof(SCSI_PASS_THROUGH),
			   &sptwb,
			   length,
			   &returned,
			   FALSE );	

	 memcpy( Buf, sptwb.ucDataBuf, 8 );
	 TotalSector=(Buf[0]<<24)+(Buf[1]<<16)+(Buf[2]<<8)+Buf[3];
	 return TotalSector;
	}
    return success;
}

bool SMITesterPowerOff(HANDLE device, SMI_TESTER_TYPE tester)
{
	JCASSERT( NULL != device && INVALID_HANDLE_VALUE != device);
	UCHAR Command[16];
	switch (tester)
	{
	case TESTER_SM333:	
		// power off
		memset(Command,0,16);
		Command[0x00] = 0x1B;
		Command[0x01] = 0x00;
		Command[0x04] = 0x02;
		SCSICommandReadTester(device,NULL,0,Command);
		break;

	case TESTER_SM334:
		memset(Command,0,16);
		Command[0x00] = 0x1B;
		Command[0x01] = 0x00;
		Command[0x04] = 0x02;
		SCSICommandReadTester(device,NULL,0,Command);
		break;
	default:
		return false;
	}
	return true;
}

bool SMITesterPowerOn(HANDLE device)
{
	JCASSERT( NULL != device && INVALID_HANDLE_VALUE != device);
	UCHAR Command[16];
	// power on
	memset(Command,0,16);
	Command[0x00] = 0x1B;
	Command[0x01] = 0x00;
	Command[0x04] = 0x00;
	SCSICommandReadTester(device,NULL,0,Command);

	// confirm power on	
	memset(Command,0,16);
	Command[0x00] = 0x00;
	Command[0x01] = 0x00;
	Command[0x04] = 0x00;
	for(int ii=0; ii<20; ii++)
	{	// confirm power on
		BOOL br =SCSICommandReadTester(device,NULL,0,Command);
		if ( SMIGetSize(device) > 0 )		return true;
		Sleep(1000);
	}
	return false;
}


BOOL SMISCSIReadTester(HANDLE hDevice, PUCHAR DataBuffer, ULONG BufferSize, UCHAR *Arg)
{
	int i;

	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
    ZeroMemory(&sptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
    sptdwb.sptd.Length= sizeof(SCSI_PASS_THROUGH_DIRECT);
    sptdwb.sptd.PathId = 0;
    sptdwb.sptd.TargetId = 1;
    sptdwb.sptd.Lun = 0;
    sptdwb.sptd.CdbLength = CDB16GENERIC_LENGTH;
    sptdwb.sptd.SenseInfoLength = 0x00;
    sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_IN;
    sptdwb.sptd.DataTransferLength = BufferSize;
    sptdwb.sptd.TimeOutValue = 5;
    sptdwb.sptd.DataBuffer = DataBuffer;
    sptdwb.sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);

	for(i=0;i<16;i++)	sptdwb.sptd.Cdb[i] = Arg[i];
	
    ULONG length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
    ULONG	returned = 0;
    BOOL	success;

    if( hDevice != INVALID_HANDLE_VALUE )
	{

		success = DeviceIoControl(hDevice, IOCTL_SCSI_PASS_THROUGH_DIRECT,
			&sptdwb, length,
			&sptdwb, length,
			&returned, FALSE );
      
		FlushFileBuffers( hDevice);
	}
	return success;
}


BOOL SCSICommandReadTester(HANDLE hDisk,UCHAR *Buffer,int BufferSize , UCHAR *Command)
{
	int i;

	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	ZeroMemory(&sptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
    sptdwb.sptd.Length= sizeof(SCSI_PASS_THROUGH_DIRECT);
    sptdwb.sptd.PathId = 0;
    sptdwb.sptd.TargetId = 1;
    sptdwb.sptd.Lun = 0;
    sptdwb.sptd.CdbLength = CDB16GENERIC_LENGTH;
    sptdwb.sptd.SenseInfoLength = 0x00;
    sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_IN;
    sptdwb.sptd.DataTransferLength = BufferSize;
    sptdwb.sptd.TimeOutValue = TIME_OUT;
    sptdwb.sptd.DataBuffer = Buffer;
    sptdwb.sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);

	for(i=0;i<16;i++)
	{
		sptdwb.sptd.Cdb[i] = Command[i];
	}
		
    ULONG length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	ULONG	returned = 0;
	BOOL	success;

	if( hDisk != INVALID_HANDLE_VALUE )
	{
		success = DeviceIoControl(
					   hDisk,
					   IOCTL_SCSI_PASS_THROUGH_DIRECT,
					   &sptdwb,
					   length,
					   &sptdwb,
					   length,
					   &returned,
					   FALSE );
		FlushFileBuffers( hDisk);
	}

	return success;	
}




