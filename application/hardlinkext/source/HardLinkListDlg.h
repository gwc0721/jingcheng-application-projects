// HardLinkListDlg.h : CHardLinkListDlg 的声明

#pragma once

#include "resource.h"       // 主符号

#include <atlhost.h>


// CHardLinkListDlg

class CHardLinkListDlg : 
	public CAxDialogImpl<CHardLinkListDlg>
{
public:
	CHardLinkListDlg()
	{
	}

	~CHardLinkListDlg()
	{
	}

	enum { IDD = IDD_HARDLINKLISTDLG };

BEGIN_MSG_MAP(CHardLinkListDlg)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
	COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
	CHAIN_MSG_MAP(CAxDialogImpl<CHardLinkListDlg>)
END_MSG_MAP()

// 处理程序原型:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		CAxDialogImpl<CHardLinkListDlg>::OnInitDialog(uMsg, wParam, lParam, bHandled);
		bHandled = TRUE;
		return 1;  // 使系统设置焦点
	}

	LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		EndDialog(wID);
		return 0;
	}

	LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		EndDialog(wID);
		return 0;
	}
};


