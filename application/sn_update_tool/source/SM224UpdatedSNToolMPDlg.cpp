// SM224UpdatedSNToolMPDlg.cpp : implementation file
#include "stdafx.h"
#include <stdext.h>

LOCAL_LOGGER_ENABLE(_T("upsn"), LOGGER_LEVEL_NOTICE);

#include "sm224testB.h"

#include "smidisk.h"

#include "SM224UpdatedSNToolMPDlg.h"
#include "UPSNTool_MessageBox.h"
#include "UpdateSNTool_Caution.h"
#include <cfgmgr32.h>
#include <process.h>
#include <Windows.h>
#include <Winuser.h>
#include <WinSock.h>
#include "ColorListCtrl.h"
#include <Setupapi.h>
#include <cfgmgr32.h>
#include <winioctl.h>

#pragma warning(disable:4089)

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DUMMY_TEST

const BOOL PHYSICAL_DRIVE = FALSE;

#define		nColorListCtrlCols	10

#define		ncol_item		0	//=== Item						===//
#define		ncol_progress	1	//=== Progress					===//
#define		ncol_status		2	//=== Status					===//
#define		ncol_capa		3	//=== Capacity					===//
#define		ncol_sn			4	//=== Serial Number				===//
#define		ncol_vpid		5	//=== VID/PID					===//
#define		ncol_flash		6	//=== Flash ID					===//
#define		ncol_bb			7	//=== Bad Block					===//
#define		ncol_inq		8	//=== Inquiry String			===//
#define		ncol_wp			9	//=== Write Protect				===//
#define		MYBUFSIZE 10*1024*1024

//Updated SN tool parameter
#define List_STATUS  1
#define List_ModelName  2
#define List_CurrentSN  3

//========================================================================================================================//
void PassAllMessage()
{
	///////////////////////////////////////////
	MSG msg;
	while( PeekMessage(&msg,NULL, 0, 0, PM_REMOVE) )
	{
		TranslateMessage( &msg );      /* Translates virtual key codes.  */
		DispatchMessage( &msg );       /* Dispatches message to window.  */
	}
	//////////////Simon///////////////////////
}

/////////////////////////////////////////////////////////////////////////////
// CSM224UpdatedSNToolMPDlg dialog
CSM224UpdatedSNToolMPDlg::CSM224UpdatedSNToolMPDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSM224UpdatedSNToolMPDlg::IDD, pParent)
	, m_str_test_model_name(_T(""))
	, m_capacity(0)
	, m_str_count_current(_T(""))
	, m_str_count_limit(_T(""))
	, m_is_set_sn(false)
	, m_btn_start(NULL)
	, m_btn_scan_drive(NULL)
	, m_btn_quit(NULL)
	, m_btn_set_sn(NULL)
	, m_edit_input_sn(NULL)
	, m_scan_timer(0)
	, m_str_fw_rev(_T(""))
{
	memset(m_vendor_specific, 0, VENDOR_LENGTH + 1);
	memset(m_flash_id, 0, SECTOR_SIZE);
}


void CSM224UpdatedSNToolMPDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_SNUpdatedTool, m_ListUpdatedSNTool);
	DDX_Text(pDX, IDC_TESTMODULE, m_str_test_model_name);
	DDX_Text(pDX, IDC_STATIC_CurrentCnt, m_str_count_current);
	DDX_Text(pDX, IDC_STATIC_LimitCnt, m_str_count_limit);
	DDX_Text(pDX, IDC_FWREV, m_str_fw_rev);
}


BEGIN_MESSAGE_MAP(CSM224UpdatedSNToolMPDlg, CDialog)
	ON_BN_CLICKED(ID_Start, OnStartUpdatedSN)
	ON_BN_CLICKED(IDC_BUTTON_ScanDrive, OnBUTTONScanDrive)
	ON_BN_CLICKED(ID_Quit, OnQuit)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_EN_CHANGE(IDC_EDIT_InputSN, OnChangeEDITInputSN)
	ON_BN_CLICKED(IDC_BUTTON_SetSN, OnBUTTONSetSN)
	ON_BN_CLICKED(IDC_BUTTON_ClearCnt, OnBUTTONClearCnt)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSM224UpdatedSNToolMPDlg message handlers

