/*
* Copyright 2011, Silicon Motion Technology Corp.
*
*/
#include "StdAfx.h"
#ifdef WIN32
#include "SMIFunc.h"
#include "ntddscsi.h"
#include "stddef.h"
#else
#include "SMIFunc.h"
#endif


/*
 * Search for a specific USB device by vendor and device ID
 */
pUSB_DEVICE find_usb_dev(int vendor_id, int product_id)
#ifdef WIN32
{
	CHAR szRtnLetter=' ';
	BYTE cb[16];
	char szInquiry[0x24];
	pUSB_DEV_HANDLE hDisk;
	CHAR cLetter=' ';
	for(cLetter='D' ; cLetter<='Z' ; cLetter++)
	{
		
		if( (hDisk=usb_open(cLetter)) != INVALID_HANDLE_VALUE)
		{
			memset(szInquiry,0,sizeof(szInquiry));
			memset(cb,0,sizeof(cb));
			cb[0x00] = 0x12;
			cb[0x04] = 0x24;
			usb_scsi_read( hDisk , (BYTE*)szInquiry,0x24, cb , 0x6);
			if(strstr(szInquiry+5,"smi") != NULL){
				szRtnLetter=cLetter;
				break;
			}
			usb_close(hDisk);
		}
	}
	return szRtnLetter;
}
#else
{
	
	pUSB_BUS bus;
	pUSB_DEVICE	dev;
	int num_bus;
	int num_dev;

	num_bus = usb_find_busses();
	num_dev = usb_find_devices();

	for (bus = usb_busses; bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) 
		{

//			printf("dev->descriptor.idVendor=%lx,dev->descriptor.idProduct=%lx\n",dev->descriptor.idVendor,dev->descriptor.idProduct);

			if ((dev->descriptor.idVendor == vendor_id) &&
			    (dev->descriptor.idProduct == product_id))
			    {
				return dev;
			}
		}
	}
	return NULL;
}
#endif

/*
 * Implement all non-Linux based functions here
*/
#ifdef WIN32
pUSB_DEV_HANDLE usb_open(pUSB_DEVICE dev){
	pUSB_DEV_HANDLE tempHandle;
	CHAR DiskName[32];
	sprintf( DiskName, "\\\\.\\%c:", dev);
	// ==================================
	tempHandle = CreateFile(	DiskName, 
						GENERIC_READ|GENERIC_WRITE, 
				  		FILE_SHARE_READ|FILE_SHARE_WRITE, 
						NULL, 
						OPEN_EXISTING, 
						FILE_FLAG_NO_BUFFERING, 
						NULL );

	if(tempHandle !=INVALID_HANDLE_VALUE)
	{
//		pUSB_DEV_HANDLE Handle = (HANDLE)::GlobalAlloc(LMEM_ZEROINIT , sizeof(HANDLE));
//		Handle = tempHandle;
		return tempHandle;
	}else
	{
		return NULL;
	}
	return NULL;
}
int usb_close(pUSB_DEV_HANDLE udev){
	CloseHandle(udev);
//	if(!(::GlobalFree(udev)) ){
//		return true;
//	}
	return false;
}
int usb_detach_kernel_driver_np(pUSB_DEV_HANDLE udev , int n){
	return true;
}
int usb_set_configuration(pUSB_DEV_HANDLE udev , int n){
	return true;
}
int usb_claim_interface(pUSB_DEV_HANDLE udev , int n){
	return true;
}
int usb_set_altinterface(pUSB_DEV_HANDLE udev , int n){
	return true;
}
int usb_release_interface(pUSB_DEV_HANDLE udev , int n){
	return true;
}

typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER 
{
    SCSI_PASS_THROUGH_DIRECT sptd;
    ULONG             Filler;      // realign buffer to double word boundary
    UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;

#endif

/*
 * Open and configure a USB device for bulk operation
 */
pUSB_DEV_HANDLE  bulk_usb_open(pUSB_DEVICE dev)
{
	pUSB_DEV_HANDLE udev = usb_open(dev);
	if (!udev)
		return NULL;

	/*
	 * Try to detach a current driver if one exists, but don't print a
	 * warning as often times a driver won't be attached
	 */
	usb_detach_kernel_driver_np(udev, 0);

	if (usb_set_configuration(udev, 1) < 0)
		printf("WARNING: Couldn't set USB configuration!\n");

	if (usb_claim_interface(udev, 0) < 0)
		printf("WARNING: Couldn't set USB interface!\n");

	if (usb_set_altinterface(udev, 0))
		printf("WARNING: Couldn't set USB altinterface!\n");

	return udev;
}


/*
 * Close a USB device
 */
int bulk_usb_close(pUSB_DEV_HANDLE udev)
{
	if (usb_release_interface(udev, 0) < 0)
		printf("WARNING: Couldn't release USB interface!\n");

	return usb_close(udev);
}

#ifndef WIN32

void append_le_uint32(uint32_t *buffer, unsigned int i)
{
		unsigned char *p = (unsigned char *)buffer;
    p[0] = i;
    p[1] = i >> 8;
    p[2] = i >> 16;
    p[3] = i >> 24;
}

unsigned int read_le_uint32(uint32_t *buffer)
{
    unsigned int i = 0;
    unsigned char *p = (unsigned char *)buffer;
    i |= (p[0]);
    i |= (p[1]) << 8;
    i |= (p[2]) << 16;
    i |= (p[3]) << 24;
    return i;
}

/*
 * Send a Command Block Wrapper (CBW)
 */
int storage_send_cbw(usb_dev_handle *devh, unsigned int tag,
                            int length, unsigned char flags,
                            unsigned char *cb, int cb_length)
{
	_sCBWStruct_ cbw;

	append_le_uint32(&cbw.signature, CBW_SIGNATURE);
	append_le_uint32(&cbw.tag, tag);
	append_le_uint32(&cbw.data_trans_len, tag);
	cbw.flags = flags;
	cbw.lun = 0;
	cbw.len = cb_length;
	memcpy(&cbw.cmd, cb, cb_length);

	return usb_bulk_write(devh, SM3255_ENDPOINT_OUT_ADDR, 
			      (char*)&cbw, sizeof(cbw), USB_BULK_TX_TIMEOUT);
}


/*
 * Perform a USB bulk SCSI read
 */
int usb_scsi_read(usb_dev_handle *devh, unsigned char *buf,
                         int length, unsigned char *cb, int cb_length)
{
	int ret;
	static unsigned int tag = 0x1234;
	_sCSBStruct_ csw;
	unsigned char packet[USB_BULK_RX_SIZE];
	tag++;

	ret = storage_send_cbw(devh, tag, length, CBW_DIR_IN, cb, cb_length);
	if (ret != sizeof(_sCBWStruct_)) {
		printf("storage_send_cbw: returned %d, expected %d\n",
		       ret, sizeof(_sCBWStruct_));
		return 0;
	}

retry:
	/* Do not send read command if length is 0 */
	if (length != 0) {
		ret = usb_bulk_read(devh, SM3255_ENDPOINT_IN_ADDR, (char*) packet,
				    sizeof(packet), USB_BULK_RX_TIMEOUT);
		if (ret < 0) {
			perror("usb_bulk_read");
			return ret;
		}

		if (ret > length) {
			printf("usb_bulk_read: long read\n");
			return 0;
		}

		if (ret > 0)
			memcpy(buf, packet, ret);

		if (ret != length) {
			if (ret != sizeof(packet)) {
				printf("usb_bulk_read: short read\n");
				return 0;
			}

			/* We need more packets */
			length -= sizeof(packet);
			buf += sizeof(packet);
			goto retry;
		}
	}

	memset(&csw, 0, sizeof(csw));
	ret = usb_bulk_read(devh, SM3255_ENDPOINT_IN_ADDR, (char*)&csw, 
			    sizeof(csw), USB_BULK_RX_TIMEOUT);
	if (ret < 0) {
		perror("usb_bulk_read CSW");
		return ret;
	}

	if (ret != sizeof(csw)) {
		fprintf(stderr, "usb_bulk_read CSW: short read\n");
		return 0;
	}

	/* Fixup endianess of response data */
	csw.signature = read_le_uint32(&csw.signature);
	csw.tag = read_le_uint32(&csw.tag);

	if (csw.signature != CSW_SIGNATURE) {
		printf("usb_bulk_read CSW: invalid signature 0x%08x\n",
		       csw.signature);
		return 0;
	}

	/*if (csw.tag != tag) {
		printf("usb_bulk_read CSW: invalid tag (got 0x%08x, "
		       "expected 0x%08x)\n", csw.tag, tag);
		return 0;
	}*/

	if (csw.status) {
		printf("usb_bulk_read: command failed with status 0x%02x\n"
		       , csw.status);
		return 0;
	}

	return 1;
}


/*
 * Perform a USB bulk SCSI write
 */
int usb_scsi_write(usb_dev_handle *devh, unsigned char *buf,
                          int length, unsigned char *cb, int cb_length)
{
	int ret;
	static unsigned int tag = 0x4321;
	_sCSBStruct_ csw;
	tag++;

	ret = storage_send_cbw(devh, tag, length, CBW_DIR_OUT, cb, cb_length);
	if (ret != sizeof(_sCBWStruct_)) {
		printf("storage_send_cbw: returned %d, expected %d\n",
		       ret, sizeof(_sCBWStruct_));
		return 0;
	}

retry:
	if(cb[0x00] == 0xF1 && cb[0x01] == 0x27){
		printf("Pretest code downloading...\n");
		ret = usb_bulk_write(devh, SM3255_ENDPOINT_OUT_ADDR, (char*)buf, 
				     USB_BULK_TX_SIZE, USB_PRETEST_TIMEOUT);
	}
	else
	ret = usb_bulk_write(devh, SM3255_ENDPOINT_OUT_ADDR, (char*)buf, 
			     USB_BULK_TX_SIZE, USB_BULK_TX_TIMEOUT);
	if (ret != USB_BULK_TX_SIZE) {
		if (ret != length) {
			printf("usb_bulk_write: returned %d, expected %d\n",
			       ret, length);
			return 0;
		}

		printf("usb_bulk_write: returned %d, expected %d\n", 
		       ret, USB_BULK_TX_SIZE);
		return 0;
	}

	length -= USB_BULK_TX_SIZE;

	if (length > 0) {
		buf += USB_BULK_TX_SIZE;
		goto retry;
	}

	memset(&csw, 0, sizeof(csw));
	ret = usb_bulk_read(devh, SM3255_ENDPOINT_IN_ADDR, (char*)&csw, 
			    13, USB_BULK_RX_TIMEOUT);
	if (ret < 0) {
		perror("usb_bulk_read CSW");
		return ret;
	}
	if (ret != sizeof(csw)) {
		fprintf(stderr, "usb_bulk_read CSW: short read\n");
		return 0;
	}

	/* Fixup endianess of response data */
	csw.signature = read_le_uint32(&csw.signature);
	csw.tag = read_le_uint32(&csw.tag);

	if (csw.signature != CSW_SIGNATURE) {
		printf("usb_bulk_write CSW: invalid signature 0x%08x\n",
		       csw.signature);
		return 0;
	}

	/*if (csw.tag != tag) {
		printf("usb_bulk_write CSW: invalid tag (got 0x%08x, "
		       "expected 0x%08x)\n", csw.tag, tag);
		return 0;
	}*/

	if (csw.status) {
		printf("usb_bulk_write: command failed with status 0x%02x\n"
		       , csw.status);
		return 0;
	}

	return 1;
}
#else
int usb_scsi_write(usb_dev_handle *devh, unsigned char *buf,
                   int length, unsigned char *cb, int cb_length)
{
	int i;
      SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	  ZeroMemory(&sptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
      sptdwb.sptd.Length= sizeof(SCSI_PASS_THROUGH_DIRECT);
      sptdwb.sptd.PathId = 0;
      sptdwb.sptd.TargetId = 1;
      sptdwb.sptd.Lun = 0;
      sptdwb.sptd.CdbLength = cb_length;//CDB10GENERIC_LENGTH;
      sptdwb.sptd.SenseInfoLength = 0x00;
      sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_OUT;
      sptdwb.sptd.DataTransferLength = length;
      sptdwb.sptd.TimeOutValue = 300;
      sptdwb.sptd.DataBuffer = buf;
      sptdwb.sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);
	  for(i=0;i<cb_length;i++)
	  {
        sptdwb.sptd.Cdb[i] = cb[i];
	  }	
	  

      ULONG llength = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	
	  ULONG	returned = 0;
	  BOOL	success;

	  if( devh != INVALID_HANDLE_VALUE )
	  {
         success = DeviceIoControl(
					   devh,
					   IOCTL_SCSI_PASS_THROUGH_DIRECT,
					   &sptdwb,
					   llength,
					   &sptdwb,
					   llength,
					   &returned,
					   FALSE );
		 FlushFileBuffers( devh);
	  }
	  
	  return success;

	return true;
}

int usb_scsi_read(usb_dev_handle *devh, unsigned char *buf,
                  int length, unsigned char *cb, int cb_length)
{
	int i;

		SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	    ZeroMemory(&sptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
        sptdwb.sptd.Length= sizeof(SCSI_PASS_THROUGH_DIRECT);
        sptdwb.sptd.PathId = 0;
        sptdwb.sptd.TargetId = 1;
        sptdwb.sptd.Lun = 0;
        sptdwb.sptd.CdbLength = cb_length;
        sptdwb.sptd.SenseInfoLength = 0x00;
        sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_IN;
        sptdwb.sptd.DataTransferLength = length;
        sptdwb.sptd.TimeOutValue = 300;
        sptdwb.sptd.DataBuffer = buf;
        sptdwb.sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);

		for(i=0;i<cb_length;i++)
		{
          sptdwb.sptd.Cdb[i] = cb[i];
		}
		
        ULONG llength = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	    ULONG	returned = 0;
	    BOOL	success;

	    if( devh != INVALID_HANDLE_VALUE )
		{
         success = DeviceIoControl(
					   devh,
					   IOCTL_SCSI_PASS_THROUGH_DIRECT,
					   &sptdwb,
					   llength,
					   &sptdwb,
					   llength,
					   &returned,
					   FALSE );
	      FlushFileBuffers( devh);
		}

	  return success;

	return true;
}
#endif
