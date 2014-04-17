#pragma once
// SM224UpdatedSNToolMPDlg.h : header file

#include "LogFile.h"
#include "UPSNTOOL_SelTestMode.h"

//model name length
#define NVDISK2_MODEL_LENGTH    15
#define SM631GX_MODEL_LENGTH    8

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
// CSM224UpdatedSNToolMPDlg dialog

class CSM224UpdatedSNToolMPDlg : public CDialog
{
// Construction
public:
	CSM224UpdatedSNToolMPDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data

	enum { IDD = IDD_DIALOG_UPSN_MAINPAGE };

	//void ClearList_1(int i);
	// return error code

	void ReShowMainPage_UPSN();
	void SetCursor_UPSN();
	//void List_Show();
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
	afx_msg void OnBUTTONScanDrive();
	afx_msg void OnQuit();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnChangeEDITInputSN();
	afx_msg void OnBUTTONSetSN();
	afx_msg void OnBUTTONClearCnt();
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
	int VerifySN_UPSNTool(HANDLE hDisk, bool verify_sn);
	int UpdatedSNtoDevice(HANDLE device, bool verify_sn);
	DWORD UpsnLoadIspFile(HANDLE device, BYTE * isp_buf, DWORD isp_len);
	bool UPSN_UpdateISP2244LT(HANDLE device, BYTE * isp_buf, DWORD isp_len);
	bool UPSN_UpdateISPfromConfig(BYTE *ISPBuf, DWORD len);
	// read isp and verify, save isp to ISP_R
	bool UPSN_ReadISP2244LT(HANDLE device, const BYTE * isp_buf, DWORD isp_len);
	bool Reset_UPSNTool(HANDLE hDisk);
	bool CheckFlashID(HANDLE device);
	DWORD GetRunTimeBad(HANDLE device);

protected:
	bool LoadConfig(void);
	bool LoadGlobalConfig(LPCTSTR sec, LPCTSTR key, CString & val);
	bool LoadGlobalConfig(LPCTSTR sec, LPCTSTR key, int & val, int def_val = 0);
	bool LoadLocalConfig(LPCTSTR sec, LPCTSTR key, CString & val);
	bool LoadLocalConfig(LPCTSTR sec, LPCTSTR key, BYTE * buf, DWORD &len);
	bool LoadLocalConfig(LPCTSTR sec, LPCTSTR key, int &val);

	void UpdateCnt_UPSNTool(int new_count);

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

	bool ScanDrive(char & drive_letter, CString & drive_name, HANDLE & device);

	DWORD ExcuseExternalProcess(const DEVICE_INFO & info);


protected:
	CListCtrl	m_ListUpdatedSNTool;
	CLogFile	m_log_file;

protected:
	TEST_MODE	m_test_mode;
	int			m_model_index;
	int			m_controller_ver;
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
	int			m_sata_speed;
	int			m_trim_enable;
	int			m_devslp_enable;
	int			m_disable_isp_check_version;
	int			m_isp_check_sum;

	// current connect count
	int			m_count_current;
	int			m_count_limit;

protected:
	BYTE		m_vendor_specific[VENDOR_LENGTH + 1];
	BYTE		m_flash_id[SECTOR_SIZE];
	BYTE		m_mpisp[MPISP_LENGTH];
	DWORD		m_mpisp_len;

	// device property
protected:
	SMI_TESTER_TYPE	m_tester;

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
	//afx_msg void OnStnClickedStaticTestmodule2();
	CString m_str_fw_rev;
//	afx_msg void OnStnClickedTestmodule();
//	afx_msg void M();
};