BOOL CSM224UpdatedSNToolMPDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	try
	{
		m_scan_timer = 0;

		if ( !LoadConfig() ) 
		{
			PostMessage(WM_CLOSE, 0, 0);
			return FALSE;
		}
		RetrieveController();

		CString str_title;
		CSM224testBApp * app = dynamic_cast<CSM224testBApp *>(AfxGetApp());
		ASSERT(app);

		str_title.Format(_T("SMI. Update Serial Number Tool  Ver. %s"), app->GetVer() );
		this->SetWindowText(str_title);

		ListView_SetExtendedListViewStyle(m_ListUpdatedSNTool, LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);
		// TODO: Add extra initialization here
		m_ListUpdatedSNTool.InsertColumn(0, _T("Item"), LVCFMT_CENTER, 60, -1);
		m_ListUpdatedSNTool.InsertColumn(1, _T("Status"), LVCFMT_CENTER, 120, -1);
		m_ListUpdatedSNTool.InsertColumn(2, _T("Model Name"), LVCFMT_CENTER, 220, -1);
		m_ListUpdatedSNTool.InsertColumn(3, _T("Current S/N"), LVCFMT_CENTER, 180, -1);
		m_ListUpdatedSNTool.SetTextColor(RGB(0,0,255));

		m_ListUpdatedSNTool.DeleteAllItems();

		m_edit_input_sn->LimitText(SN_LENGTH_OEM);


		m_count_current = GetPrivateProfileInt(SEC_PARAMETER, KEY_COUNT_CURRENT, 0, 
			app->GetRunFolder() + FILE_COUNT_CURRENT);
		m_str_count_current.Format(_T("%d"), m_count_current);
		
		m_count_limit=GetPrivateProfileInt(SEC_PARAMETER, KEY_COUNT_LIMIT, 0, 
			app->GetRunFolder() + FILE_COUNT_LIMIT );
		m_str_count_limit.Format(_T("%d"), m_count_limit);

		ASSERT(m_btn_start);
		m_btn_start->EnableWindow(FALSE);

		m_ListUpdatedSNTool.InsertItem(0, _T("") );

		m_str_test_model_name.Format(_T("[%s]"), m_model_name);
		m_str_fw_rev.Format(_T("[%s]"), m_fw_ver);
		UpdateData(FALSE);
		m_edit_input_sn->SetFocus();
	}
	catch (stdext::CJCException & err)
	{
		MessageBox(err.WhatT(), _T("Error!"), MB_OK);
		PostMessage(WM_CLOSE, 0, 0);
		return FALSE;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSM224UpdatedSNToolMPDlg::SetStatus(COLORREF cr, LPCTSTR status)
{
	m_ListUpdatedSNTool.SetTextColor(cr);
	m_ListUpdatedSNTool.SetItemText(0,List_STATUS, status);
	PassAllMessage();
}


void CSM224UpdatedSNToolMPDlg::OnStartUpdatedSN() 
{
	// TODO: Add your control notification handler code here
	CString Sn_H5;
	BOOL isSN_equal_A=FALSE,isSN_equal_B=FALSE,isVendorModelName=FALSE,isNotSN16Char=FALSE;
	BOOL isModelFlashID=FALSE; //check if correct flash
	BOOL isCorrectConfigTxt=FALSE; //check if correct config.txt
	BOOL isMatchSelectedModelSN=0;
	int SNStr_cnt=0;

	m_btn_scan_drive->EnableWindow(TRUE);
	m_btn_start->EnableWindow(FALSE);	
#ifdef NECI
	m_btn_quit->EnableWindow(TRUE); //L0130 Lance modify for NECi request
	m_edit_input_sn->EnableWindow(TRUE);//L0130 Lance modify for NECi request
#else
	m_btn_quit->EnableWindow(FALSE);
	m_edit_input_sn->EnableWindow(FALSE);
#endif

	SetStatus(COLOR_BLUE, _T("Scanning...") );

	bool dummy_test = false;
#ifdef _DEBUG
	if ( m_input_sn == _T("dummy") )		dummy_test = true;
#endif

	//scan device
	HANDLE device = NULL;
	UPSNTool_MessageBox UPSN_MessageBox;

	CTime	start_time;

	DEVICE_INFO	info;
	info.m_size = sizeof(DEVICE_INFO);
	info.m_drive_letter = 0;
	info.m_capacity = 0;
	info.m_init_bad = 0;
	info.m_new_bad = 0;
	info.m_serial_number = m_input_sn;
	info.m_model_name = m_oem_model_name;
	info.m_capacity = m_capacity;
	info.m_fw_version = m_fw_ver;
	info.m_error_code = UPSN_UNKNOW_FAIL;
	info.m_pass_fail = false;

	try
	{
		LOG_NOTICE(_T("start update sn, count %d / %d"), m_count_current, m_count_limit);
		char drive_letter = 0;
		CString drive_name;
		bool br = false;
		LOG_DEBUG(_T("Test Mode: %d"), m_test_mode); 
		if (!dummy_test)
		{
			br = ScanDrive(drive_letter, drive_name, device);
			if( !br || NULL == device || INVALID_HANDLE_VALUE == device)//"no scan drive" or "scan drive fail"
			{
				MessageBox(_T("Please Check Device and Scan Drive again"), _T(""), MB_OK);
				throw new CUpsnWarning;
			}
			info.m_drive_letter = drive_letter;
			info.m_drive_name = drive_name;
		}
		else
		{
			LOG_WARNING(_T("dummy sacn..."));
			info.m_drive_letter = _T('Z');
			info.m_drive_name = _T("\\.\Z:");
			if (m_test_mode == MODE_UPDATE_VERIFY)	info.m_error_code = UPSN_PASS;
			else if (m_test_mode == MODE_VERIFY_ONLY)	info.m_error_code = UPSN_VERIFYSN_PASS;
			
		}
		SetStatus(COLOR_BLUE, _T("Loading...") );

		start_time = CTime::GetCurrentTime();
		Sleep(500);

		if (!dummy_test)
		{
			info.m_init_bad = Get_InitialBadBlockCount(FALSE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, device);
			BYTE IDTable[SECTOR_SIZE];
			memset(IDTable,0,sizeof(IDTable));
			int ir = Get_IDENTIFY(FALSE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, device, IDTable);

			if (!ir)
			{
				MessageBox(_T("Failure on reading id table"), _T(""), MB_OK);
				throw new CUpsnWarning;
			}

			// get current serial number and show
			CString current_sn;
			ReadConvertString(IDTable + SN_OFFSET_ID, current_sn, SN_LENGTH);
			m_ListUpdatedSNTool.SetItemText(0, List_CurrentSN, current_sn);
			//info.m_serial_number = current_sn;

			// get model name and show
			CString current_model_name;
			ReadConvertString(IDTable + MODEL_OFFSET_ID, current_model_name, MODEL_LENGTH);
			m_ListUpdatedSNTool.SetItemText(0, List_ModelName, current_model_name);
			//info.m_model_name = current_model_name;

			// check sn
			LOG_DEBUG(_T("input sn = %s"), m_input_sn);
			bool verify_sn = true;
			if ( ( (MODE_VERIFY_ONLY == m_test_mode) && (m_input_sn == m_global_prefix) ) )	
			{
				LOG_DEBUG(_T("skip verify sn"))
				verify_sn = false;
				info.m_serial_number = current_sn;
				info.m_model_name = current_model_name;
			}
			else
			{
				// check sn
				verify_sn = true;
				if ( (m_input_sn.Left(5) != m_sn_prefix) || ( !CheckFlashID(device) ) )
				{
					LOG_ERROR(_T("sn do not match"))
					throw new CUpsnCaution(CUpsnCaution::CAUTION_SN_CAPACITY);
				}

				if (m_input_sn.GetLength() < SN_LENGTH_OEM)
				{
					LOG_ERROR(_T("wrong sn length. length = %d"), m_input_sn.GetLength())
					throw new CUpsnCaution(CUpsnCaution::CAUTION_SN_LENGTH);
				}
			}
			// header of inputted serial number (prefix)
			ir = UpdatedSNtoDevice(device, verify_sn);
			info.m_error_code = ir;
			// close device before run external cmd
			TestClose(device); 
			device = NULL;
		}

		SetStatus(COLOR_BLUE, _T("External test..."));
		DWORD exit_code = ExcuseExternalProcess(info);
		
		if (!dummy_test)
		{
			SetStatus(COLOR_BLUE, _T("Getting runtime bad..."));
			TestOpen(drive_letter, &device);
			if ( NULL == device || INVALID_HANDLE_VALUE == device)
			{
				MessageBox(_T("Failure on openning device"), _T("Error!"), MB_OK);
				throw new CUpsnWarning();
			}
			info.m_new_bad = GetRunTimeBad(device);
			TestClose(device);
			device = NULL;
		}
		if ( 0 != exit_code ) throw new CUpsnError(exit_code);
		info.m_pass_fail = true;

		SetStatus(COLOR_BLUE, _T("Complete"));
		//info.m_error_code = UPSN_PASS;
		UPSN_MessageBox.DoModal( &info );

		// log1
		br = m_log_file.WriteLog1(info, start_time);
		br = m_log_file.WriteLog2(info, start_time);
	}
	catch (stdext::CJCException &err)
	{
		MessageBox(err.WhatT(), _T("Error!"), MB_OK);
	}
	catch (CUpsnError * pe)
	{	// error
		SetStatus(COLOR_RED, _T("Fail"));
		info.m_error_code = pe->GetErrorCode();
		UPSN_MessageBox.DoModal( &info );
		info.m_error_code = pe->GetErrorCode();
		bool br = m_log_file.WriteLog1(info, start_time);
		pe->Delete();
	}
	catch (CUpsnCaution * pe)
	{
		CUpdateSNTool_Caution caution_dlg;
		int ir = caution_dlg.DoModal(pe->GetErrorCode() );
		if(CUpdateSNTool_Caution::CAUTION_RETRY == ir) 
		{
			ClearInputSn();
		}
		else if(CUpdateSNTool_Caution::CAUTION_CANCEL == ir)
		{
			m_edit_input_sn->SetSel(0,-1);
			m_edit_input_sn->Clear();
		}
		SetStatus(COLOR_BLUE, _T("Standy By!"));
		pe->Delete();
	}
	catch (CUpsnWarning * pe)
	{
		pe->Delete();
	}

	m_edit_input_sn->EnableWindow(TRUE);
	m_btn_quit->EnableWindow(TRUE);

	UpdateCnt_UPSNTool(m_count_current + 1);
	ReShowMainPage_UPSN();
	OnChangeEDITInputSN();
	ClearInputSn();
	Invalidate(TRUE);
	PassAllMessage();

	if (NULL != device || INVALID_HANDLE_VALUE != device)	TestClose(device); 
	return;
}

DWORD CSM224UpdatedSNToolMPDlg::ExcuseExternalProcess(const DEVICE_INFO & info)
{
	LOG_STACK_TRACE();

	if ( m_external_cmd.IsEmpty() ) return 0;
	// process placeholder
	stdext::auto_array<TCHAR> cmd_line(MAX_STRING_LEN * 2);

	LPTSTR str = cmd_line;
	JCSIZE remain = MAX_STRING_LEN * 2;
	memset(cmd_line, 0, remain * sizeof(TCHAR) ); 

	LPCTSTR str_src = m_external_param;

	int ptr_src = 0;
	int len = m_external_param.GetLength();
	while (1)
	{
		int next_holder = m_external_param.Find(_T('%'), ptr_src);
		if ( (next_holder + 1) >= len) THROW_ERROR(ERR_PARAMETER, _T("format error in external process string."));

		if ( next_holder < 0 )
		{	// copy remain
			int src_len = (len - ptr_src);
			wmemcpy_s(str, remain, str_src + ptr_src, src_len );
			memset(str + src_len, 0, (remain - src_len) * sizeof (TCHAR));
			break;
		}
		// copy pre-string
		int src_len = (next_holder - ptr_src);
		wmemcpy_s(str, remain, str_src + ptr_src, src_len );
		str += src_len;
		remain -= src_len;
		ptr_src = next_holder + 2;

		TCHAR holder = str_src[next_holder +1];
		int attached_len = 0;
		switch (holder)
		{
		case _T('D'):	// drive name
			_tcscpy_s(str, remain, info.m_drive_name);
			attached_len = info.m_drive_name.GetLength();
			break;
		case _T('N'):
			_tcscpy_s(str, remain, info.m_serial_number);
			attached_len = info.m_serial_number.GetLength();
			break;

		case _T('L'):	// drive letter
			str[0] = info.m_drive_letter;
			attached_len = 1;
			break;

		case _T('T'):	// test mode
			if (MODE_UPDATE_VERIFY == m_test_mode)				_tcscpy_s(str, remain, _T("UPDATE"));
			else if (MODE_VERIFY_ONLY == m_test_mode)			_tcscpy_s(str, remain, _T("VERIFY"));
			attached_len = 6;
			break;

		case _T('F'):	// f/w version
			_tcscpy_s(str, remain, info.m_fw_version);
			attached_len = info.m_fw_version.GetLength();
			break;
		case _T('M'):	// model name
			_tcscpy_s(str, remain, info.m_model_name);
			attached_len = info.m_model_name.GetLength();
			break;


		//case _T(''):
		default:
			THROW_ERROR(ERR_PARAMETER, _T("unknow placeholder %%%c"), holder);
		}
		str += attached_len, remain -= attached_len;
	}
	LOG_NOTICE(_T("external test: <%s\\%s %s>"), m_external_path, m_external_cmd, cmd_line);


	SHELLEXECUTEINFO shell_info = {0};
	shell_info.cbSize = sizeof(SHELLEXECUTEINFO);
	shell_info.fMask = SEE_MASK_NOCLOSEPROCESS;
	//shell_info.hwnd = m_hWnd;
	shell_info.hwnd = NULL;
	shell_info.lpVerb = NULL;
	shell_info.lpFile = m_external_cmd;	
	shell_info.lpParameters = cmd_line;	
	shell_info.lpDirectory = m_external_path;
	shell_info.nShow = (m_external_show_wnd) ? SW_SHOW : SW_HIDE;
	shell_info.hInstApp = NULL;

	// exec process
	BOOL br = ShellExecuteEx(&shell_info);
	if (!br) THROW_WIN32_ERROR(_T("failure on running command <%s\\%s %s>"), m_external_path, m_external_cmd, cmd_line);

	UINT timeout = (0==m_external_timeout)? INFINITE:(UINT)m_external_timeout * 1000;
	DWORD ir =0;
#ifdef NECI
	ir = WaitForSingleObject(shell_info.hProcess, timeout);
#else
	while (1) 
	{
		ir = MsgWaitForMultipleObjects(1, &shell_info.hProcess, FALSE, timeout, QS_ALLINPUT);
		if (WAIT_OBJECT_0 == ir || WAIT_TIMEOUT == ir) break;
		else if ( (WAIT_OBJECT_0 + 1) == ir)
		{
			PassAllMessage();
		}
		else THROW_WIN32_ERROR(_T("failure on waiting external test"));	
	}
#endif

	if (WAIT_TIMEOUT == ir)
	{
		TerminateProcess(shell_info.hProcess, UPSN_EXTER_TIMEOUT);
		LOG_ERROR(_T("external test timeout. timer = %dms"), timeout);
		throw new CUpsnError(UPSN_EXTER_TIMEOUT);
	}
	//WaitForSingleObject(shell_info.hProcess, INFINITE);
	
	// check return code
	DWORD exit_code = 0;
	GetExitCodeProcess(shell_info.hProcess, &exit_code);
	CloseHandle(shell_info.hProcess);
	LOG_NOTICE(_T("running command finished, exit code = %d"), exit_code);

	return exit_code;
}

DWORD CSM224UpdatedSNToolMPDlg::GetRunTimeBad(HANDLE device)
{
	LOG_STACK_TRACE();
	stdext::auto_array<BYTE> buf(SECTOR_SIZE);
	bool ir = Get_SMART(FALSE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, device, buf);
	if (!ir)
	{
		MessageBox(_T("Failure on reading SMART."), _T("Error!"), MB_OK);
		throw new CUpsnWarning();
	}

	DWORD new_bad = MAKELONG(MAKEWORD(buf[0x1F], buf[0x20]), MAKEWORD(buf[0x20], buf[0x21]));
	LOG_DEBUG(_T("new_bad = %d"), new_bad);
	return new_bad;
}

void CSM224UpdatedSNToolMPDlg::OnBUTTONScanDrive()
{
	m_btn_scan_drive->SetWindowText( _T("Scanning..") );
	m_btn_start->EnableWindow(FALSE);	
	m_btn_quit->EnableWindow(FALSE);
	m_btn_scan_drive->EnableWindow(FALSE);
}


bool CSM224UpdatedSNToolMPDlg::ScanDrive(char & drive_letter, CString & drive_name, HANDLE & device)
{
	int port_number = 0;
	bool tester_connect = false;
	bool hub_connect = false;
	bool success = false;

#ifndef _DEBUG
	m_scan_timer = SetTimer(0x543,30*1000,0);//Scan Drive timer,30 seconds limit
#endif
    m_edit_input_sn->EnableWindow(TRUE);

    BOOL    Find=FALSE;
    UCHAR   InquiryData[SECTOR_SIZE];
	HANDLE	hDevice = NULL;

	for(int ii=0; ii<24; ii++)
	{
		drive_letter='C'+ii;
        memset(InquiryData,0,SECTOR_SIZE);
		drive_name.Format(_T("\\\\.\\%c:"), drive_letter);
		hDevice = CreateFile(drive_name,
			GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 
			NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL );

		if( hDevice == INVALID_HANDLE_VALUE )	continue;

		if( Send_Inquiry_XP_SM333(hDevice, InquiryData, SECTOR_SIZE) )
		{
			m_tester = NON_SMI_TESTER;
			Send_Initial_Card_Command(hDevice, true);
			if(Send_Inquiry_Tester(hDevice, InquiryData, SECTOR_SIZE, m_tester)) 
			{
				LOG_DEBUG(_T("Drive %c is Tester"), drive_letter);
				port_number = InquiryData[0X1D] ;
				if(port_number == 0)	LOG_DEBUG(_T("Port %d is up"), port_number);
				if((port_number==0)||(port_number==1))	port_number = 1;
				tester_connect = true;
				if (InquiryData[0x0C])
				{
					Find = TRUE;
					hub_connect = true;
				}
				break;
			}
		}
		CloseHandle(hDevice);
		hDevice = NULL;
	}
	if (NULL != hDevice && INVALID_HANDLE_VALUE)	CloseHandle(hDevice);
	hDevice = NULL;
	CString Cstr_1; 

	LOG_DEBUG(_T("found device %s"), drive_name);
	SetStatus(COLOR_BLUE, _T("Scanning.."));
	//port number
	Cstr_1.Format( _T("Port%d"), port_number);
	m_ListUpdatedSNTool.SetItemText(0,0,Cstr_1);
	PassAllMessage();

/////////////////////////////////////////////
	device = INVALID_HANDLE_VALUE;
	int api_ret=0;
	//for record current Input S/N,read input serial number into g_InputSN
	try
	{
		if( !tester_connect )
		{
			SetStatus(COLOR_BLUE, _T("") );
			throw new CUpsnError(UPSN_SCANDRIVE_FAIL);
 		}
	
		if( !hub_connect )
		{
			SetStatus(COLOR_RED, _T("No Card !") );
			throw new CUpsnError(UPSN_SCANDRIVE_FAIL);
		}

		// hub_connect must be true
		success = TestOpen(drive_letter, &device);
		if(!success)
		{
			SetStatus(COLOR_RED, _T("Card Fail!") );
			throw new CUpsnError(UPSN_OPENDRIVE_FAIL);
		}

		// successeed
		Sleep(200);

		int ic_ver = CheckIf_SMIChip(FALSE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, device);

		if(ic_ver == Unknown_CHIP)
		{
			SetStatus(COLOR_RED, _T("No Card !") );
			throw new CUpsnError(UPSN_OPENDRIVE_FAIL);
		}

		if(ic_ver != m_controller_ver) 
		{
			MessageBox(_T("Please check IC version"), _T(""), MB_OK);
			throw new CUpsnWarning();
		}

		success = true;
	}
	catch (...)
	{
		KillTimer(0x543);//kill scan drive timer ??
		if (NULL != device &&  INVALID_HANDLE_VALUE != device)
		{
			TestClose(device); 
			device = NULL;
		}
		throw;
	}

	KillTimer(0x543);//kill scan drive timer
	return success;
}

int CSM224UpdatedSNToolMPDlg::UpdatedSNtoDevice(HANDLE device, bool verify_sn)
{
	LOG_STACK_TRACE();
	ASSERT(NULL != device && INVALID_HANDLE_VALUE != device);
	bool success=false;

	stdext::auto_array<BYTE>	isp_buf(ISP_LENGTH);
	memset(isp_buf, 0, ISP_LENGTH);
	DWORD isp_len = 0;

	int ir = 0;

	isp_len = UpsnLoadIspFile(device, isp_buf, ISP_LENGTH);
	if(MODE_UPDATE_VERIFY == m_test_mode)
	{	//test mode(update sn and verify)
		UPSN_UpdateISPfromConfig(isp_buf, isp_len);
		success = UPSN_UpdateISP2244LT(device, isp_buf, isp_len);
		// verify before reset
		UPSN_ReadISP2244LT(device, isp_buf, isp_len);
		//updated success and reset device!!										
		success = Reset_UPSNTool(device);
		if(!success)	throw new CUpsnError(UPSN_Reset_FAIL);

		//read isp back to check again
		success = UPSN_ReadISP2244LT(device, isp_buf, isp_len);
		ir = VerifySN_UPSNTool(device, verify_sn);
		if( ir != UPSN_VERIFYSN_PASS )	throw new CUpsnError(ir);
		ir = UPSN_PASS;
	}
	else	//only verify
	{
		ir = VerifySN_UPSNTool(device, verify_sn);
		if( ir != UPSN_VERIFYSN_PASS)	throw new CUpsnError(ir);
	}		
	return ir;
}

void CSM224UpdatedSNToolMPDlg::OnQuit() 
{
	// TODO: Add your control notification handler code here
	//K1024 Lance add for save current count to file
	UpdateData();
	WritePrivateProfileString(SEC_PARAMETER, KEY_COUNT_CURRENT, m_str_count_current, FILE_COUNT_CURRENT);
	CDialog::OnCancel();
}

void CSM224UpdatedSNToolMPDlg::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	if(nIDEvent==0x543)//Re ScanDrive until 30 seconds time out
	{
		DEVICE_INFO info;
		info.m_error_code = UPSN_SCANDRIVE_TimeOut_FAIL;
		UPSNTool_MessageBox UPSN_MessageBox;
		KillTimer(0x543);//kill scan drive timer
		//show error message box
		UPSN_MessageBox.DoModal(&info);
		OnChangeEDITInputSN();
	}
		
	CDialog::OnTimer(nIDEvent);
}

