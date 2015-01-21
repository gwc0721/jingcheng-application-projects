// SM224UpdatedSNToolMPDlg.cpp : implementation file
#include "stdafx.h"
#include <stdext.h>

LOCAL_LOGGER_ENABLE(_T("upsn"), LOGGER_LEVEL_NOTICE);

#include "UpdateSnTool.h"
#include "UpdateSnToolDlg.h"
#include "UpsnMessageDlg.h"
#include "UpsnCaution.h"
#include "ColorListCtrl.h"

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
// CUpdateSnToolDlg dialog
CUpdateSnToolDlg::CUpdateSnToolDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUpdateSnToolDlg::IDD, pParent)
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
	, m_ferri_factory(NULL)
	, m_flash_id(NULL)
{
	memset(m_vendor_specific, 0, VENDOR_LENGTH + 1);
}


void CUpdateSnToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_SNUpdatedTool, m_ListUpdatedSNTool);
	DDX_Text(pDX, IDC_TESTMODULE, m_str_test_model_name);
	DDX_Text(pDX, IDC_STATIC_CurrentCnt, m_str_count_current);
	DDX_Text(pDX, IDC_STATIC_LimitCnt, m_str_count_limit);
	DDX_Text(pDX, IDC_FWREV, m_str_fw_rev);
}


BEGIN_MESSAGE_MAP(CUpdateSnToolDlg, CDialog)
	ON_BN_CLICKED(ID_Start, OnStartUpdatedSN)
	ON_BN_CLICKED(ID_Quit, OnQuit)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_EN_CHANGE(IDC_EDIT_InputSN, OnEditInputSn)
	ON_BN_CLICKED(IDC_BUTTON_SetSN, OnButtonSetSn)
	ON_BN_CLICKED(IDC_BUTTON_ClearCnt, OnButtonClearCnt)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUpdateSnToolDlg message handlers

BOOL CUpdateSnToolDlg::OnInitDialog() 
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
		CUpdateSnToolApp * app = dynamic_cast<CUpdateSnToolApp *>(AfxGetApp());
		ASSERT(app);

		str_title.Format(_T("SMI. Update Serial Number Tool  Ver. %s"), app->GetVer() );
		this->SetWindowText(str_title);

		ListView_SetExtendedListViewStyle(m_ListUpdatedSNTool, LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);

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
		
		// Load SDK
		InitializeSdk(m_controller, m_ferri_factory);
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

void CUpdateSnToolDlg::SetStatus(COLORREF cr, LPCTSTR status)
{
	m_ListUpdatedSNTool.SetTextColor(cr);
	m_ListUpdatedSNTool.SetItemText(0,List_STATUS, status);
	PassAllMessage();
}


