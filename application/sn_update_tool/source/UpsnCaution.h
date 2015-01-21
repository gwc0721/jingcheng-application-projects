#pragma once
// UpdateSNTool_Caution.h : header file
//
#include "../res/resource.h"

class CUpsnCaution	: public CUserException
{
public:
	enum CAUTION
	{
		CAUTION_NONE,		//0
		CAUTION_SN_LENGTH,		//1
		CAUTION_SN_CAPACITY,	//2
	};
	CUpsnCaution(CAUTION error_code)
		: m_message_option( error_code)
	{}

	CAUTION GetErrorCode(void) const {return m_message_option;}

protected:
	CAUTION m_message_option;
};

/////////////////////////////////////////////////////////////////////////////
// CUpsnCautionDlg dialog

class CUpsnCautionDlg : public CDialog
{
// Construction
public:
	CUpsnCautionDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	enum { IDD = IDD_DIALOG_UpdateSNTool_caution };
	enum RETURN
	{
		CAUTION_RETRY,
		CAUTION_CANCEL,
	};

public:
	INT_PTR DoModal(CUpsnCaution::CAUTION msg_id)
	{
		m_caution = msg_id;
		return CDialog::DoModal();
	}

// Overrides
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnRetry();
	virtual void OnCancel();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()

public:
	CUpsnCaution::CAUTION	m_caution;
};
