#include "stdafx.h"

#include "ferri_comm.h"
#include "sm2244lt.h"
#include "sm2236.h"

LOCAL_LOGGER_ENABLE(_T("ferri_sdk"), LOGGER_LEVEL_NOTICE);

//CFerriFactory	g_factory;
///////////////////////////////////////////////////////////////////////////////
//---- static apis


bool InitializeSdk(LPCTSTR ctrl_name, IFerriFactory * & factory)
{
	JCASSERT(NULL == factory);
	stdext::auto_array<TCHAR> dll_path(MAX_PATH);
	_stprintf_s(dll_path, MAX_PATH-1, _T("SMIDLL_%s.dll"), ctrl_name);

	CFerriFactory * _fac = new CFerriFactory(ctrl_name);
	bool br = _fac->LoadDll(dll_path);

	factory = static_cast<IFerriFactory*>(_fac);
	return true;
}


///////////////////////////////////////////////////////////////////////////////
//---- CFerriFactory
CFerriFactory::CFerriFactory(LPCTSTR ctrl_name)
: m_ref(1), m_dll(NULL), m_ctrl_name(ctrl_name)
{
	memset(&m_apis, 0, sizeof(SMI_API_SET));
}
	
CFerriFactory::~CFerriFactory(void)
{
	if (m_dll)	FreeLibrary(m_dll);
}

bool CFerriFactory::CreateDevice(HANDLE dev, IFerriDevice * & ferri)
{
	JCASSERT(NULL == ferri);
	JCASSERT(dev);

	//if (m_ctrl_name == _T("LT2244") )
	//{
	//	CFerriLT2244 * _ferri = new CFerriLT2244(dev);
	//	_ferri->LoadDll(dll_path);
	//	ferri = static_cast<IFerriDevice*>(_ferri);
	//}
	//else if ( m_ctrl_name == _T("SM2236") )
	//{
	//	CFerriSM2236 * _ferri = new CFerriSM2236(dev);
	//	_ferri->LoadDll(dll_path);
	//	ferri = static_cast<IFerriDevice*>(_ferri);
	//}
	return true;
}

bool CFerriFactory::ScanDevice(IFerriDevice * & ferri)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == ferri);

	TCHAR drive_letter;
	TCHAR drive_name[MAX_PATH];

	bool tester_connect = false;
	bool hub_connect = false;
	bool success = false;

	stdext::auto_array<UCHAR> inquiry_buf(SECTOR_SIZE);
	HANDLE	h_dev = NULL;
	SMI_TESTER_TYPE tester_type = NON_SMI_TESTER;
	UINT port_num = 0;


	for(drive_letter = _T('C'); drive_letter <= _T('Z'); drive_letter++)
	{
		_stprintf_s(drive_name, _T("\\\\.\\%c:"), drive_letter);
		memset(inquiry_buf, 0, SECTOR_SIZE);

		h_dev = CreateFile(drive_name,
			GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 
			NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL );
		if( h_dev == INVALID_HANDLE_VALUE )	continue;

		if( Send_Inquiry_XP_SM333(h_dev, inquiry_buf, SECTOR_SIZE) )
		{
			tester_type = NON_SMI_TESTER;
			Send_Initial_Card_Command(h_dev, true);
			if(Send_Inquiry_Tester(h_dev, inquiry_buf, SECTOR_SIZE, tester_type)) 
			{
				LOG_DEBUG(_T("Drive %c is Tester"), drive_letter);
				port_num = inquiry_buf[0X1D] ;
				if((port_num==0)||(port_num==1))	port_num = 1;
				LOG_DEBUG(_T("Port %d is up"), port_num);
				tester_connect = true;
				if (inquiry_buf[0x0C])	hub_connect = true;
				break;
			}
		}
		CloseHandle(h_dev);
		h_dev = NULL;
	}
	if ( (NULL != h_dev) && (INVALID_HANDLE_VALUE!=h_dev) )	CloseHandle(h_dev);
	h_dev = INVALID_HANDLE_VALUE;

	LOG_DEBUG(_T("found device %s"), drive_name);

	if( !tester_connect )	THROW_ERROR_EX(ERR_DEVICE, FERR_NO_TESTER, _T("no tester"));
	if( !hub_connect )		THROW_ERROR_EX(ERR_DEVICE, FERR_NO_CARD, _T("no tester"));

	success = TestOpen((char)drive_letter, &h_dev);
	if(!success)			THROW_ERROR_EX(ERR_DEVICE, FERR_OPEN_DRIVE_FAIL, _T("card fail"));

	// create device
	if ( m_ctrl_name == _T("LT2244") )
	{
		CFerriLT2244 * _ferri = new CFerriLT2244(&m_apis, drive_letter, drive_name, tester_type, port_num, h_dev);
		ferri = static_cast<IFerriDevice*>(_ferri);
	}
	else if (m_ctrl_name == _T("SM2236") )
	{
		CFerriSM2236 * _ferri = new CFerriSM2236(&m_apis, drive_letter, drive_name, tester_type, port_num, h_dev);
		ferri = static_cast<IFerriDevice*>(_ferri);
	}
	Sleep(200);

	if ( !ferri->CheckIsFerriChip() )
	{
		ferri->Release();
		ferri = NULL;
		THROW_ERROR_EX(ERR_DEVICE, FERR_CONTROLLER_NOT_MATCH, _T("wrong controller"));
	}
	return true;
}