void CUpdateSnToolDlg::OnStartUpdatedSN() 
{
	m_btn_scan_drive->EnableWindow(TRUE);
	m_btn_start->EnableWindow(FALSE);	

	IFerriDevice * ferri_dev = NULL;

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
	CUpsnMessageDlg UPSN_MessageBox;
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

	bool br = false;
	try
	{
		LOG_NOTICE(_T("start update sn, count %d / %d"), m_count_current, m_count_limit);
		LOG_DEBUG(_T("Test Mode: %d"), m_test_mode); 
		
		SetStatus(COLOR_BLUE, _T("Scanning.."));

		m_ferri_factory->ScanDevice(ferri_dev);
		CString str_port;
		str_port.Format(_T("Port%d"), ferri_dev->GetTesterPort() );
		m_ListUpdatedSNTool.SetItemText(0,0, str_port);
		PassAllMessage();

		if (!ferri_dev)
		{
			MessageBox(_T("Please Check Device and Scan Drive again"), _T(""), MB_OK);
			throw new CUpsnWarning;
		}
		ferri_dev->SetMpisp(m_mpisp, m_mpisp_len);
		info.m_drive_letter = ferri_dev->GetDriveLetter();
		CJCStringT str_drive_name;
		ferri_dev->GetDriveName(str_drive_name);
		info.m_drive_name = str_drive_name.c_str();

		SetStatus(COLOR_BLUE, _T("Loading...") );

		start_time = CTime::GetCurrentTime();
		Sleep(500);


		info.m_init_bad = ferri_dev->GetInitialiBadBlockCount();
		stdext::auto_array<BYTE> identify_buf(SECTOR_SIZE);
		memset(identify_buf, 0, sizeof(SECTOR_SIZE));
		br = ferri_dev->ReadIdentify(identify_buf, SECTOR_SIZE);
		if (!br)
		{
			MessageBox(_T("Failure on reading id table"), _T(""), MB_OK);
			throw new CUpsnWarning;
		}

		// get current serial number and show
		CString current_sn;
		ReadConvertString(identify_buf + SN_OFFSET_ID, current_sn, SN_LENGTH);
		m_ListUpdatedSNTool.SetItemText(0, List_CurrentSN, current_sn);

		// get model name and show
		CString current_model_name;
		ReadConvertString(identify_buf + MODEL_OFFSET_ID, current_model_name, MODEL_LENGTH);
		m_ListUpdatedSNTool.SetItemText(0, List_ModelName, current_model_name);

		//--

		bool verify_sn = true;
		LOG_DEBUG(_T("input sn = %s"), m_input_sn);
		if ( ( (MODE_VERIFY_ONLY == m_test_mode) && (m_input_sn == m_global_prefix) ) )	
		{
			LOG_DEBUG(_T("skip verify sn"))
			verify_sn = false;
			info.m_serial_number = current_sn;
			info.m_model_name = current_model_name;
		}
		else
		{	// check sn
			verify_sn = true;
			bool check_flash_id = true;
			if (m_flash_id)	check_flash_id = ferri_dev->CheckFlashId(m_flash_id, SECTOR_SIZE);
			else			LOG_WARNING(_T("ignore checking flash id."));
			if ( (m_input_sn.Left(5) != m_sn_prefix) || ( !check_flash_id ) )
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
		int	ir = UpsnUpdateToDevice(ferri_dev, verify_sn);
		info.m_error_code = ir;
		// close device before run external cmd
		ferri_dev->Disconnect();

		SetStatus(COLOR_BLUE, _T("External test..."));
		DWORD exit_code = ExcuseExternalProcess(info);
		SetStatus(COLOR_BLUE, _T("Getting runtime bad..."));
		br = ferri_dev->Connect();
		if ( !br )
		{
			MessageBox(_T("Failure on openning device"), _T("Error!"), MB_OK);
			throw new CUpsnWarning();
		}
		info.m_new_bad = GetRunTimeBad(ferri_dev);
		if ( 0 != exit_code ) throw new CUpsnError(exit_code);
		info.m_pass_fail = true;

		SetStatus(COLOR_BLUE, _T("Complete"));

		// log1
		br = m_log_file.WriteLog1(info, start_time);
		br = m_log_file.WriteLog2(info, start_time);
		UPSN_MessageBox.DoModal( &info );
	}
	catch (stdext::CJCException &err)
	{
		int err_code = err.GetErrorID();
		CString msg;
		if ( (err_code & 0xFFFF0000) == stdext::CJCException::ERR_DEVICE)
		{
			switch (err_code & 0xFFFF)
			{
			case FERR_NO_TESTER:	
				msg = _T("No Tester!");
				info.m_error_code = UPSN_SCANDRIVE_FAIL; 
				break;

			case FERR_NO_CARD:		
				msg = _T("No Card!");
				info.m_error_code = UPSN_SCANDRIVE_FAIL; 
				break;

			case FERR_OPEN_DRIVE_FAIL:
				msg = _T("No Card!");
				info.m_error_code = UPSN_OPENDRIVE_FAIL; 
				break;

			case FERR_CONTROLLER_NOT_MATCH:
				msg = _T("Wrong Controller!");
				info.m_error_code = UPSN_OPENDRIVE_FAIL; 
				break;
			}
			SetStatus(COLOR_RED, msg);
			m_log_file.WriteLog1(info, start_time);
			UPSN_MessageBox.DoModal( &info );
		}
		else MessageBox(err.WhatT(), _T("Error!"), MB_OK);
	}
	catch (CUpsnError * pe)
	{	// error
		SetStatus(COLOR_RED, _T("Fail"));
		info.m_error_code = pe->GetErrorCode();
		bool br = m_log_file.WriteLog1(info, start_time);
		UPSN_MessageBox.DoModal( &info );
		pe->Delete();
	}
	catch (CUpsnCaution * pe)
	{
		CUpsnCautionDlg caution_dlg;
		int ir = caution_dlg.DoModal(pe->GetErrorCode() );
		if(CUpsnCautionDlg::CAUTION_RETRY == ir) 
		{
			ClearInputSn();
		}
		else if(CUpsnCautionDlg::CAUTION_CANCEL == ir)
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

	UpsnUpdateCnt(m_count_current + 1);
	ReShowMainPage();
	OnEditInputSn();
	ClearInputSn();
	Invalidate(TRUE);
	PassAllMessage();

	if (ferri_dev) ferri_dev->Release();
	ferri_dev = NULL;
	return;
}

DWORD CUpdateSnToolDlg::ExcuseExternalProcess(const DEVICE_INFO & info)
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

		default:
			THROW_ERROR(ERR_PARAMETER, _T("unknow placeholder %%%c"), holder);
		}
		str += attached_len, remain -= attached_len;
	}
	LOG_NOTICE(_T("external test: <%s\\%s %s>"), m_external_path, m_external_cmd, cmd_line);


	SHELLEXECUTEINFO shell_info = {0};
	shell_info.cbSize = sizeof(SHELLEXECUTEINFO);
	shell_info.fMask = SEE_MASK_NOCLOSEPROCESS;
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
	
	// check return code
	DWORD exit_code = 0;
	GetExitCodeProcess(shell_info.hProcess, &exit_code);
	CloseHandle(shell_info.hProcess);
	LOG_NOTICE(_T("running command finished, exit code = %d"), exit_code);

	return exit_code;
}

