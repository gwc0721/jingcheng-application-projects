// UPSNTOOL_SelTestMode.cpp : implementation file
//
#include "stdafx.h"
#include "sm224testB.h"
#include "SM224UpdatedSNToolMPDlg.h"
#include "UPSNTOOL_SelTestMode.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CUpsnSelTestModeDlg dialog


CUpsnSelTestModeDlg::CUpsnSelTestModeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUpsnSelTestModeDlg::IDD, pParent)
{
}


void CUpsnSelTestModeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_model_list);
}


BEGIN_MESSAGE_MAP(CUpsnSelTestModeDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_SNupdateNverify, OnBUTTONSNupdateNverify)
	ON_BN_CLICKED(IDC_BUTTON_SNverifyOnly, OnBUTTONSNverifyOnly)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUpsnSelTestModeDlg message handlers
BOOL CUpsnSelTestModeDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CString str;
	// TODO: Add extra initialization here

	// Show application version
	CUpdateSnToolApp * app = dynamic_cast<CUpdateSnToolApp *>(AfxGetApp());
	ASSERT(app);
	str.Format(_T("SMI. Update Serial Number Tool  Ver. %s"), app->GetVer() );
	this->SetWindowText(str);

	// Read selections from "model_list.txt"
	bool br = LoadModelList();
	if (!br)
	{
		EndDialog(0);
		return FALSE;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CUpsnSelTestModeDlg::OnBUTTONSNupdateNverify() 
{
	// TODO: Add your control notification handler code here
	UpdateData();
	int mode_index = m_model_list.GetCurSel();
	StartUpdate(mode_index, MODE_UPDATE_VERIFY);
}

void CUpsnSelTestModeDlg::OnBUTTONSNverifyOnly() 
{
	// TODO: Add your control notification handler code here
	UpdateData();
	int mode_index = m_model_list.GetCurSel();
	StartUpdate(mode_index, MODE_VERIFY_ONLY);
}

bool CUpsnSelTestModeDlg::StartUpdate(int model_index, TEST_MODE test_mode)
{
	CUpdateSnToolDlg dlg;
	dlg.Start(test_mode, model_index);
	return true;
}


bool CUpsnSelTestModeDlg::LoadModelList(void)
{
	CUpdateSnToolApp * app = dynamic_cast<CUpdateSnToolApp *>(AfxGetApp());
	ASSERT(app);

	CString model_file =  app->GetRunFolder() + FILE_MODEL_LIST;
	CString err_msg;

	UINT models_count = GetPrivateProfileInt(MODLE_LIST_MODELS, KEY_COUNT, 0, model_file); 
	if (models_count == 0) 
	{
		err_msg.Format(_T("Open %s file failed or no model count defined."), model_file);
		AfxMessageBox(err_msg);
		return false;
	}

	m_model_list.ResetContent();
	for (UINT ii=0; ii < models_count; ++ii)
	{
		CString str_sec;
		str_sec.Format(_T("MODEL%02d"), ii + 1);
		TCHAR	str_model_name[MAX_STRING_LEN];
		DWORD ir = 0;
		ir = GetPrivateProfileString(str_sec, KEY_MODEL_NAME, NULL, str_model_name, MAX_STRING_LEN, model_file);
		if (ir == 0)
		{
			err_msg.Format(_T("Model name is not finde in %s."), str_sec);
			AfxMessageBox(err_msg);
			return false;
		}
		m_model_list.AddString(str_model_name);
	}

	m_model_list.SetCurSel(0);
	return true;
}
	

