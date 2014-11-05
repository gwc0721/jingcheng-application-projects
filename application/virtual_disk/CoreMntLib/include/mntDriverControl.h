#pragma once

#include "mntImage.h"

class CDriverControl
{
public:
	CDriverControl(ULONG64 total_sec, const CJCStringT & symbo_link);		// length in sectors
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

protected:
	UINT	m_dev_id;
	HANDLE	m_ctrl;			// device handle for control
	HANDLE  m_thd;	// thread of exchange request
	HANDLE  m_thd_event;	
	IImage	* m_image;		// image interface
	TCHAR	m_mount_point;
};