DWORD CUpdateSnToolDlg::GetRunTimeBad(IFerriDevice * dev)
{
	LOG_STACK_TRACE();
	JCASSERT(dev);
	stdext::auto_array<BYTE> buf(SECTOR_SIZE);
	bool br = dev->ReadSmart(buf, SECTOR_SIZE);
	if (!br)
	{
		MessageBox(_T("Failure on reading SMART."), _T("Error!"), MB_OK);
		throw new CUpsnWarning();
	}

	DWORD new_bad = MAKELONG(MAKEWORD(buf[0x1F], buf[0x20]), MAKEWORD(buf[0x20], buf[0x21]));
	LOG_DEBUG(_T("new_bad = %d"), new_bad);
	return new_bad;
}

int CUpdateSnToolDlg::UpsnUpdateToDevice(IFerriDevice * dev, bool verify_sn)
{
	LOG_STACK_TRACE();
	JCASSERT(dev);
	bool success=false;

	stdext::auto_interface<IIspBuffer> isp_buf;
	dev->CreateIspBuf(0, isp_buf);		JCASSERT(isp_buf);

	int ir = 0;
	success = UpsnLoadIspFile(dev, isp_buf);
	JCSIZE isp_data_len = isp_buf->GetSize();
	if(MODE_UPDATE_VERIFY == m_test_mode)
	{	//test mode(update sn and verify)
		UpsnUpdateIspFromConfig(isp_buf);
		success = UpsnUpdateIspFerri(dev, isp_buf);
		// verify before reset
		UpsnReadIspFerri(isp_data_len, dev, isp_buf);
		//updated success and reset device!!										
		SetStatus(COLOR_BLUE, _T("Card Power Off"));
		success = dev->ResetTester();
		SetStatus(COLOR_BLUE, _T("Card Power on"));
		if(!success)	throw new CUpsnError(UPSN_Reset_FAIL);
		//read isp back to check again
		success = UpsnReadIspFerri(isp_data_len, dev, isp_buf);
		ir = UpsnVerifySn(dev, verify_sn);
		if( ir != UPSN_VERIFYSN_PASS )	throw new CUpsnError(ir);
		ir = UPSN_PASS;
	}
	else	//only verify
	{
		ir = UpsnVerifySn(dev, verify_sn);
		if( ir != UPSN_VERIFYSN_PASS)	throw new CUpsnError(ir);
	}		
	return ir;
}

void CUpdateSnToolDlg::OnQuit() 
{
	//K1024 Lance add for save current count to file
	UpdateData();
	WritePrivateProfileString(SEC_PARAMETER, KEY_COUNT_CURRENT, m_str_count_current, FILE_COUNT_CURRENT);
	CDialog::OnCancel();

	// clean
	if (m_ferri_factory) m_ferri_factory->Release();
	m_ferri_factory = NULL;
	delete [] m_flash_id;
	m_flash_id = NULL;
}

void CUpdateSnToolDlg::OnTimer(UINT nIDEvent) 
{
	if(nIDEvent==0x543)//Re ScanDrive until 30 seconds time out
	{
		DEVICE_INFO info;
		info.m_error_code = UPSN_SCANDRIVE_TimeOut_FAIL;
		CUpsnMessageDlg UPSN_MessageBox;
		KillTimer(0x543);//kill scan drive timer
		//show error message box
		UPSN_MessageBox.DoModal(&info);
		OnEditInputSn();
	}
		
	CDialog::OnTimer(nIDEvent);
}