BOOL CSM224UpdatedSNToolMPDlg::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	if ((pMsg->message == WM_KEYDOWN) || (pMsg->message == WM_SYSKEYDOWN))
	{
		TCHAR tszKeyName[100];
		if (GetKeyNameText(pMsg->lParam, tszKeyName, 100))
		{
			CString KeyName;
			KeyName.Format(_T("%s"), tszKeyName);
			if(KeyName == "Enter" || KeyName=="Num Enter")
			{
				//K1018 Lance add
				if(!m_is_set_sn)	OnBUTTONSetSN();
				else
				{	
					OnStartUpdatedSN(); 
					m_is_set_sn = false;
				}
				return true;
			}	
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}

void CSM224UpdatedSNToolMPDlg::ReShowMainPage_UPSN()
{
	m_btn_scan_drive->EnableWindow(TRUE);
	m_btn_start->EnableWindow(TRUE);	
	m_btn_quit->EnableWindow(TRUE);
	m_edit_input_sn->EnableWindow(TRUE);
	ClearInputSn();
}

void CSM224UpdatedSNToolMPDlg::SetCursor_UPSN()
{
	m_edit_input_sn->SetFocus();
}

void CSM224UpdatedSNToolMPDlg::OnChangeEDITInputSN() 
{
	m_btn_set_sn->EnableWindow(TRUE);
	m_btn_start->EnableWindow(FALSE);
	m_is_set_sn = false; //L0130 Lance add for 20120117 NECi issue3 request
}

void CSM224UpdatedSNToolMPDlg::OnBUTTONSetSN() 
{
	// TODO: Add your control notification handler code here
	m_edit_input_sn->GetWindowText(m_input_sn);
	m_input_sn.TrimRight();
	m_btn_set_sn->EnableWindow(FALSE);
	m_btn_start->EnableWindow(TRUE);
	m_is_set_sn = true;//K1024 Lance add
	//L0130 Lance add for NECi 20120117 request
	SetStatus(COLOR_BLUE, _T("Ready"));
}

void CSM224UpdatedSNToolMPDlg::OnBUTTONClearCnt() 
{
	// TODO: Add your control notification handler code here
	CWnd* pWnd = GetParent();
	int iLimitCnt=0;
	int iValMesBox=0;
	//L0130 Lance add for NECi request
	iValMesBox=MessageBox(_T("                        Clear Counts?"), _T("SMI. Update Serial Number Tool"), MB_OKCANCEL);
	if ( IDOK == iValMesBox ) UpdateCnt_UPSNTool(0);
}

HBRUSH CSM224UpdatedSNToolMPDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	pDC->SetBkMode(OPAQUE);
	// TODO: Change any attributes of the DC here
	if(nCtlColor == CTLCOLOR_STATIC)
	{
		switch(pWnd->GetDlgCtrlID())
		{
		case IDC_STATIC_CurrentCnt:
		    if( (m_count_limit - m_count_current) <= 20 )	pDC->SetBkColor(RGB(240 , 25 , 25));//color red
		    else		pDC->SetBkColor(RGB(170 , 170 , 170));	//color grey	
			break;	
				 
		case IDC_STATIC_LimitCnt:
			 pDC->SetBkColor(RGB(170 , 170 , 170));	//color grey
			 break;		 
		}
	}
	// TODO: Return a different brush if the default is not desired
	return hbr;
}