bool CFerriFactory::LoadDll(LPCTSTR path)
{
	m_dll = LoadLibrary(path);
	if (m_dll == NULL)	THROW_WIN32_ERROR(_T(" failure on loading dll %s "), path );

	// load entry
	m_apis.mf_check_if_smi_chip = (SMI_API_TYPE_0)GetProcAddress(m_dll, "CheckIf_SMIChip");
	if (!m_apis.mf_check_if_smi_chip) THROW_WIN32_ERROR(_T("function %S is not included in %s."), "CheckIf_SMIChip", path );

	m_apis.mf_get_id_isp_ver = (SMI_API_TYPE_1)GetProcAddress(m_dll, "Get_IC_ISPVer");
	if (!m_apis.mf_get_id_isp_ver) THROW_WIN32_ERROR(_T("function %S is not included in %s."), "Get_IC_ISPVer", path );

	m_apis.mf_read_flash_id = (SMI_API_TYPE_1)GetProcAddress(m_dll, "Read_FlashID");
	if (!m_apis.mf_read_flash_id) THROW_WIN32_ERROR(_T("function %S is not included in %s."), "Read_FlashID", path );

	m_apis.mf_update_isp = (lpUpdate_ISP)GetProcAddress(m_dll, "Update_ISP");
	if (!m_apis.mf_update_isp) THROW_WIN32_ERROR(_T("function %S is not included in %s."), "Update_ISP", path );

	m_apis.mf_read_isp = (lpRead_ISP)GetProcAddress(m_dll, "Read_ISP");
	if (!m_apis.mf_read_isp) THROW_WIN32_ERROR(_T("function %S is not included in %s."), "Read_ISP", path );

	m_apis.mf_get_initial_bad_block_count = (SMI_API_TYPE_0)GetProcAddress(m_dll, "Get_InitialBadBlockCount");
	if (!m_apis.mf_get_initial_bad_block_count) THROW_WIN32_ERROR(_T("function %S is not included in %s."), "Get_InitialBadBlockCount", path );

	m_apis.mf_get_identify = (SMI_API_TYPE_1)GetProcAddress(m_dll, "Get_IDENTIFY");
	if (!m_apis.mf_get_identify) THROW_WIN32_ERROR(_T("function %S is not included in %s."), "Get_IDENTIFY", path );

	m_apis.mf_get_smart = (SMI_API_TYPE_1)GetProcAddress(m_dll, "Get_SMART");
	if (!m_apis.mf_get_smart) THROW_WIN32_ERROR(_T("function %S is not included in %s."), "Get_SMART", path );

	return true;
}


///////////////////////////////////////////////////////////////////////////////
//---- CFerriComm

#define RETRY_COUNT		(40)

CFerriComm::CFerriComm(
				SMI_API_SET * apis, TCHAR drive_letter, LPCTSTR drive_name, 
				SMI_TESTER_TYPE tester, UINT port, HANDLE dev)
	: m_ref(1), m_dev(dev), m_drive_letter(drive_letter), m_drive_name(drive_name)
	, m_mpisp(NULL), m_apis(apis), m_tester(tester), m_tester_port(port)
{
	JCASSERT(m_dev);
	JCASSERT(m_apis);
}

CFerriComm::~CFerriComm(void)
{
	if (m_dev)	TestClose(m_dev);
	delete [] m_mpisp;
}

bool CFerriComm::ResetTester(void)
{
	LOG_STACK_TRACE();
	CheckDevice(m_dev);

	bool br = false;
	SMITesterPowerOff(m_dev, m_tester);
	Sleep(2000);
	br = SMITesterPowerOn(m_dev);
	return br;
}

bool CFerriComm::ReadIsp(JCSIZE len, IIspBuffer * & isp1, IIspBuffer * & isp2)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == isp1);
	JCASSERT(NULL == isp2);
	CheckDevice(m_dev);
	JCASSERT(m_apis);

	CreateIspBuf(len, isp1);
	CreateIspBuf(len, isp2);
	
	int ir = m_apis->mf_read_isp(FALSE, NULL, NULL, m_dev, len, 
		(UCHAR*)(isp1->GetBuffer()), (UCHAR*)(isp2->GetBuffer()));
	isp1->ReleaseBuffer();
	isp2->ReleaseBuffer();
	return (ir != 0);
}

