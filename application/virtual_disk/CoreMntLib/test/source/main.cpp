
#include <stdio.h>
#include <tchar.h>

#include <iostream>
#include <conio.h>

#define LOGGER_LEVEL LOGGER_LEVEL_DEBUGINFO

#include <vld.h>
#include <stdext.h>
#include <jcapp.h>

LOCAL_LOGGER_ENABLE(_T("CoreMntTest"), LOGGER_LEVEL_DEBUGINFO);

#include "../../include/mntFileImage.h"
#include "../../include/mntSparseImage.h"
#include "../../include/mntSyncMountmanager.h"

#define _STATIC_CPPLIB
#define BLOCK_LENGTH         0x1000


class CCoreMntTestApp : public jcapp::CJCAppBase<jcapp::AppArguSupport>
{
public:
	CCoreMntTestApp(void);

public:
	bool Initialize(void);
	virtual int Run(void);
	void CleanUp(void);

protected:
	void CreateParameter(jcparam::CParamSet * &param);

public:
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

	HMODULE	m_driver_module;

protected:
};

typedef jcapp::CJCApp<CCoreMntTestApp>	CApplication;
static CApplication the_app;
#define _class_name_	CApplication

BEGIN_ARGU_DEF_TABLE()
	ARGU_DEF_ITEM(_T("driver"),		_T('p'), CJCStringT,	m_driver,		_T("load user mode driver.") )
	ARGU_DEF_ITEM(_T("mount"),		_T('m'), TCHAR,			m_mnt_point,	_T("mount point.") )
	ARGU_DEF_ITEM(_T("deviceid"),	_T('d'), UINT,			m_dev_id,		_T("deviceid.") )
	ARGU_DEF_ITEM(_T("filesize"),	_T('s'), FILESIZE,		m_file_size,	_T("image file size in sectors.") )
	ARGU_DEF_ITEM(_T("filename"),	_T('f'), CJCStringT,	m_filename,		_T("image file name.") )
	ARGU_DEF_ITEM(_T("connect"),	_T('c'), bool,			m_connect,		_T("connect user mode driver to device.") )
	ARGU_DEF_ITEM(_T("remove"),		_T('r'), UINT,			m_remove,		_T("remove dead device.") )
	//ARGU_DEF_ITEM(_T("open"),	_T('o'), CJCStringT,	m_device, _T("image file size in byte.") )

	//ARGU_DEF_ITEM(_T("length"),		_T('l'), int,		m_length, _T("set length.") )
END_ARGU_DEF_TABLE()

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	if(dwCtrlType == CTRL_C_EVENT)
	{
		LOG_DEBUG(_T("Ctrl + C has been pressed."));
		SetEvent(the_app.m_exit_event);
	}
	else
	{
		LOG_DEBUG(_T("console event 0x%X"), dwCtrlType);
	}
	return TRUE;
}

//#define LOCAL_DEBUG

CCoreMntTestApp::CCoreMntTestApp(void) 
	: jcapp::CJCAppBase<jcapp::AppArguSupport>(ARGU_SUPPORT_HELP | ARGU_SUPPORT_OUTFILE)
	, m_dev_id(UINT_MAX), m_file_size(0x100000LL), m_mnt_point(0)
	, m_exit_event(NULL), m_connect(false), m_remove(UINT_MAX)
	, m_driver_module(NULL) 
{
}

