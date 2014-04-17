// UPSNTool_MessageBox.cpp : implementation file
//

#include "stdafx.h"
#include "sm224testB.h"
#include "UPSNTool_MessageBox.h"
#include "UpdateSNTool_Caution.h"
#include "smidisk.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const MSG_DEFINE UPSNTool_MessageBox::m_msg_define[] = {
	// msg code,			pass/fail,	beep,			message
	{UPSN_PASS,				true,		MB_ICONWARNING,	_T("Update S/N Complete !")},
	{UPSN_SCANDRIVE_FAIL,	false,		(UINT)(-1),		_T("Scan Drive Fail!")},
	{UPSN_Reset_FAIL,		false,		(UINT)(-1),		_T("Reset Device Fail!")},
	{UPSN_VERIFYSN_PASS,	true,		MB_ICONWARNING,	_T("Verify S/N Complete!")},
	{UPSN_VERIFYSN_FAIL,	false,		(UINT)(-1),		_T("Verify S/N Fail!")},
	{UPSN_DOWNLOADSN_FAIL,	false,		(UINT)(-1),		_T("Download S/N Fail!")},
	{UPSN_ISPVersionNoMatch_FAIL, false, (UINT)(-1),	_T("Compare ISP version Not Match!") },
	{UPSN_CheckSumNoMatch_FAIL, false,	(UINT)(-1),		_T("Compare CheckSum Not Match!")},
	{UPSN_OPENDRIVE_FAIL,	false,		(UINT)(-1),		_T("Open Drive Fail!")},
	{UPSN_SCANDRIVE_TimeOut_FAIL, false, (UINT)(-1),	_T("Scan Drive exceed 30 Seconds!") },
	{UPSN_OPENFILE_FAIL,	false,		(UINT)(-1),		_T("Open File Fail!")},
	{UPSN_WRITEFILE_FAIL,	false,		(UINT)(-1),		_T("Write File Fail!")},
	{UPSN_CheckModelName_FAIL, false,	(UINT)(-1),		_T("Compare ModelName Not Match!")},
	{UPSN_VERIFYvendorspecfic_FAIL, false, (UINT)(-1),	_T("Compare vendorspecfic Not Match!")},
	{UPSN_CheckCapacity_FAIL, false,	(UINT)(-1),		_T("Compare Capacity Not Match!")},
	{UPSN_VERIFY_IF_FAIL,	false,		(UINT)(-1),		_T("Compare I/F Setting Not Match!")},
	{UPSN_VERIFY_TRIM_FAIL, false,		(UINT)(-1),		_T("Compare TRIM Not Match!")},
	{UPSN_VERIFY_DEVSLP_FAIL, false,	(UINT)(-1),		_T("Compare DEVSLP Not Match!")},
	{UPSN_WriteISP_Compare_FAIL, false, (UINT)(-1),		_T("Write ISP and Read ISP to Compare Not Match!")},
	{UPSN_ReadISP_FAIL,		false,		(UINT)(-1),		_T("Read ISP FAIL!")},
	{UPSN_VERIFY_FW_VERSION, false,		(UINT)(-1),		_T("Compare f/w version not match!")},
	{UPSN_EXTER_TIMEOUT,	false,		(UINT)(-1),		_T("External test timeout.")},
	{UPSN_MAX_EXTER_ERR,	false,		(UINT)(-1),		_T("External test returned an error.")},
	{UPSN_UNKNOW_FAIL,		false,		(UINT)(-1),		_T("Unknow Fail !!")},		// last one must be unknow message
};

const DWORD UPSNTool_MessageBox::m_msg_count = sizeof(UPSNTool_MessageBox::m_msg_define) / sizeof(MSG_DEFINE);


/////////////////////////////////////////////////////////////////////////////
// UPSNTool_MessageBox dialog


UPSNTool_MessageBox::UPSNTool_MessageBox(CWnd* pParent /*=NULL*/)
	: CDialog(UPSNTool_MessageBox::IDD, pParent)
	, m_msg_pass_fail(_T(""))
	, m_msg_content(_T(""))
	, m_msg_error_code(_T(""))
{
}


void UPSNTool_MessageBox::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_STATIC_MEG1, m_msg_pass_fail);
	DDX_Text(pDX, IDC_STATIC_MEG2, m_msg_content);
	DDX_Text(pDX, IDC_STATIC_MEG3, m_msg_error_code);
}


BEGIN_MESSAGE_MAP(UPSNTool_MessageBox, CDialog)
	ON_BN_CLICKED(ID_UPSN_MSGBOX_OK, OnUpsnMsgboxOk)
	ON_WM_CTLCOLOR()
	ON_WM_PAINT()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// UPSNTool_MessageBox message handlers

BOOL UPSNTool_MessageBox::OnInitDialog() 
{
	ASSERT(m_device_info);
	CDialog::OnInitDialog();
	// TODO: Add extra initialization here

	CSM224testBApp * app = dynamic_cast<CSM224testBApp *>(AfxGetApp());
	ASSERT(app);
	m_strToolVer = app->GetVer();

	// find error defination
	const MSG_DEFINE	*msg = NULL;
	DWORD err_index = 0;
	int msg_code = m_device_info->m_error_code;
	if ( (UPSN_MIN_EXTER_ERR <= msg_code) && (msg_code < UPSN_MAX_EXTER_ERR) ) msg_code = UPSN_MAX_EXTER_ERR;
	for (; err_index < (m_msg_count-1); ++err_index)
	{
		if (msg_code == m_msg_define[err_index].m_err_code) break;
	}
	//if (err_index >= m_msg_count) err_index = 
	ASSERT(err_index < m_msg_count);
	msg = m_msg_define + err_index;

	if (msg->m_pass_fail)
	{
		m_failed = false;
		//m_msg_pass_fail = _T("");
		m_msg_error_code.Format(_T("(FW Version is \"%s\")"), m_device_info->m_fw_version);
	}
	else
	{
		m_failed = true;
		m_msg_pass_fail = _T("Fail !!");
		m_msg_error_code.Format(_T("Error Code(0x%X)"), m_device_info->m_error_code);
	}

	m_msg_content = msg->m_err_msg;
	MessageBeep(msg->m_beep);
	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void UPSNTool_MessageBox::OnUpsnMsgboxOk() 
{
	CDialog::OnOK();
}

HBRUSH UPSNTool_MessageBox::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	
	// TODO: Change any attributes of the DC here
	if(nCtlColor == CTLCOLOR_STATIC)
	{
		switch(pWnd->GetDlgCtrlID())
		{
		case IDC_STATIC_MEG3:
		case IDC_STATIC_MEG2:
		case IDC_STATIC_MEG1:
			if (m_failed)	pDC->SetTextColor(RGB(255 , 0 , 0));		// read
			else			pDC->SetTextColor(RGB(0 , 0 , 255));		// blue
			break;	
		}
	}
	// TODO: Return a different brush if the default is not desired
	return hbr;
}

void UPSNTool_MessageBox::OnPaint() //L0130 Lance add for NECi request
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	int i=0;
	CWnd* pParent;
	CRect aRect,bRect;
	pParent = this->GetParent();
	pParent->GetWindowRect(&aRect);
	this->GetWindowRect(&bRect);
	this->MoveWindow(bRect.left,aRect.bottom,bRect.Width(),bRect.Height());
	i=1;
	// Do not call CDialog::OnPaint() for painting messages
}
