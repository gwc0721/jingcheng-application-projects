// HardLinkListDlg.h : CHardLinkListDlg 的声明

#pragma once

#include "../resource.h"       // 主符号

#include <atlhost.h>
#include <Commctrl.h>


// CHardLinkListDlg

class CHardLinkListDlg : 
	public CAxDialogImpl<CHardLinkListDlg>
{
public:
	CHardLinkListDlg();

	~CHardLinkListDlg();

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
		HWND hwnd = GetDlgItem(IDC_HARDLINK_LIST);
		if (hwnd)
		{
			LVCOLUMN col;
			memset(&col, 0, sizeof(col));
			col.mask = LVCF_TEXT | LVCF_WIDTH;
			col.cx = 500;
			col.pszText = _T("file name");

			ListView_InsertColumn(hwnd,0, &col );
		}

		return 1;  // 使系统设置焦点
	}

	LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		DestroyWindow();
		//EndDialog(wID);
		return 0;
	}

	void InsertItem(LPCTSTR str)
	{
		HWND hwnd = GetDlgItem(IDC_HARDLINK_LIST);
		if (hwnd)
		{
			LVITEM item;
			memset(&item, 0, sizeof(item));

			item.mask = LVIF_TEXT;
			//item.pszText = new TCHAR[256];
			//_tcscpy_s(item.pszText, 256, str);
			item.pszText = (LPTSTR) str;

			item.iSubItem = 0;
			item.iItem = 0;

			ListView_InsertItem(hwnd, &item);
		}

	}
};