bool CSM224UpdatedSNToolMPDlg::Reset_UPSNTool(HANDLE hDisk)
{
	SetStatus(COLOR_BLUE, _T("Card Power Off"));

	SMITesterPowerOff(hDisk, m_tester);
	SMITesterPowerOn(hDisk, m_tester);

	SetStatus(COLOR_BLUE, _T("Card Power on"));
	return true;
}

int CSM224UpdatedSNToolMPDlg::VerifySN_UPSNTool(HANDLE hDisk, bool verify_sn)
{
	UCHAR IDTable[512];
	HANDLE hRead= INVALID_HANDLE_VALUE;
	HANDLE hWrite= INVALID_HANDLE_VALUE;
    BYTE AdapterID=0;
    BYTE TargetID=0;

	ULONG iCapacity_C1;
	CHAR	vendor_specific_cid[VENDOR_LENGTH + 1];
	int api_ret;

	//get idtable
	memset(IDTable,0,sizeof(IDTable));
	api_ret = Get_IDENTIFY(FALSE, hRead, hWrite, hDisk, IDTable);
	if (0 == api_ret)	return (UPSN_VERIFYSN_FAIL);

	//capacity
	iCapacity_C1=((IDTable[0x7B]<<24)+(IDTable[0x7A]<<16)+(IDTable[0x79]<<8)+(IDTable[0x78]));
	if(m_capacity != iCapacity_C1) return (UPSN_CheckCapacity_FAIL);

	//compare vendor specific
	for(int ii = 0; ii < VENDOR_LENGTH_WORD; ii++)
	{
		vendor_specific_cid[ii * 2]=IDTable[0x103 + ii * 2];
		vendor_specific_cid[(ii * 2) + 1]=IDTable[0x102 + ii*2];
	}
	if ( 0 != memcmp(vendor_specific_cid, m_vendor_specific, VENDOR_LENGTH) )
		return (UPSN_VERIFYvendorspecfic_FAIL);

	// compare model name
	if ( !ConvertStringCompare(m_oem_model_name, IDTable + 0x36, MODEL_LENGTH) )
		return (UPSN_CheckModelName_FAIL);

	//compare sn
	if ( verify_sn)
	{
		LOG_DEBUG(_T("verify sn in idtable"));
		if ( !ConvertStringCompare(m_input_sn, IDTable + 0x14, SN_LENGTH) )
			return (UPSN_VERIFYSN_FAIL);
	}

	//check IF Setting word76 bit1~3
	BYTE speed = 0;

	if(m_sata_speed == 1) speed = 0x02;	//only support gen 1
	else if(m_sata_speed == 2)	speed = 0x06;	//support gen 1/2
    if((IDTable[152] & 0x0E) != speed)	return (UPSN_VERIFY_IF_FAIL);

	//check TRIM word169 bit1
	int trim_idtable = (IDTable[338] & 0x01);
	if (m_trim_enable != trim_idtable)	return (UPSN_VERIFY_TRIM_FAIL);

	//check device sleep word78 bit8
	int devslp_idtable = IDTable[157] & 0x01;
	if (m_devslp_enable != devslp_idtable)	return (UPSN_VERIFY_DEVSLP_FAIL);

	// check firmware version
	LOG_DEBUG(_T("check f/w version in idtable") );
	if ( !ConvertStringCompare(m_fw_ver, IDTable + FWVER_OFFSET, FWVER_LENGTH, 0) )
		return (UPSN_VERIFY_FW_VERSION);

	return UPSN_VERIFYSN_PASS;
}

