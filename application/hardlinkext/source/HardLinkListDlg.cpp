// HardLinkListDlg.cpp : CHardLinkListDlg 的实现

#include "stdafx.h"
#include "HardLinkListDlg.h"

#include <stdext.h>
LOCAL_LOGGER_ENABLE(_T("hlchk.dlg"), LOGGER_LEVEL_ERROR);


// CHardLinkListDlg
CHardLinkListDlg::CHardLinkListDlg()
{
	LOG_DEBUG(_T("constructed 0x%08X"), (UINT)(this) );
}	

CHardLinkListDlg::~CHardLinkListDlg()
{
	LOG_DEBUG(_T("destructed 0x%08X"), (UINT)(this) );
}

LRESULT CHardLinkListDlg::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	DestroyWindow();
	return 0;
}