int CCoreMntTestApp::Run(void)
{
	LOG_STACK_TRACE();
	SetConsoleCtrlHandler(HandlerRoutine, TRUE);

	stdext::auto_interface<IImage>	img;

	if ( m_driver.empty() )		THROW_ERROR(ERR_USER, _T("driver must be set."));
	// load driver
	// get app dir
	CJCStringT drv_path;
	__super::GetAppPath(drv_path);

	drv_path += (CJCStringT(_T("\\")) + m_driver + _T(".dll") );
	m_driver_module = LoadLibrary(drv_path.c_str());
	if (m_driver_module == NULL) THROW_WIN32_ERROR(_T(" failure on loading driver %s "), drv_path.c_str() );

	// load entry
	GET_DRV_FACT_PROC proc = (GET_DRV_FACT_PROC) (GetProcAddress(m_driver_module, "GetDriverFactory") );
	if (proc == NULL)	THROW_WIN32_ERROR(_T("file %s is not a virtual disk driver."), drv_path.c_str() );

	stdext::auto_interface<IDriverFactory> factory;
	BOOL br = (proc)(CJCLogger::Instance(), factory);
	if (!br) THROW_ERROR(ERR_APP, _T("failure on getting factory."));
	JCASSERT( factory.valid() );

	// create parameters
	if (m_filename.empty() )	m_filename = L"tst_img01";
	stdext::auto_interface<jcparam::CParamSet> param;
	CreateParameter(param);

	factory->CreateDriver(_T(""), static_cast<jcparam::IValue*>(param), img);
	JCASSERT( img.valid() );

	if (m_remove < UINT_MAX)
	{
		CDriverControl * driver = new CDriverControl(m_remove, true);
		driver->Disconnect();
		delete driver;
		return 0;
	}

#ifdef LOCAL_DEBUG
	LOG_DEBUG(_T("open image file: %s"), m_filename.c_str() );
	img = new FileImage(m_filename.c_str(), m_file_size );

	//std::auto_ptr<IImage> img(new FileImage(m_filename.c_str() ) );
 //   std::auto_ptr<IImage> sparse(new SparseImage(img, 0, m_file_size * SECTOR_SIZE, BLOCK_LENGTH, 512, true));
#endif

#ifndef LOCAL_DEBUG
	CSyncMountManager mount_manager;
	m_dev_id = mount_manager.CreateDevice(m_file_size);		// length in sectors;

	_tprintf( _T("device: %s%d was created.\n") SYMBO_DIRECT_DISK, m_dev_id);
	//sparse.get()->SetId(m_dev_id);

	if (m_connect)
	{
		LOG_DEBUG(_T("connect to device %d"), m_dev_id);
		mount_manager.Connect(m_dev_id, img);
	}

	if (m_mnt_point)
	{
		LOG_DEBUG(_T("mount device %d to %c:"), m_dev_id, m_mnt_point);
		mount_manager.MountDriver(m_dev_id, m_mnt_point);
	}
#endif

	printf("press ctrl+c for unmount...\n");
	//_getch();
	
#ifndef LOCAL_DEBUG
	WaitForSingleObject(m_exit_event, -1);
	LOG_DEBUG(_T("unmount device"))
	mount_manager.Disconnect(m_dev_id);
	mount_manager.UnmountImage(m_dev_id);
#else
	_getch();
#endif

	return 0;
}

void CCoreMntTestApp::CreateParameter(jcparam::CParamSet * &param)
{
	JCASSERT(param == NULL);
	jcparam::CParamSet::Create(param);

	jcparam::IValue * val = NULL;

	//m_cmd_parser.m_remain.GetSubValue(_T("filename"), val);
	val = jcparam::CTypedValue<CJCStringT>::Create(m_filename);
	param->SetSubValue(_T("FILENAME"), val);
	val->Release(); val = NULL;

	val = jcparam::CTypedValue<ULONG64>::Create(m_file_size);
	//m_cmd_parser.m_remain.GetSubValue(_T("filesize"), val);
	param->SetSubValue(_T("FILESIZE"), val);
	val->Release(); val = NULL;
}


bool CCoreMntTestApp::Initialize(void)
{
	m_exit_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_exit_event == NULL) THROW_WIN32_ERROR(_T("failure on creating event."));
	return true;
}

void CCoreMntTestApp::CleanUp(void)
{
	CloseHandle(m_exit_event);
	FreeLibrary(m_driver_module);
	__super::CleanUp();
}

int _tmain(int argc, _TCHAR* argv[])
{
	int ret_code = 0;
	try
	{
		the_app.Initialize();
		ret_code = the_app.Run();
	}
	catch (stdext::CJCException & err)
	{
		stdext::jc_fprintf(stderr, _T("error: %s\n"), err.WhatT() );
		ret_code = err.GetErrorID();
	}
    catch(const std::exception & ex)
    {
        std::cout << ex.what();
    }
	the_app.CleanUp();

	stdext::jc_printf(_T("Press any key to continue..."));
	getc(stdin);
	return ret_code;
}

