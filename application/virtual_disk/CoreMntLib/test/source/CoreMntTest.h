#include <stdio.h>
#include <tchar.h>

#include <iostream>
#include <conio.h>

#define LOGGER_LEVEL LOGGER_LEVEL_DEBUGINFO
#define LOG_OUT_CLASS_SIZE
#include <stdext.h>
#include <jcapp.h>

#include "../../include/mntSyncMountmanager.h"
#include "../../include/mntImage.h"

class CCoreMntTestApp : public jcapp::CJCAppSupport<jcapp::AppArguSupport>
{
public:
	CCoreMntTestApp(void);

public:
	virtual int Initialize(void);
	virtual int Run(void);
	virtual void CleanUp(void);
	virtual LPCTSTR AppDescription(void) const { return 
		_T("CoreMnt Management Utility\n")
		_T("\t by Jingcheng Yuan\n");
	};

protected:
	void CreateParameter(jcparam::CParamSet * &param);
	void InstallDriver(CSyncMountManager & mntmng,const CJCStringT & driver);
	void LoadUserModeDriver(const CJCStringT & driver, IImage * & img);

	//void StartTestPort(ITestAuditPort * test_port);
	static DWORD WINAPI StaticStartTestPort(LPVOID param);
	void RunTestPort(ITestAuditPort * test_port);

public:
	static const TCHAR LOG_CONFIG_FN[];
	// 挂载点，如果没有指定，则不挂载
	TCHAR	m_mnt_point;
	// 用于和驱动通信的device id，如果没有指定device id，则创建device
	UINT	m_dev_id;
	UINT	m_remove;	// remove dead device
	// image文件大小，单位字节
	FILESIZE	m_file_size;

	CJCStringT	m_filename;
	bool	m_connect;
	HANDLE	m_exit_event;
	CJCStringT	m_driver;
	CJCStringT	m_device_name;
	CJCStringT	m_config;		// file name of configuration.
	bool	m_install_driver, m_uninstall_driver;

	HANDLE m_test_thread;
	ITestAuditPort * m_test_port;

protected:
	enum DRIVER_STATUS
	{	
		ST_INIT	=0,	ST_DRV_INSTALLED =1, 
		ST_DRV_LOADED=2, 
		ST_DRV_CONNECTED=3,
		ST_MOUNTED=4
	};
protected:
	CSyncMountManager	m_mount_manager;
	CJCStringT			m_app_path;
	DRIVER_STATUS				m_status;
};

typedef jcapp::CJCApp<CCoreMntTestApp>	CApplication;