bool CFerriComm::CheckIsFerriChip(void)
{
	LOG_STACK_TRACE();
	JCASSERT(m_apis);
	CheckDevice(m_dev);
	int ir = 0, retry = 1;
	while ( (retry > 0) )
	{
		ir = m_apis->mf_check_if_smi_chip(FALSE, NULL, NULL, m_dev);
		LOG_DEBUG(_T("check smi chip = %d, retry = %d"), ir, retry);
		if (ir > 0) break;
		Sleep(1000);
		retry --;
	}
	return  (ir > 0);
}

void CFerriComm::CheckDevice(HANDLE dev)
{
	LOG_STACK_TRACE_EX(_T("dev=0x%08X"), dev);
	if ( (NULL == dev) || (INVALID_HANDLE_VALUE == dev) )	
		THROW_ERROR_EX(ERR_DEVICE, FERR_DEVICE_NOT_CONNECT,_T("no device connect"));
}

bool CFerriComm::Connect(void)
{
	LOG_STACK_TRACE();
	JCASSERT(m_dev == NULL);

	// retry
	int retry = RETRY_COUNT;
	bool br = false;

	HANDLE hdev = INVALID_HANDLE_VALUE;
	UCHAR Command[16];
	memset(Command,0,16);

	CHAR DiskName[64];
	sprintf_s( DiskName, "\\\\.\\%c:", m_drive_letter );

	while (retry > 0)
	{
		if (hdev != INVALID_HANDLE_VALUE) CloseHandle(hdev);

		LOG_DEBUG(_T("retry: %d"), retry);
		retry --;
		Sleep(1500);

		hdev = CreateFileA(DiskName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 
							NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
		if (hdev == INVALID_HANDLE_VALUE) continue;
		LOG_DEBUG(_T("open device: 0x%08X"), hdev);

		br = SCSICommandReadTester(hdev, NULL, 0, Command);
		LOG_DEBUG(_T("test utility: %d"), br);
		if (!br) continue;
		// connected OK
		m_dev = hdev;
		break;
	}
	if (!m_dev)
	{
		LOG_ERROR(_T("failure on re-connect to device"));
		return false;
	}

	br = CheckIsFerriChip();
	if (!br)
	{
		LOG_ERROR(_T("failure on check controller."));
		Disconnect();
		return false;
	}
	return true;
}

bool CFerriComm::ReadSmart(LPVOID buf, JCSIZE buf_len)
{
	LOG_STACK_TRACE();
	CheckDevice(m_dev);
	if (buf_len < SECTOR_SIZE) THROW_ERROR(ERR_APP, _T("buffer is not enough."));
	int ir = m_apis->mf_get_smart(FALSE, NULL, NULL, m_dev, (UCHAR*)buf);
	LOG_DEBUG(_T("result = %d"),ir);
	return (ir!=0);
}


///////////////////////////////////////////////////////////////////////////////
//---- CIspBufferComm
CIspBufferComm::CIspBufferComm(JCSIZE reserved)
: m_ref(1), m_buf(NULL), m_buf_size(reserved), m_data_len(0)
{
	m_buf = new BYTE[m_buf_size];
}

CIspBufferComm::~CIspBufferComm(void)
{
	delete [] m_buf;
}

bool CIspBufferComm::LoadFromFile(LPCTSTR fn)
{
	FILE* stream = NULL;
	LOG_DEBUG(_T("open isp file : %s"), fn);
	_tfopen_s(&stream, fn, _T("rb"));
	if ( stream == NULL)	return false;	
	m_data_len = fread(m_buf, sizeof(BYTE), m_buf_size, stream );		
	fclose( stream );
	return true;
}

bool CIspBufferComm::SaveToFile(LPCTSTR fn)
{
	FILE * stream = NULL;
	_tfopen_s(&stream, fn, _T("wb+"));
	if (NULL == stream) return false;
	fwrite(m_buf, sizeof(BYTE), m_data_len, stream );
	fclose( stream );
	return true;
}

DWORD CIspBufferComm::CheckSum(void)
{
	DWORD check_sum=0;
	for(DWORD ii=0; ii< m_data_len; ii++)	check_sum += m_buf[ii];
	return check_sum;
}

bool CIspBufferComm::Compare(IIspBuffer * isp) const
{
	JCSIZE tag_len = isp->GetSize();
	if (tag_len != m_data_len) return false;

	LPVOID tag_buf = isp->GetBuffer();
	bool br = (memcmp(tag_buf, m_buf, m_data_len) == 0);
	isp->ReleaseBuffer();
	return br;
}


void CIspBufferComm::FillConvertString(LPCTSTR src, BYTE * dst, DWORD len, char filler)
{
	memset(dst, filler, len);
	DWORD src_len = _tcslen(src);
	LPCTSTR str_src = src;
	// copy src to dst ( unicode -> UTF8)
	for (DWORD ii = 0; ii < src_len; ++ii)	dst[ii] = (BYTE)(str_src[ii]);
	// convert byte order
	BYTE t;
	for (DWORD ii = 0; ii < len; ii += 2)		t=dst[ii], dst[ii]=dst[ii+1], dst[ii+1]=t ;
}