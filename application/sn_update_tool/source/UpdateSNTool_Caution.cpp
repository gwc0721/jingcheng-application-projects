// UpdateSNTool_Caution.cpp : implementation file
//

#include "stdafx.h"
#include "sm224testB.h"
#include "UpdateSNTool_Caution.h"
#include "UPSNTool_MessageBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUpdateSNTool_Caution dialog
CUpdateSNTool_Caution::CUpdateSNTool_Caution(CWnd* pParent /*=NULL*/)
	: CDialog(CUpdateSNTool_Caution::IDD, pParent)
	, m_caution(CUpsnCaution::CAUTION_NONE)
{
}


void CUpdateSNTool_Caution::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CUpdateSNTool_Caution, CDialog)
	ON_BN_CLICKED(ID_Retry, OnRetry)
	ON_WM_CTLCOLOR()
	ON_WM_PAINT()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUpdateSNTool_Caution message handlers

BOOL CUpdateSNTool_Caution::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	TCHAR String_Caution1[]=_T("S/N is not 16 characters!!");
	TCHAR String_Caution2[]=_T("S/N is not matching to actual capacity!!");

	switch (m_caution)
	{
	case CUpsnCaution::CAUTION_SN_LENGTH:
		(GetDlgItem(IDC_STATIC_Caution))->SetWindowText(String_Caution1);
		break;

	case CUpsnCaution::CAUTION_SN_CAPACITY:
		(GetDlgItem(IDC_STATIC_Caution))->SetWindowText(String_Caution2);
		break;
	}
	Beep(4500,1000);//Fail
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CUpdateSNTool_Caution::OnRetry() 
{
	// TODO: Add your control notification handler code here
	EndDialog(CAUTION_RETRY);
}

void CUpdateSNTool_Caution::OnCancel() 
{
	// TODO: Add extra cleanup here
	EndDialog(CAUTION_CANCEL);
}

HBRUSH CUpdateSNTool_Caution::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	
	// TODO: Change any attributes of the DC here
	if(nCtlColor == CTLCOLOR_STATIC)
	{
		switch(pWnd->GetDlgCtrlID())
		{
			case IDC_STATIC_caution1:
				pDC->SetTextColor(RGB(255 , 0 , 0));
				break;
			case IDC_STATIC_Caution: 
				pDC->SetTextColor(RGB(255 , 0 , 0));
			    break;
		}

	}
	// TODO: Return a different brush if the default is not desired
	return hbr;
}

void CUpdateSNTool_Caution::OnPaint() 
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
