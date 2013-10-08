#pragma once

#include "../include/IUsbMassStorage.h"
#include <usb.h>

class CUsbMassStorage: virtual public IUsbMassStorage
{
public:
	CUsbMassStorage(struct usb_device * dev);
public:
	virtual ~CUsbMassStorage(void);


public:
	bool Inquiry(BYTE * buf, int buf_len);
	bool UsbScsiRead(BYTE *buf, int length, BYTE *cb, int cb_length);
	bool UsbScsiWrite();

protected:
	usb_dev_handle * m_dev;
};

#define STRONGPAGETABLESIZE		256
#define VERSION			"0.2"
#define MIN(a,b)	((a<b?a:b))
#define SMI_VENDOR_ID		0x090C
#define SM3255_DEVICE_ID		0x3000
#define SM3255_MEM_DEVICE_ID	0x3000


#define SM3255_ENDPOINT_IN_ADDR	0x81
#define SM3255_ENDPOINT_OUT_ADDR	0x02

#define MAX_READ_SIZE		(256 * 1024)	/* 128 KByte */
#define USB_PRETEST_TIMEOUT	3600000
#define USB_BULK_TX_TIMEOUT	3600000
#define USB_BULK_RX_TIMEOUT	50000

#define USB_BULK_TX_SIZE	512
#define USB_BULK_RX_SIZE	1024

#define CBW_DIR_IN				0x80
#define CBW_DIR_OUT				0x00

#define CSW_SIGNATURE			0x53425355
#define CBW_SIGNATURE			0x43425355


#define debug_enable 1
#define debug if (debug_enable) printf

#ifdef WIN32

#define uint32_t unsigned int
#define uint8_t	 unsigned char

#else

typedef signed char int8_t;
typedef unsigned char   uint8_t;
typedef short  int16_t;
typedef unsigned short  uint16_t;
typedef int  int32_t;
typedef unsigned   uint32_t;
typedef long long  int64_t;
typedef unsigned long long   uint64_t;

#endif

//=== Parameter's definition ===//
#define UCHAR							uint8_t
#define BYTE							uint8_t
#define DWORD							unsigned long int
#define CHAR							char
#define true							1
#define false							0
#define BOOL							int


/* See table 5.1 of USB Mass Storage Class Bulk-Only Transport */
typedef struct cmd_block_wrap_t {
	uint32_t signature;
	uint32_t tag;
	uint32_t data_trans_len;
	uint8_t  flags;
	uint8_t  lun;
	uint8_t  len;
	uint8_t  cmd[16];
}_sCBWStruct_,*pCBWStruct;

/* See table 5.2 of USB Mass Storage Class Bulk-Only Transport */
typedef struct cmd_status_wrap_t {
	uint32_t signature;
	uint32_t tag;
	uint32_t data_residue;
	uint8_t  status;
}_sCSBStruct_,*pCSWStruct;