BOOL CUpdateSnToolDlg::PreTranslateMessage(MSG* pMsg) 
{
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
				if(!m_is_set_sn)	OnButtonSetSn();
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

void CUpdateSnToolDlg::ReShowMainPage()
{
	m_btn_scan_drive->EnableWindow(TRUE);
	m_btn_start->EnableWindow(TRUE);	
	m_btn_quit->EnableWindow(TRUE);
	m_edit_input_sn->EnableWindow(TRUE);
	ClearInputSn();
}

void CUpdateSnToolDlg::UpsnSetCurson()
{
	m_edit_input_sn->SetFocus();
}

void CUpdateSnToolDlg::OnEditInputSn() 
{
	m_btn_set_sn->EnableWindow(TRUE);
	m_btn_start->EnableWindow(FALSE);
	m_is_set_sn = false; //L0130 Lance add for 20120117 NECi issue3 request
}

void CUpdateSnToolDlg::OnButtonSetSn() 
{
	m_edit_input_sn->GetWindowText(m_input_sn);
	m_input_sn.TrimRight();
	m_btn_set_sn->EnableWindow(FALSE);
	m_btn_start->EnableWindow(TRUE);
	m_is_set_sn = true;//K1024 Lance add
	//L0130 Lance add for NECi 20120117 request
	SetStatus(COLOR_BLUE, _T("Ready"));
}

void CUpdateSnToolDlg::OnButtonClearCnt() 
{
	CWnd* pWnd = GetParent();
	int iLimitCnt=0;
	int iValMesBox=0;
	//L0130 Lance add for NECi request
	iValMesBox=MessageBox(_T("                        Clear Counts?"), _T("SMI. Update Serial Number Tool"), MB_OKCANCEL);
	if ( IDOK == iValMesBox ) UpsnUpdateCnt(0);
}

HBRUSH CUpdateSnToolDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	pDC->SetBkMode(OPAQUE);
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
	return hbr;
}

int CUpdateSnToolDlg::UpsnVerifySn(IFerriDevice * dev, bool verify_sn)
{
	stdext::auto_array<BYTE> idtable(SECTOR_SIZE);
	ULONG iCapacity_C1;
	CHAR	vendor_specific_cid[VENDOR_LENGTH + 1];

	//get idtable
	memset(idtable,0,SECTOR_SIZE);
	bool br = false;
	br = dev->ReadIdentify(idtable, SECTOR_SIZE);
	if (! br )	return (UPSN_VERIFYSN_FAIL);

	WORD * widtab = (WORD*)((BYTE*)(idtable));

	//capacity
	iCapacity_C1=((idtable[0x7B]<<24)+(idtable[0x7A]<<16)+(idtable[0x79]<<8)+(idtable[0x78]));
	if(m_capacity != iCapacity_C1) return (UPSN_CheckCapacity_FAIL);

	//compare vendor specific
	for(int ii = 0; ii < VENDOR_LENGTH_WORD; ii++)
	{
		vendor_specific_cid[ii * 2]=idtable[0x103 + ii * 2];
		vendor_specific_cid[(ii * 2) + 1]=idtable[0x102 + ii*2];
	}
	if ( 0 != memcmp(vendor_specific_cid, m_vendor_specific, VENDOR_LENGTH) )
		return (UPSN_VERIFYvendorspecfic_FAIL);

	// compare model name
	if ( !ConvertStringCompare(m_oem_model_name, idtable + 0x36, MODEL_LENGTH) )
		return (UPSN_CheckModelName_FAIL);

	//compare sn
	if ( verify_sn)
	{
		LOG_DEBUG(_T("verify sn in idtable"));
		if ( !ConvertStringCompare(m_input_sn, idtable + 0x14, SN_LENGTH) )
			return (UPSN_VERIFYSN_FAIL);
	}
	
	if (m_sata_speed > 0)
	{	//check IF Setting word76 bit1~3
		BYTE speed = 0;
		if(m_sata_speed == 1) speed = 0x02;	//only support gen 1
		else if(m_sata_speed == 2)	speed = 0x06;	//support gen 1/2
		if((idtable[152] & 0x0E) != speed)	return (UPSN_VERIFY_IF_FAIL);
	}

	if (m_trim_enable >= 0)
	{	//check TRIM word169 bit1
		int trim_idtable = (idtable[338] & 0x01);
		if (m_trim_enable != trim_idtable)	return (UPSN_VERIFY_TRIM_FAIL);
	}

	if (m_devslp_enable >= 0)
	{	//check device sleep word78 bit8
		int devslp_idtable = idtable[157] & 0x01;
		if (m_devslp_enable != devslp_idtable)	return (UPSN_VERIFY_DEVSLP_FAIL);
	}
	
	if ( m_cap_c && m_cap_h && m_cap_s)
	{	// check CHS setting
		LOG_NOTICE(_T("checking chs: val=%X,%X,%X"), widtab[IDTAB_CYLINDERS_1], widtab[IDTAB_HEADS_1], widtab[IDTAB_SECTORS_1]);
		LOG_DEBUG(_T("id_2=%d,%d,%d"), widtab[IDTAB_CYLINDERS_2], widtab[IDTAB_HEADS_2], widtab[IDTAB_SECTORS_2]);

		if ( (m_cap_c != widtab[IDTAB_CYLINDERS_1]) || (m_cap_h != widtab[IDTAB_HEADS_1]) || (m_cap_s != widtab[IDTAB_SECTORS_1]) )
			return (UPSN_VERIFY_CHS);
		if ( (m_cap_c != widtab[IDTAB_CYLINDERS_2]) || (m_cap_h != widtab[IDTAB_HEADS_2]) || (m_cap_s != widtab[IDTAB_SECTORS_2]) )
			return (UPSN_VERIFY_CHS);
	}
	if (m_udma_mode >= 0)
	{	// Get UDMA mode from id
		BYTE udma = 0;
		for (int ii = 0; ii <= m_udma_mode; ++ii)	udma <<= 1, udma |=1;
		LOG_NOTICE(_T("check udma: config=%d, exp=0x%02X, val=0x%02X"), m_udma_mode, udma, idtable[IDTAB_UDMA * 2]);
		if (udma !=	idtable[IDTAB_UDMA * 2]) return (UPSN_VERIFY_UDMA);
	}

	// check firmware version
	LOG_DEBUG(_T("check f/w version in idtable") );
	if ( !ConvertStringCompare(m_fw_ver, idtable + FWVER_OFFSET, FWVER_LENGTH, 0) )
		return (UPSN_VERIFY_FW_VERSION);

	return UPSN_VERIFYSN_PASS;
}

