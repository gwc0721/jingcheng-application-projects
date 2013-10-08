
#include "config.h"
#include <stdext.h>
#include "SM3257EN.h"

LOCAL_LOGGER_ENABLE(_T("CSM3257EN"), LOGGER_LEVEL_DEBUGINFO);

CSM3257EN::CSM3257EN(IUsbMassStorage * dev)
	: m_dev(dev)
{
	if (m_dev == NULL) THROW_ERROR(ERR_APP, "invalid device");
}

CSM3257EN::~CSM3257EN(void)
{
	if (m_dev) delete m_dev;
}
	
bool CSM3257EN::ReadFlashIDBuffer(BYTE * buf, int & buf_len)
{
	ASSERT(m_dev);
	ASSERT(buf);

	BYTE cmd[16];
	memset(cmd, 0 , sizeof(cmd));
	cmd[0x00] = 0xF0;
	cmd[0x01] = 0x06;
	cmd[0x0B] = 0x01;
	if (buf_len > 512) buf_len = 512;
	return m_dev->UsbScsiRead(buf, buf_len, cmd, 0x10);

}

bool CSM3257EN::GetISPVer(CStringA & ver)
{
	LOG_STACK_TRACE()

	ASSERT(m_dev);
	stdext::auto_array<BYTE>	buf(512);

	BYTE cmd[16];
	memset(cmd, 0 , sizeof(cmd));
	cmd[0x00] = 0xF0;
	cmd[0x01] = 0x2A;
	cmd[0x0B] = 0x01;

	m_dev->UsbScsiRead(buf, 512, cmd, 0x10);
	ver = (char*)(buf+0x190);
	return true;
}
