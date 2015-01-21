#pragma once
// UpsnTestModeDlg.h : header file

#include "../res/resource.h"

enum TEST_MODE
{
	MODE_UPDATE_VERIFY, MODE_VERIFY_ONLY,
};
/////////////////////////////////////////////////////////////////////////////
// CUpsnSelTestModeDlg dialog

class CUpsnSelTestModeDlg : public CDialog
{
// Construction
public:
	CUpsnSelTestModeDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	enum { IDD = IDD_DIALOG_UPSN_SelectTestMode };

// Overrides
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:

	// Generated message map functions
	afx_msg void OnBUTTONSNupdateNverify();
	afx_msg void OnBUTTONSNverifyOnly();
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()

protected:
	bool LoadModelList(void);
	bool StartUpdate(int model_index, TEST_MODE test_mode);

public:
	CComboBox m_model_list;
};