void CUpdateSnToolDlg::UpsnUpdateCnt(int new_count)
{
	UpdateData();
	m_count_current = new_count;
	m_str_count_current.Format(_T("%d"), m_count_current);
	WritePrivateProfileString(SEC_PARAMETER, KEY_COUNT_CURRENT, m_str_count_current, FILE_COUNT_CURRENT);
	UpdateData(FALSE);
}

bool CUpdateSnToolDlg::UpsnLoadIspFile(IFerriDevice * dev, IIspBuffer * isp_buf)
{
	LOG_STACK_TRACE();
	JCASSERT(isp_buf);
	JCASSERT(dev);
	bool br = isp_buf->LoadFromFile(m_isp_file_name);

	if (!br)
	{
		AfxMessageBox(_T("open ISP file error!!\n") );
		throw new CUpsnError(UPSN_OPENFILE_FAIL);
	}

	// check sum
	DWORD check_sum = isp_buf->CheckSum();
	LOG_DEBUG(_T("isp checksum=:0x%08X"), check_sum);
	LOG_DEBUG(_T("checksum in config=:0x%08X"), m_isp_check_sum);
	if(m_isp_check_sum != check_sum )		throw new CUpsnError(UPSN_CheckSumNoMatch_FAIL);

	// check isp version
	if (!m_disable_isp_check_version)
	{
		LOG_DEBUG(_T("check isp version"));

		BYTE   isp_ver_device[ISP_VER_LENGTH + 10];
		BYTE   isp_ver_isp[ISP_VER_LENGTH + 10];
		memset(isp_ver_device, 0, ISP_VER_LENGTH + 10);
		br = dev->GetIspVersion(isp_ver_device, ISP_VER_LENGTH + 10);
		LOG_DEBUG(_T("device isp version: %S"), isp_ver_device);
		br = isp_buf->GetIspVersion(isp_ver_isp, ISP_VER_LENGTH + 10);
		CString str_ver((const char*)(isp_ver_isp), ISP_VER_LENGTH);
		LOG_DEBUG(_T("file isp version: %s"), str_ver)
		
		if ( !br || 
			memcmp(isp_ver_device, isp_ver_isp, ISP_VER_LENGTH) != 0)
		{
			throw new CUpsnError(UPSN_ISPVersionNoMatch_FAIL);
		}
	}
	return true;
}


bool CUpdateSnToolDlg::UpsnUpdateIspFerri(IFerriDevice * dev, IIspBuffer * isp_buf)
{
	JCASSERT(dev);
	JCASSERT(isp_buf);

	CString ISPName;
	//depend on config.txt to create new isp bin
	//backup new isp bin
	ISPName.Format(ISP_BACKUP_FILE_W, (m_model_index+1));
	CString file_name = ISP_BACKUP_PATH;
	file_name += ISPName;

	//write ispbuf to bin file
	bool br = isp_buf->SaveToFile(file_name);
	if (!br) throw new CUpsnError(UPSN_WRITEFILE_FAIL);

	int retry_count = 0;
	br = false;
	while ( retry_count < RETRY_TIMES)
	{
		br = dev->DownloadIsp(isp_buf);
		if (br) break;
		retry_count ++;
	}
	if(!br)	throw new CUpsnError(UPSN_DOWNLOADSN_FAIL);

	return true;
}