void CSM224UpdatedSNToolMPDlg::UpdateCnt_UPSNTool(int new_count)
{
	UpdateData();
	m_count_current = new_count;
	m_str_count_current.Format(_T("%d"), m_count_current);
	WritePrivateProfileString(SEC_PARAMETER, KEY_COUNT_CURRENT, m_str_count_current, FILE_COUNT_CURRENT);
	UpdateData(FALSE);
}

DWORD CSM224UpdatedSNToolMPDlg::UpsnLoadIspFile(HANDLE device, BYTE * isp_buf, DWORD len)
{
	LOG_STACK_TRACE();
	// load isp from file
	FILE* stream = NULL;
	LOG_DEBUG(_T("open isp file : %s"), m_isp_file_name);
	_tfopen_s(&stream, m_isp_file_name, _T("rb"));
	if ( stream == NULL)
	{
		AfxMessageBox(_T("open ISP file error!!\n") );
		throw new CUpsnError(UPSN_OPENFILE_FAIL);
	}
	DWORD isplength = fread(isp_buf, sizeof(BYTE), len, stream );		
	fclose( stream );
	if (0 == isplength)		throw new CUpsnError(UPSN_OPENFILE_FAIL);

	// check sum
	DWORD check_sum=CheckSum(isp_buf, isplength);
	LOG_DEBUG(_T("isp checksum=:0x%08X"), check_sum);
	LOG_DEBUG(_T("checksum in config=:0x%08X"), m_isp_check_sum);
	//for(DWORD ii=0; ii< isplength; ii++)	check_sum += isp_buf[ii];

	if(m_isp_check_sum != check_sum )		throw new CUpsnError(UPSN_CheckSumNoMatch_FAIL);

	// check isp version
	if (!m_disable_isp_check_version)
	{
		LOG_DEBUG(_T("check isp version"));

		BYTE   isp_ver_device[ISP_VER_LENGTH + 10];
		memset(isp_ver_device, 0, ISP_VER_LENGTH + 10);
		int ir = Get_IC_ISPVer(	FALSE, 
			INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 
			device, isp_ver_device); //dll
		LOG_DEBUG(_T("device isp version: %S"), isp_ver_device);

		CString str_ver((char*)(isp_buf + ISP_VER_OFFSET), ISP_VER_LENGTH);
		LOG_DEBUG(_T("file isp version: %s"), str_ver)
		
		if ( !ir || 
			memcmp(isp_ver_device, isp_buf + ISP_VER_OFFSET, ISP_VER_LENGTH) != 0)
		{
			throw new CUpsnError(UPSN_ISPVersionNoMatch_FAIL);
		}
	}
	return isplength;
}


