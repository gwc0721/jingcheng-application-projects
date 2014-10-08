#pragma once

#include "mntImage.h"
#include "../../Comm/virtual_disk.h"

class CDriverControl
{
public:
	CDriverControl(ULONG64 total_sec/*, IImage * image*/);		// length in sectors
	CDriverControl(UINT dev_id, bool dummy);		// length in sectors
	~CDriverControl(void);

	UINT GetDeviceId(void) const {return m_dev_id;}
	void Connect(IImage * image);
	void Mount(TCHAR mnt_point);
	void Disconnect(void);
	void Unmount(void);



protected:
	static DWORD WINAPI StaticRun(LPVOID param); 
	DWORD Run(void);
	void RequestExchange(CORE_MNT_EXCHANGE_REQUEST &request, CORE_MNT_EXCHANGE_RESPONSE &response);
	
	

protected:
	UINT	m_dev_id;
	HANDLE	m_ctrl;			// device handle for control
	//HANDLE	m_exchange;		// handle for exchage request thread
	HANDLE  m_thd;	// thread of exchange request
	HANDLE  m_thd_event;	
	IImage	* m_image;		// image interface
	TCHAR	m_mount_point;
};