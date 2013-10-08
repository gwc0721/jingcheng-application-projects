/*
* Copyright 2011, Silicon Motion Technology Corp.
*
*/
#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <stdint.h>
#include <usb.h>
#include <getopt.h>
#else
#include <windows.h>
#include <winioctl.h>
#include <ctype.h>
#include <memory.h>
#include "scsidefs.h"
#include "wnaspi32.h"
#include <cfgmgr32.h>
#endif

#include <errno.h>
#include <string.h>

#pragma pack(push, 1)
#pragma pack(1)

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
#define USB_BULK_RX_SIZE	512

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
//=== Parameter's definition ===//
#define UCHAR							uint8_t
#define BYTE							uint8_t
#define DWORD							unsigned long int
#define CHAR							char
#define true							1
#define false							0
#define BOOL							int

#endif
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

//Chapter 4. Host-Side Data Types and Macros
#ifndef WIN32
typedef struct usb_device _sUSB_DEVICE_,*pUSB_DEVICE;

typedef struct usb_bus	_sUSB_BUS_,*pUSB_BUS;

typedef struct usb_dev_handle _sUSB_DEV_HANDLE,*pUSB_DEV_HANDLE;//usb_dev_handle

#else
#define SPT_SENSE_LENGTH	32
#define SPTWB_DATA_LENGTH	4*512
#define CDB6GENERIC_LENGTH	6
#define CDB10GENERIC_LENGTH	16
#define sleep Sleep
typedef char pUSB_DEVICE;
typedef void usb_dev_handle;
typedef HANDLE pUSB_DEV_HANDLE;
#define __func__	"__FUNC__"
#define __rfile__	"__RFile__"


#endif
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^Parameter/Structure Definitions ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^//


pUSB_DEVICE find_usb_dev(int vendor_id, int product_id);

pUSB_DEV_HANDLE  bulk_usb_open(pUSB_DEVICE dev);

pUSB_DEV_HANDLE usb_open(pUSB_DEVICE dev);

int usb_close(pUSB_DEV_HANDLE udev);

int bulk_usb_close(pUSB_DEV_HANDLE udev);

int usb_scsi_read (usb_dev_handle *devh, unsigned char *buf, int length, unsigned char *cb, int cb_length);

int usb_scsi_write(usb_dev_handle *devh, unsigned char *buf, int length, unsigned char *cb, int cb_length);
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ API's Definitions ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^//

#pragma pack(pop)