bool CSM224UpdatedSNToolMPDlg::UPSN_UpdateISP2244LT(HANDLE device, BYTE * isp_buf, DWORD isp_len)
{
	CString ISPName;

	//depend on config.txt to create new isp bin

	//backup new isp bin
	ISPName.Format(ISP_BACKUP_FILE_W, (m_model_index+1));
	CString file_name = ISP_BACKUP_PATH;
	file_name += ISPName;

	//write ispbuf to bin file
	FILE * stream = NULL;
	_tfopen_s(&stream, file_name, _T("wb+"));
	if (NULL == stream) throw new CUpsnError(UPSN_WRITEFILE_FAIL);
	fwrite(isp_buf, sizeof(BYTE),isp_len, stream );
	fclose( stream );
	stream = NULL;

	int retry_count = 0;
	int api_ret=0;
	while ( retry_count < RETRY_TIMES)
	{
		api_ret = Update_ISP(FALSE, 
			INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 
			device, m_mpisp_len, m_mpisp, isp_len, isp_buf); //dll
		if (api_ret) break;
		retry_count ++;
	}
	if(0 == api_ret)	throw new CUpsnError(UPSN_DOWNLOADSN_FAIL);

	return true;
}

bool CSM224UpdatedSNToolMPDlg::UPSN_UpdateISPfromConfig(BYTE *ISPBuf, DWORD len)
{
	LOG_STACK_TRACE();
    bool success = true;

	FillConvertString(m_oem_model_name, ISPBuf + MODEL_START_ADDR_2244LT, MODEL_LENGTH);

	//save capacity to isp buffer
	ISPBuf[CHS_START_ADDR_2244LT] = (UCHAR)((m_capacity & 0xFF0000) >> 16);
	ISPBuf[CHS_START_ADDR_2244LT+1] = (UCHAR)((m_capacity & 0xFF000000) >> 24);
	ISPBuf[CHS_START_ADDR_2244LT+2] = (UCHAR)(m_capacity & 0xFF);
	ISPBuf[CHS_START_ADDR_2244LT+3] = (UCHAR)((m_capacity & 0xFF00) >> 8);

	//I/F Setting
	if(m_sata_speed == 1)
	{
		ISPBuf[IF_ADDR_2244LT] = ISPBuf[IF_ADDR_2244LT] | 0x10; //force sata gen1
	}
	else if(m_sata_speed == 2)
	{
		ISPBuf[IF_ADDR_2244LT] = ISPBuf[IF_ADDR_2244LT] & 0xE0;
	}

	//TRIM
	if(0 == m_trim_enable )		ISPBuf[TRIM_ADDR_2244LT] = ISPBuf[TRIM_ADDR_2244LT] & 0xF7;	// not support
	else	ISPBuf[TRIM_ADDR_2244LT] = ISPBuf[TRIM_ADDR_2244LT] | 0x08;

	//DEVSLP
	if (0 == m_devslp_enable)		ISPBuf[DEVSLP_ADDR_2244LT] = ISPBuf[DEVSLP_ADDR_2244LT] & 0xEF;	// not support
	else					ISPBuf[DEVSLP_ADDR_2244LT] = ISPBuf[DEVSLP_ADDR_2244LT] | 0x10;


	//save sn to isp buffer
	FillConvertString(m_input_sn, ISPBuf + SN_START_ADDR_2244LT, SN_LENGTH);

	//save vendor specific to isp buffer
	for(int ii = 0; ii < VENDOR_LENGTH; ii += 2)
	{
		ISPBuf[VENDOR_START_ADDR_2244LT+ii]= m_vendor_specific[ii+1];
		ISPBuf[VENDOR_START_ADDR_2244LT+ii+1]= m_vendor_specific[ii];
	}

	//fix isp ic temperature offset
	ISPBuf[0x9B0] = 0x0;
	return success;
}

bool CSM224UpdatedSNToolMPDlg::UPSN_ReadISP2244LT(HANDLE device, const BYTE * isp_buf, DWORD isp_len)
{
	LOG_STACK_TRACE();

	bool success=true;	

	stdext::auto_array<BYTE> ISPReadBuf1(ISP_LENGTH);
	stdext::auto_array<BYTE> ISPReadBuf2(ISP_LENGTH);

	memset(ISPReadBuf1, 0x0, ISP_LENGTH);
	memset(ISPReadBuf2, 0x0, ISP_LENGTH);

	CString ISPName;
	CString file_name;

	LOG_DEBUG(_T("read isp from Ferri"))
	int api_ret = Read_ISP(FALSE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 
		device, isp_len, ISPReadBuf1, ISPReadBuf2); //dll
	if(0 == api_ret)	throw new CUpsnError(UPSN_ReadISP_FAIL);

	//backup read isp
	ISPName.Format(ISP_BACKUP_FILE_R, (m_model_index+1));
	file_name = ISP_BACKUP_PATH;
	file_name += ISPName;

	FILE* stream = NULL;
	LOG_DEBUG(_T("dump isp3 to %s"), file_name);
	_tfopen_s(&stream, file_name, _T("wb+") );
	if (NULL == stream)	throw new CUpsnError(UPSN_WRITEFILE_FAIL);
	fwrite(ISPReadBuf1, sizeof(unsigned char), isp_len, stream );
	fclose( stream );
	
	// compare
	if ( memcmp(isp_buf, ISPReadBuf1, isp_len) != 0 )
	{
		LOG_ERROR(_T("isp 3 and source isp are different!"));
		throw new CUpsnError(UPSN_WriteISP_Compare_FAIL);
	}

	if ( memcmp(isp_buf, ISPReadBuf2, isp_len) != 0 )
	{
		LOG_ERROR(_T("isp 4 and source isp are different!"));
		throw new CUpsnError(UPSN_WriteISP_Compare_FAIL);
	}

	return success;
}

