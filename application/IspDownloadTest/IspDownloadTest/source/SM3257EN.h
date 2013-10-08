#pragma once
#include <UsbMassStorage.h>
#include <comm_define.h>


class CSM3257EN
{
public:
	CSM3257EN(IUsbMassStorage * dev);
public:
	~CSM3257EN(void);

public:
	bool ReadFlashIDBuffer(BYTE * buf, int & buf_len);
	bool GetISPVer(CStringA & ver);
protected:
	IUsbMassStorage * m_dev;
};