bool CUpdateSnToolDlg::UpsnUpdateIspFromConfig(IIspBuffer * isp_buf)
{
	LOG_STACK_TRACE();
	JCASSERT(isp_buf);

    bool success = true;

	// model name
	isp_buf->SetModelName(m_oem_model_name, MODEL_LENGTH);
	//save capacity to isp buffer
	isp_buf->SetCapacity(m_capacity);

	//I/F Setting
	if (m_sata_speed)		isp_buf->SetConfig(FCONFIG_SATA_SPEED, m_sata_speed);
	//TRIM
	if (m_trim_enable >=0)	isp_buf->SetConfig(FCONFIG_TRIM, m_trim_enable);
	//DEVSLP
	if (m_devslp_enable >= 0)	isp_buf->SetConfig(FCONFIG_DEVSLP, m_devslp_enable);
	// CHS
	if ( m_cap_c && m_cap_h && m_cap_s)
	{
		isp_buf->SetConfig(FCONFIG_CYLINDERS, m_cap_c);
		isp_buf->SetConfig(FCONFIG_HEDERS, m_cap_h);
		isp_buf->SetConfig(FCONFIG_SECTORS, m_cap_s);
	}
	if (m_udma_mode >= 0)		isp_buf->SetConfig(FCONFIG_UDMA_LEVEL, m_udma_mode);

	//save sn to isp buffer
	isp_buf->SetSerianNumber(m_input_sn, SN_LENGTH);
	//save vendor specific to isp buffer
	isp_buf->SetVendorSpecific(m_vendor_specific, VENDOR_LENGTH);
	//fix isp ic temperature offset
	isp_buf->SetConfig(FCONFIG_TEMPERATURE, 0);
	return success;
}

bool CUpdateSnToolDlg::UpsnReadIspFerri(JCSIZE data_len, IFerriDevice * dev, IIspBuffer * isp_ref)
{
	LOG_STACK_TRACE();
	JCASSERT(dev);
	JCASSERT(isp_ref);

	bool success=true;	

	stdext::auto_interface<IIspBuffer> isp_read_1;
	stdext::auto_interface<IIspBuffer> isp_read_2;

	CString ISPName;
	CString file_name;

	LOG_DEBUG(_T("read isp from Ferri"));
	bool br = dev->ReadIsp(data_len, isp_read_1, isp_read_2);
	if(!br)	throw new CUpsnError(UPSN_ReadISP_FAIL);

	//backup read isp
	ISPName.Format(ISP_BACKUP_FILE_R, (m_model_index+1));
	file_name = ISP_BACKUP_PATH;
	file_name += ISPName;

	LOG_DEBUG(_T("dump isp3 to %s"), file_name);
	br = isp_read_1->SaveToFile(file_name);
	if ( !br )	throw new CUpsnError(UPSN_WRITEFILE_FAIL);
	
	// compare
	if ( !isp_ref->Compare(isp_read_1) )
	{
		LOG_ERROR(_T("isp 3 and source isp are different!"));
		throw new CUpsnError(UPSN_WriteISP_Compare_FAIL);
	}

	if ( !isp_ref->Compare(isp_read_2) )
	{
		LOG_ERROR(_T("isp 4 and source isp are different!"));
		throw new CUpsnError(UPSN_WriteISP_Compare_FAIL);
	}
	return success;
}

bool CUpdateSnToolDlg::LoadGlobalConfig(LPCTSTR sec, LPCTSTR key, CString & val, bool mandatory)
{
	CUpdateSnToolApp * app = dynamic_cast<CUpdateSnToolApp *>(AfxGetApp());
	JCASSERT(app);
	CString model_file = app->GetRunFolder() + FILE_MODEL_LIST;

	TCHAR str[MAX_STRING_LEN];
	DWORD ir = GetPrivateProfileString(sec, key, NULL, str, MAX_STRING_LEN, model_file);
	if (0 == ir)
	{
		if ( mandatory ) THROW_ERROR(ERR_PARAMETER, _T("failure on loading global config %s/%s"), sec, key);
		LOG_DEBUG(_T("no global config %s/%s"), sec, key);
	}
	else
	{
		val = str;
		LOG_DEBUG(_T("global config %s/%s=%s"), sec, key, val);
	}
	return true;
}

bool CUpdateSnToolDlg::LoadGlobalConfig(LPCTSTR sec, LPCTSTR key, int & val, int def_val)
{
	CUpdateSnToolApp * app = dynamic_cast<CUpdateSnToolApp *>(AfxGetApp());
	JCASSERT(app);
	CString model_file = app->GetRunFolder() + FILE_MODEL_LIST;

	val = GetPrivateProfileInt(sec, key, def_val, model_file);
	if (-1 == val) THROW_ERROR(ERR_PARAMETER, _T("failure on loading local config %s/%s"), sec, key);
	LOG_DEBUG(_T("local config %s/%s=%d"), sec, key, val);
	return true;
}