bool CSM224UpdatedSNToolMPDlg::CheckFlashID(HANDLE device)
{
	LOG_STACK_TRACE();
	ASSERT(NULL != device && INVALID_HANDLE_VALUE != device);

	bool success = true;
	UCHAR FlashIDBuf[SECTOR_SIZE]={0};
	int api_ret = 0;
	memset(FlashIDBuf,0x0,SECTOR_SIZE);
	api_ret = Read_FlashID(FALSE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 
		device, FlashIDBuf);

	success = false;
	if( (1 == api_ret) &&
		(0 == memcmp(FlashIDBuf+FLASH_ID_OFFSET, m_flash_id + FLASH_ID_OFFSET, FLASH_ID_SIZE) ) )
	{
		success = true;
	}
	else LOG_ERROR(_T("flash id do not match"));
	return success;
}

bool CSM224UpdatedSNToolMPDlg::LoadGlobalConfig(LPCTSTR sec, LPCTSTR key, CString & val)
{
	CSM224testBApp * app = dynamic_cast<CSM224testBApp *>(AfxGetApp());
	JCASSERT(app);
	CString model_file = app->GetRunFolder() + FILE_MODEL_LIST;

	TCHAR str[MAX_STRING_LEN];
	DWORD ir = GetPrivateProfileString(sec, key, NULL, str, MAX_STRING_LEN, model_file);
	if (0 == ir) THROW_ERROR(ERR_PARAMETER, _T("failure on loading global config %s/%s"), sec, key);
	val = str;
	LOG_DEBUG(_T("global config %s/%s=%s"), sec, key, val);
	return true;
}

bool CSM224UpdatedSNToolMPDlg::LoadGlobalConfig(LPCTSTR sec, LPCTSTR key, int & val, int def_val)
{
	CSM224testBApp * app = dynamic_cast<CSM224testBApp *>(AfxGetApp());
	JCASSERT(app);
	CString model_file = app->GetRunFolder() + FILE_MODEL_LIST;

	val = GetPrivateProfileInt(sec, key, def_val, model_file);
	if (-1 == val) THROW_ERROR(ERR_PARAMETER, _T("failure on loading local config %s/%s"), sec, key);
	LOG_DEBUG(_T("local config %s/%s=%d"), sec, key, val);
	return true;
}

bool CSM224UpdatedSNToolMPDlg::LoadLocalConfig(LPCTSTR sec, LPCTSTR key, CString & val)
{
	TCHAR str[MAX_STRING_LEN];
	DWORD ir = GetPrivateProfileString(sec, key, NULL, str, MAX_STRING_LEN, m_config_file);
	if (0 == ir) THROW_ERROR(ERR_PARAMETER, _T("failure on loading local config %s/%s"), sec, key);
	val = str;
	LOG_DEBUG(_T("local config %s/%s=%s"), sec, key, val);
	return true;
}

bool CSM224UpdatedSNToolMPDlg::LoadLocalConfig(LPCTSTR sec, LPCTSTR key, BYTE * buf, DWORD &len, int checksum)
{
	TCHAR filename[MAX_STRING_LEN];
	DWORD ir = GetPrivateProfileString(sec, key, NULL, filename, MAX_STRING_LEN, m_config_file);
	if (ir == 0) THROW_ERROR(ERR_PARAMETER, _T("failure on loading local config %s/%s"), sec, key);

	FILE * ffile = NULL;
	_tfopen_s(&ffile, filename, _T("rb"));
	if ( NULL == ffile )	THROW_ERROR(ERR_PARAMETER, _T("failure on openning file %s"), filename);
	ir = fread(buf, 1, len, ffile);
	fclose(ffile);
	len = ir;
	DWORD file_checksum = CheckSum(buf, len);

	LOG_DEBUG(_T("load bin file %s, length=%d, checksum=0x%08X"), filename, len, file_checksum);
	LOG_DEBUG(_T("input checksum=0x%08X"), checksum)

	if (checksum && (DWORD)(checksum) != file_checksum)
	{
		LOG_ERROR(_T("checksum failed, input:0x%08X, file:0x%08X"), checksum, file_checksum);
		THROW_ERROR(ERR_PARAMETER, _T("%s checksum do not match"), filename);
	}
	//file_name = str;
	return true;
}

bool CSM224UpdatedSNToolMPDlg::LoadLocalConfig(LPCTSTR sec, LPCTSTR key, int & val, int def_val)
{
	val = GetPrivateProfileInt(sec, key, def_val, m_config_file);
	if (-1 == val) THROW_ERROR(ERR_PARAMETER, _T("failure on loading local config %s/%s"), sec, key);
	LOG_DEBUG(_T("local config %s/%s=%d"), sec, key, val);
	return true;
}

DWORD CSM224UpdatedSNToolMPDlg::CheckSum(BYTE * buf, DWORD len)
{
	DWORD check_sum=0;
	for(DWORD ii=0; ii< len; ii++)	check_sum += buf[ii];
	return check_sum;
}

