#pragma once
// UpsnMessageDlg.h : header file

#include "../res/resource.h"
#include "LogFile.h"

//=== Error Code Definition ===// ===>>>
#define UPSN_
#define UPSN_PASS             			0xA0
#define UPSN_SCANDRIVE_FAIL   			0xA1
#define UPSN_DOWNLOADSN_FAIL  			0xA2
#define UPSN_OPENDRIVE_FAIL   			0xA3
#define UPSN_SCANDRIVE_TimeOut_FAIL 	0xA4
#define UPSN_Reset_FAIL              	0xA5//K1018 Lance add
#define UPSN_VERIFYSN_FAIL             	0xA6//K1018 Lance add
#define UPSN_VERIFYSN_PASS             	0xA7//K1019 Lance add
#define UPSN_VERIFYvendorspecfic_FAIL	0xA8
#define UPSN_CheckCapacity_FAIL        	0xA9//L0607 Lance add
#define UPSN_CheckModelName_FAIL       	0xAA//L0607 Lance add
#define UPSN_CheckSumNoMatch_FAIL		0xAB
#define UPSN_ISPVersionNoMatch_FAIL		0xAC
#define UPSN_OPENFILE_FAIL     			0xAD
#define UPSN_WRITEFILE_FAIL     		0xAE
#define UPSN_UNKNOW_FAIL      			0xAF

#define UPSN_VERIFY_IF_FAIL             0xA00
#define UPSN_VERIFY_TRIM_FAIL           0xA01
#define UPSN_VERIFY_DEVSLP_FAIL         0xA02
#define UPSN_WriteISP_Compare_FAIL      0xA03
#define UPSN_ReadISP_FAIL               0xA04
#define UPSN_VERIFY_FW_VERSION			0xA05
#define UPSN_EXTER_TIMEOUT				0xA06
#define UPSN_VERIFY_CHS					0xA07
#define UPSN_VERIFY_UDMA				0xA08

#define UPSN_MIN_EXTER_ERR				1
#define UPSN_MAX_EXTER_ERR				100

struct MSG_DEFINE
{
	int		m_err_code;
	bool	m_pass_fail;
	UINT	m_beep;
	LPCTSTR	m_err_msg;
};
/////////////////////////////////////////////////////////////////////////////
// CUpsnMessageDlg dialog

class CUpsnMessageDlg : public CDialog
{
// Construction
public:
	CUpsnMessageDlg(CWnd* pParent = NULL);   // standard constructor
// Dialog Data
	enum { IDD = IDD_DIALOG_UPSN_MessageBox };

// Overrides
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

public:
	INT_PTR DoModal(const DEVICE_INFO * info)
	{
		//m_message_option = msg_id;
		m_device_info = info;
		return CDialog::DoModal();
	}

// Implementation
protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnUpsnMsgboxOk();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()

protected:
	const DEVICE_INFO * m_device_info;
	//int	m_message_option;
	CString m_strToolVer;
	static const MSG_DEFINE	m_msg_define[];
	static const DWORD			m_msg_count;
	bool	m_failed;

public:
	CString m_msg_pass_fail;
	CString m_msg_content;
	CString m_msg_error_code;
};


