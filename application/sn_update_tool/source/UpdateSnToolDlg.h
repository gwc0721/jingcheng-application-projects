#pragma once
// UpdateSnToolDlg.h : header file
#include "LogFile.h"
#include "UpsnTestModeDlg.h"
#include <FerriSDK.h>

//model name length
class CUpsnError	: public CUserException
{
public:
	CUpsnError(int error_code)
		: m_message_option(error_code)
	{}

	int GetErrorCode(void) const {return m_message_option;}

protected:
	int m_message_option;
};

class CUpsnWarning	: public CUserException
{
public:
	CUpsnWarning()
		: m_message_option(0)
	{}

	int GetErrorCode(void) const {return m_message_option;}

protected:
	int m_message_option;
};

#define COLOR_BLUE		RGB(0,0,255)
#define COLOR_RED		RGB(255,0,0)


/////////////////////////////////////////////////////////////////////////////
// CUpdateSnToolDlg dialog

class CUpdateSnToolDlg : public CDialog
{
// Construction
public:
	CUpdateSnToolDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	enum { IDD = IDD_DIALOG_UPSN_MAINPAGE };

	void ReShowMainPage();
	void UpsnSetCurson();
	void SetStatus(COLORREF cr, LPCTSTR status);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
// Overrides
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
// Implementation
protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnStartUpdatedSN();
	afx_msg void OnQuit();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnEditInputSn();
	afx_msg void OnButtonSetSn();
	afx_msg void OnButtonClearCnt();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	DECLARE_MESSAGE_MAP()

public:
	INT_PTR Start(TEST_MODE test_mode, int model_index)
	{
		m_test_mode = test_mode, m_model_index = model_index;
		return DoModal();
	}

	// device operations
protected:
	// return actual isp len
	int UpsnVerifySn(IFerriDevice * dev, bool verify_sn);
	int UpsnUpdateToDevice(IFerriDevice * dev, bool verify_sn);
	bool UpsnLoadIspFile(IFerriDevice * dev, IIspBuffer * isp_buf);
	bool UpsnUpdateIspFerri(IFerriDevice * dev, IIspBuffer * isp_buf);
	bool UpsnUpdateIspFromConfig(IIspBuffer * isp_buf);
	// read isp and verify, save isp to ISP_R
	bool UpsnReadIspFerri(JCSIZE data_len, IFerriDevice * dev, IIspBuffer * isp_ref);
	DWORD GetRunTimeBad(IFerriDevice * dev);

protected:
	DWORD CheckSum(BYTE * buf, DWORD len);
	bool LoadConfig(void);
	bool LoadGlobalConfig(LPCTSTR sec, LPCTSTR key, CString & val, bool mandatory = false);
	// def_val：缺省值。如果dev_val==-1，则这个项目是必须的
	bool LoadGlobalConfig(LPCTSTR sec, LPCTSTR key, int & val, int def_val = 0);
	bool LoadLocalConfig(LPCTSTR sec, LPCTSTR key, CString & val, bool mandatory = false);
	bool LoadLocalConfig(LPCTSTR sec, LPCTSTR key, BYTE * buf, DWORD &len, int checksum = 0);
	bool LoadLocalConfig(LPCTSTR sec, LPCTSTR key, int &val, int def_val = 0);

	void UpsnUpdateCnt(int new_count);

	bool ConvertStringCompare(const CString & src, BYTE * dst, DWORD len, char filler = 0x20);
	void FillConvertString(const CString & src, BYTE * dst, DWORD len, char filler = 0x20);
	void ReadConvertString(const BYTE * src, CString & dst, DWORD len);

	void RetrieveController(void);

	void ClearInputSn(void)
	{
		ASSERT(m_edit_input_sn);
		m_edit_input_sn->SetSel(0, -1);
		m_edit_input_sn->Clear();
		m_edit_input_sn->SetFocus();
	}

	DWORD ExcuseExternalProcess(const DEVICE_INFO & info);

protected:
	CListCtrl	m_ListUpdatedSNTool;
	CLogFile	m_log_file;

protected:
	// SDK
	CString			m_controller;
	IFerriFactory	* m_ferri_factory;

	TEST_MODE	m_test_mode;
	int			m_model_index;
	CString		m_model_name;
	CString		m_config_file;
	CString		m_oem_model_name;
	CString		m_isp_file_name;
	CString		m_fw_ver;

	// external test
	CString		m_external_cmd;
	CString		m_external_param;
	CString		m_external_path;
	int			m_external_show_wnd;
	int			m_external_timeout;

	CString		m_global_prefix;
	CString		m_sn_prefix;

	// serial number from user input
	CString		m_input_sn;
	bool		m_is_set_sn;

	int			m_capacity;		// in sectors
	int			m_sata_speed;	// 0：未设置，1：FORCE GEN1, 2:GEN1, GEN2
	int			m_trim_enable;	// <0 没有设置，0: disabel, 1: enable
	int			m_devslp_enable;// <0 没有设置，0: disabel, 1: enable
	int			m_disable_isp_check_version;
	int			m_isp_check_sum;
	// CHS 设置，0：未设置
	int			m_cap_c, m_cap_h, m_cap_s;
	// UDMA setting, ，0：未设置
	int			m_udma_mode;

	// current connect count
	int			m_count_current;
	int			m_count_limit;

protected:
	BYTE		m_vendor_specific[VENDOR_LENGTH + 1];
	BYTE		* m_flash_id;
	BYTE		m_mpisp[MPISP_LENGTH];
	DWORD		m_mpisp_len;

	// point of controllers
protected:
	CButton *	m_btn_start;
	CButton *	m_btn_scan_drive;
	CButton *	m_btn_quit;
	CButton *	m_btn_set_sn;
	CEdit	*	m_edit_input_sn;
	UINT_PTR	m_scan_timer;

// controls variable
public:
	CString m_str_test_model_name;
	CString m_str_count_current;
	CString m_str_count_limit;
	CString m_str_fw_rev;
};

