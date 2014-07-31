
// CrazyCatDlg.h : 头文件
//

#pragma once
#include "afxwin.h"

#include "ChessBoard.h"

// CCrazyCatDlg 对话框
class CCrazyCatDlg : public CDialog
	, public IMessageListen
{
// 构造
public:
	CCrazyCatDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_CRAZYCAT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	afx_msg void OnCellRadiusChanged(void);
	afx_msg void OnClickedPlay();
	afx_msg void OnBnClickedBtSearch();
	afx_msg void OnStnClickedChessboard();
	afx_msg void OnEnChangeTxtMessage();
	afx_msg void OnClickedShowPath();

	CRect m_board_rect;

public:
	CChessBoardUi m_chessboard;
	int	m_cell_radius;
	bool m_init;

	CEdit m_message_wnd;

protected:
	virtual void SendTextMsg(const CJCStringT & txt);
	virtual void SetTextStatus(const CJCStringT & txt);
public:
	afx_msg void OnBnClickedStop();
	BOOL m_player_blue;
	BOOL m_player_cat;
	CString m_txt_status;
	BOOL m_show_path;
	BOOL m_show_search;
	afx_msg void OnClickedReset();
};