bool CUpdateSnToolDlg::LoadLocalConfig(LPCTSTR sec, LPCTSTR key, CString & val, bool mandatory)
{
	TCHAR str[MAX_STRING_LEN];
	DWORD ir = GetPrivateProfileString(sec, key, NULL, str, MAX_STRING_LEN, m_config_file);
	if (0 == ir)
	{
		if ( mandatory ) THROW_ERROR(ERR_PARAMETER, _T("failure on loading local config %s/%s"), sec, key);
		LOG_DEBUG(_T("no local config %s/%s"), sec, key);
	}
	val = str;
	LOG_DEBUG(_T("local config %s/%s=%s"), sec, key, val);
	return true;
}

bool CUpdateSnToolDlg::LoadLocalConfig(LPCTSTR sec, LPCTSTR key, BYTE * buf, DWORD &len, int checksum)
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

bool CUpdateSnToolDlg::LoadLocalConfig(LPCTSTR sec, LPCTSTR key, int & val, int def_val)
{
	val = GetPrivateProfileInt(sec, key, def_val, m_config_file);
	if (-1 == val) THROW_ERROR(ERR_PARAMETER, _T("failure on loading local config %s/%s"), sec, key);
	LOG_DEBUG(_T("local config %s/%s=%d"), sec, key, val);
	return true;
}

DWORD CUpdateSnToolDlg::CheckSum(BYTE * buf, DWORD len)
{
	DWORD check_sum=0;
	for(DWORD ii=0; ii< len; ii++)	check_sum += buf[ii];
	return check_sum;
}