bool CSM224UpdatedSNToolMPDlg::LoadConfig(void)
{
	TCHAR str[MAX_STRING_LEN];
	DWORD ir = 0;
	FILE *ffile = NULL;

	CSM224testBApp * app = dynamic_cast<CSM224testBApp *>(AfxGetApp());
	JCASSERT(app);


	// model list file
	CString str_sec, str_temp;
	str_sec.Format(_T("MODEL%02d"), m_model_index + 1);
	
	// serial number prefix for global, ex: EMBZ
	LoadGlobalConfig(MODLE_LIST_MODELS,		KEY_PREFIX,			m_global_prefix);	
	LoadGlobalConfig(SEC_EXTPROC,			KEY_EXT_CMD,		m_external_cmd);	
	LoadGlobalConfig(SEC_EXTPROC,			KEY_EXT_PATH,		m_external_path);	
	LoadGlobalConfig(SEC_EXTPROC,			KEY_EXT_PARAM,		m_external_param);	
	LoadGlobalConfig(SEC_EXTPROC,			KEY_EXT_SHOW,		m_external_show_wnd);	
	LoadGlobalConfig(SEC_EXTPROC,			KEY_EXT_TIMEOUT,	m_external_timeout);	
	LoadGlobalConfig(str_sec,				KEY_MODEL_NAME,		m_model_name);
	LoadGlobalConfig(str_sec,				KEY_CONFIG_FILE,	str_temp);

	m_config_file = app->GetRunFolder() + str_temp;
	if ( GetFileAttributes(m_config_file) == -1 )	
		THROW_ERROR(ERR_PARAMETER, _T("failure on openning config file %s"), m_config_file);

	m_model_name.TrimRight();

	// local config file
	DWORD vendor_len = VENDOR_LENGTH;
	DWORD fid_len = SECTOR_SIZE;
	m_mpisp_len = MPISP_LENGTH;
	int mpisp_checksum = 0;
	int flashid_checksum = 0;

	LoadLocalConfig(SEC_SOURCE,		KEY_VENDOR,			m_vendor_specific,	vendor_len);

	LoadLocalConfig(SEC_SOURCE,		KEY_FLASHID_CHECKSUM,flashid_checksum,	0);
	LoadLocalConfig(SEC_SOURCE,		KEY_FLASHID,		m_flash_id,			fid_len,	flashid_checksum);
	LoadLocalConfig(SEC_SOURCE,		KEY_MPISP_CHECKSUM,	mpisp_checksum,		0);
	LoadLocalConfig(SEC_SOURCE,		KEY_MPISP,			m_mpisp,			m_mpisp_len, mpisp_checksum);
	LoadLocalConfig(SEC_SOURCE,		KEY_ISP_CHECKSUM,	m_isp_check_sum);
	LoadLocalConfig(SEC_SOURCE,		KEY_ISP,			m_isp_file_name);

	LoadLocalConfig(SEC_CONFIGURE,	KEY_OEM_MODEL_NAME,	m_oem_model_name);
	LoadLocalConfig(SEC_CONFIGURE,	KEY_SN_PREFIX,		m_sn_prefix);		// serial number prefix, ex. EMBZ1
	LoadLocalConfig(SEC_CONFIGURE,	KEY_CAPACITY,		m_capacity);
	LoadLocalConfig(SEC_CONFIGURE,	KEY_IF_SETTING,		m_sata_speed);
	LoadLocalConfig(SEC_CONFIGURE,	KEY_TRIM,			m_trim_enable);
	LoadLocalConfig(SEC_CONFIGURE,	KEY_DEVSLP,			m_devslp_enable);
	LoadLocalConfig(SEC_CONFIGURE,	KEY_DIS_ISP_CHECK,	m_disable_isp_check_version);
	LoadLocalConfig(SEC_CONFIGURE,	KEY_FW_VER,			m_fw_ver);

	// create direct for isp backup
	if( GetFileAttributes(ISP_BACKUP_PATH) == -1)	CreateDirectory(ISP_BACKUP_PATH, NULL);

	// Load log configuration
	ir = GetPrivateProfileString(SEC_PARAMETER, KEY_DEVICE_NAME, _T(""), str, MAX_STRING_LEN, app->GetRunFolder() + FILE_DEVICE_NAME);
	m_log_file.SetDeviceName(str);
	ir = GetPrivateProfileString(SEC_PARAMETER, KEY_TEST_MACHINE, _T(""), str, MAX_STRING_LEN, app->GetRunFolder() + FILE_TEST_MACHINE_NO);
	m_log_file.SetTestMachine(str);

	ir = GetPrivateProfileInt(SEC_PARAMETER, KEY_PATA_SATA, 0, app->GetRunFolder() + FILE_DEVICE_TYPE);
	if (ir ==0)	m_controller_ver = SM631GXx;		// (SATA) SM2244LT;

	ir = GetPrivateProfileString(SEC_PARAMETER, KEY_DEVICE_TYPE, _T(""), str, MAX_STRING_LEN, app->GetRunFolder() + FILE_DEVICE_TYPE);
	m_log_file.SetDeviceType(str);
	m_log_file.SetToolVer(app->GetVer());

	CString log1;
	CString log2;
	char str_path[MAX_STRING_LEN];
	_tfopen_s(&ffile, FILE_LOG1_PATH, _T("rb") );
	if( ffile == NULL )		THROW_ERROR(ERR_PARAMETER, _T("Open SaveLOG1Path.dat Fail! "));
	memset(str_path, 0, MAX_STRING_LEN);
	fread(str_path, sizeof(char), MAX_STRING_LEN, ffile);
	log1.Format(_T("%S"), str_path);
	log1.TrimRight();
	fclose(ffile);
	ffile = NULL;

	_tfopen_s(&ffile, FILE_LOG2_PATH, _T("rb") );
	if( ffile == NULL )		THROW_ERROR(ERR_PARAMETER, _T("Open SaveLOG2Path.dat Fail! "));
	memset(str_path, 0, MAX_STRING_LEN);
	fread(str_path, sizeof(char), MAX_STRING_LEN, ffile);
	log2.Format(_T("%S"), str_path);
	log2.TrimRight();
	fclose(ffile);
	ffile = NULL;

	m_log_file.SetLogPath(log1, log2);
	return true;
}


bool CSM224UpdatedSNToolMPDlg::ConvertStringCompare(const CString & src, BYTE * dst, DWORD len, char filler)
{
	JCASSERT(len < MAX_STRING_LEN);
	BYTE temp[MAX_STRING_LEN];
	memset(temp, filler, MAX_STRING_LEN);
	FillConvertString(src, temp, len, filler);
	temp[len] = 0;

	CStringA d((char*)dst, len);
	LOG_DEBUG(_T("target=\"%S\", val=\"%S\""), temp, d);

	return ( memcmp(temp, dst, len)==0 );
}

void CSM224UpdatedSNToolMPDlg::FillConvertString(const CString & src, BYTE * dst, DWORD len, char filler)
{
	memset(dst, filler, len);
	DWORD src_len = src.GetLength();
	LPCTSTR str_src = src;
	// copy src to dst ( unicode -> UTF8)
	for (DWORD ii = 0; ii < src_len; ++ii)	dst[ii] = (BYTE)(str_src[ii]);
	// convert byte order
	BYTE t;
	for (DWORD ii = 0; ii < len; ii += 2)		t=dst[ii], dst[ii]=dst[ii+1], dst[ii+1]=t ;
}

void CSM224UpdatedSNToolMPDlg::ReadConvertString(const BYTE * src, CString & dst, DWORD len)
{
	BYTE temp[MAX_STRING_LEN];
	memset(temp, 0, MAX_STRING_LEN);
	// convert byte orcer
	for (DWORD ii = 0; ii < len; ii += 2)	temp[ii] = src[ii+1], temp[ii+1] = src[ii];
	dst.Format(_T("%S"), temp);
	dst.TrimRight();
}

void CSM224UpdatedSNToolMPDlg::RetrieveController(void)
{
	m_edit_input_sn = (CEdit*)(GetDlgItem(IDC_EDIT_InputSN) );
	ASSERT(m_edit_input_sn);

	m_btn_start = (CButton*)(GetDlgItem(ID_Start) );
	ASSERT(m_btn_start);

	m_btn_scan_drive = (CButton*)(GetDlgItem(IDC_BUTTON_ScanDrive) );
	ASSERT(m_btn_scan_drive);

	m_btn_quit = (CButton*)(GetDlgItem(ID_Quit) );
	ASSERT(m_btn_quit);

	m_btn_set_sn = (CButton*)(GetDlgItem(IDC_BUTTON_SetSN) );
	ASSERT(m_btn_set_sn);
}
