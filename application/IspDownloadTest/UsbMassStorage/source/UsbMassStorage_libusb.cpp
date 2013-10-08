
//#include "stdafx.h"
#include "UsbMassStorage_libusb.h"
#include <stdext.h>

LOCAL_LOGGER_ENABLE(_T("CUsbMassStorage"), LOGGER_LEVEL_DEBUGINFO);

CUsbMassStorage::CUsbMassStorage(struct usb_device * dev)
	: m_dev(NULL)
{
	// bulk_usb_open
	usb_dev_handle * dev_hdl = usb_open(dev);
	if (!dev_hdl) THROW_ERROR(ERR_APP, "open usb device failed");

	m_dev = dev_hdl;

	/*
	 * Try to detach a current driver if one exists, but don't print a
	 * warning as often times a driver won't be attached
	 */
//	usb_detach_kernel_driver_np(m_dev, 0);		// for linux only

	if (usb_set_configuration(m_dev, 1) < 0)	
		LOG_WARRING(_T("Couldn't set USB configuration!\n"));

	if (usb_claim_interface(m_dev, 0) < 0)
		LOG_WARRING(_T("Couldn't set USB interface!\n"));

	if (usb_set_altinterface(m_dev, 0))
		LOG_WARRING(_T("Couldn't set USB altinterface!\n"));
}

CUsbMassStorage::~CUsbMassStorage(void)
{
	if (m_dev)
	{
		if (usb_release_interface(m_dev, 0) < 0)
			LOG_WARRING(_T("Couldn't release USB interface!\n"));
		usb_close(m_dev);
	}
	m_dev = NULL;
}

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



bool CUsbMassStorage::UsbScsiRead(BYTE *buf, int buf_len, BYTE *cb, int cb_len)
{
	ASSERT(m_dev);
	ASSERT(buf);
	ASSERT(cb);

	int ret;
	static unsigned int tag = 0x1234;
	_sCSBStruct_ csw;
	tag++;

	stdext::auto_array<char> packet(USB_BULK_RX_SIZE);

	ret = storage_send_cbw(m_dev, tag, buf_len, CBW_DIR_IN, cb, cb_len);
	if (ret != sizeof(_sCBWStruct_) )	THROW_ERROR(ERR_APP, "send cbw unexpected size %d", ret);

	/* Do not send read command if length is 0 */
	while (buf_len > 0) 
	{
		ret = usb_bulk_read(
			m_dev, SM3255_ENDPOINT_IN_ADDR, (char*) packet,
//			buf_len, USB_BULK_RX_TIMEOUT);
			USB_BULK_RX_SIZE, USB_BULK_RX_TIMEOUT);

		if (ret <= 0)		THROW_ERROR(ERR_APP, "usb_bulk_read failed");

		if (ret >= buf_len)
		{
			// partial copy
			memcpy(buf, packet, buf_len);
			buf_len = 0;
		}
		else
		{
			memcpy(buf, packet, ret);
			buf_len -= USB_BULK_RX_SIZE;
			buf += USB_BULK_RX_SIZE;
		}
	}

	memset(&csw, 0, sizeof(csw));
	ret = usb_bulk_read(m_dev, SM3255_ENDPOINT_IN_ADDR, (char*)packet, 
//			    sizeof(csw), USB_BULK_RX_TIMEOUT);
			    512, USB_BULK_RX_TIMEOUT);
	if ( ret <  sizeof(csw)	)	THROW_ERROR(ERR_APP, "read CSW failed");
	memcpy((void*)(&csw), packet, sizeof(csw) );

	return true;

	/* Fixup endianess of response data */
	csw.signature = read_le_uint32(&csw.signature);
	csw.tag = read_le_uint32(&csw.tag);

	if (csw.signature != CSW_SIGNATURE) THROW_ERROR(ERR_APP, "read CSW wrong signature");

	/*if (csw.tag != tag) {
		printf("usb_bulk_read CSW: invalid tag (got 0x%08x, "
		       "expected 0x%08x)\n", csw.tag, tag);
		return 0;
	}*/

	if (csw.status)		THROW_ERROR(ERR_APP, "wrong status 0x%02x", csw.status);
	return true;
}


bool CUsbMassStorage::UsbScsiWrite()
{
	return false;
}

bool CUsbMassStorage::Inquiry(BYTE * buf, int buf_len)
{
	ASSERT(buf);
	ASSERT(m_dev);

	BYTE cb[16];
	memset(cb, 0, 16);
	cb[0] = 0x12;
	cb[4] = 0x24;

	UsbScsiRead(buf, buf_len, cb, 16);
	return true;
}