bool CUpdateSnToolDlg::LoadConfig(void)
{
	TCHAR str[MAX_STRING_LEN];
	DWORD ir = 0;
	FILE *ffile = NULL;

	CUpdateSnToolApp * app = dynamic_cast<CUpdateSnToolApp *>(AfxGetApp());
	JCASSERT(app);


	// model list file
	CString str_sec, str_temp;
	str_sec.Format(_T("MODEL%02d"), m_model_index + 1);
	
	// serial number prefix for global, ex: EMBZ
	LoadGlobalConfig(MODLE_LIST_MODELS,		KEY_PREFIX,			m_global_prefix,	true);	
	LoadGlobalConfig(MODLE_LIST_MODELS,		KEY_CONTROLLER,			m_controller,	true);	
	LoadGlobalConfig(SEC_EXTPROC,			KEY_EXT_CMD,		m_external_cmd);	
	if ( !m_external_cmd.IsEmpty() )
	{
		LoadGlobalConfig(SEC_EXTPROC,			KEY_EXT_PATH,		m_external_path);	
		LoadGlobalConfig(SEC_EXTPROC,			KEY_EXT_PARAM,		m_external_param);	
		LoadGlobalConfig(SEC_EXTPROC,			KEY_EXT_SHOW,		m_external_show_wnd);	
		LoadGlobalConfig(SEC_EXTPROC,			KEY_EXT_TIMEOUT,	m_external_timeout);
	}
	LoadGlobalConfig(str_sec,				KEY_MODEL_NAME,		m_model_name, true);
	LoadGlobalConfig(str_sec,				KEY_CONFIG_FILE,	str_temp, true);

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

	// load flash id, it is optional
	if (m_flash_id) delete [] m_flash_id;
	CString flash_id_fn;
	LoadLocalConfig(SEC_SOURCE,		KEY_FLASHID,		flash_id_fn);
	if ( !flash_id_fn.IsEmpty() )
	{
		m_flash_id = new BYTE[SECTOR_SIZE];
		// load file
		FILE * ffile = NULL;
		_tfopen_s(&ffile, flash_id_fn, _T("rb"));
		if ( NULL == ffile )	THROW_ERROR(ERR_PARAMETER, _T("failure on openning file %s"), flash_id_fn);
		JCSIZE len = fread(m_flash_id, 1, SECTOR_SIZE, ffile);
		fclose(ffile);
		DWORD file_checksum = CheckSum(m_flash_id, len);
		LOG_DEBUG(_T("load bin file %s, length=%d, checksum=0x%08X"), flash_id_fn, len, file_checksum);

		LoadLocalConfig(SEC_SOURCE,		KEY_FLASHID_CHECKSUM,flashid_checksum,	0);
		if (flashid_checksum && (flashid_checksum != file_checksum) )
		{
			LOG_ERROR(_T("checksum failed, input:0x%08X, file:0x%08X"), file_checksum, file_checksum);
			THROW_ERROR(ERR_PARAMETER, _T("%s checksum do not match"), flash_id_fn);
		}
	}
	else
	{
		LOG_WARNING(_T("flash id is not set."));
	}
	//LoadLocalConfig(SEC_SOURCE,		KEY_FLASHID,		m_flash_id,			fid_len,	flashid_checksum);

	LoadLocalConfig(SEC_SOURCE,		KEY_MPISP_CHECKSUM,	mpisp_checksum,		0);
	LoadLocalConfig(SEC_SOURCE,		KEY_MPISP,			m_mpisp,			m_mpisp_len, mpisp_checksum);
	LoadLocalConfig(SEC_SOURCE,		KEY_ISP_CHECKSUM,	m_isp_check_sum);
	LoadLocalConfig(SEC_SOURCE,		KEY_ISP,			m_isp_file_name);

	LoadLocalConfig(SEC_CONFIGURE,	KEY_OEM_MODEL_NAME,	m_oem_model_name);
	LoadLocalConfig(SEC_CONFIGURE,	KEY_SN_PREFIX,		m_sn_prefix);		// serial number prefix, ex. EMBZ1
	LoadLocalConfig(SEC_CONFIGURE,	KEY_CAPACITY,		m_capacity);
	LoadLocalConfig(SEC_CONFIGURE,	KEY_IF_SETTING,		m_sata_speed);
	LoadLocalConfig(SEC_CONFIGURE,	KEY_TRIM,			m_trim_enable,		-2);	// < 0没有设置
	LoadLocalConfig(SEC_CONFIGURE,	KEY_DEVSLP,			m_devslp_enable,	-2);
	LoadLocalConfig(SEC_CONFIGURE,	KEY_DIS_ISP_CHECK,	m_disable_isp_check_version);
	LoadLocalConfig(SEC_CONFIGURE,	KEY_FW_VER,			m_fw_ver);

	LoadLocalConfig(SEC_CONFIGURE,	KEY_CAP_CYLINDER,	m_cap_c);
	LoadLocalConfig(SEC_CONFIGURE,	KEY_CAP_HEADER,		m_cap_h);
	LoadLocalConfig(SEC_CONFIGURE,	KEY_CAP_SECTOR,		m_cap_s);
	LoadLocalConfig(SEC_CONFIGURE,	KEY_UDMA_SETTING,	m_udma_mode,		-2);
	// create direct for isp backup
	if( GetFileAttributes(ISP_BACKUP_PATH) == -1)	CreateDirectory(ISP_BACKUP_PATH, NULL);

	// Load log configuration
	ir = GetPrivateProfileString(SEC_PARAMETER, KEY_DEVICE_NAME, _T(""), str, MAX_STRING_LEN, app->GetRunFolder() + FILE_DEVICE_NAME);
	m_log_file.SetDeviceName(str);
	ir = GetPrivateProfileString(SEC_PARAMETER, KEY_TEST_MACHINE, _T(""), str, MAX_STRING_LEN, app->GetRunFolder() + FILE_TEST_MACHINE_NO);
	m_log_file.SetTestMachine(str);

	if (m_controller.IsEmpty() )
	{
		ir = GetPrivateProfileInt(SEC_PARAMETER, KEY_PATA_SATA, 0, app->GetRunFolder() + FILE_DEVICE_TYPE);
		if (ir ==0)	m_controller = _T("LT2244"); //SM631GXx;		// (SATA) SM2244LT;
		else		m_controller = _T("SM2236");
	}

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


bool CUpdateSnToolDlg::ConvertStringCompare(const CString & src, BYTE * dst, DWORD len, char filler)
{
	JCASSERT(len < MAX_STRING_LEN);
	BYTE temp[MAX_STRING_LEN];
	memset(temp, filler, MAX_STRING_LEN);
	FillConvertString(src, temp, len, filler);
	temp[len] = 0;

	CStringA d((char*)dst, len);
	LOG_DEBUG(_T("exp=\"%S\", val=\"%S\""), temp, d);

	return ( memcmp(temp, dst, len)==0 );
}

void CUpdateSnToolDlg::FillConvertString(const CString & src, BYTE * dst, DWORD len, char filler)
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

void CUpdateSnToolDlg::ReadConvertString(const BYTE * src, CString & dst, DWORD len)
{
	BYTE temp[MAX_STRING_LEN];
	memset(temp, 0, MAX_STRING_LEN);
	// convert byte orcer
	for (DWORD ii = 0; ii < len; ii += 2)	temp[ii] = src[ii+1], temp[ii+1] = src[ii];
	dst.Format(_T("%S"), temp);
	dst.TrimRight();
}

void CUpdateSnToolDlg::RetrieveController(void)
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
