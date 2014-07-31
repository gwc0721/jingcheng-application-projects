
// CrazyCatDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "CrazyCat.h"
#include "CrazyCatDlg.h"

#include "stdext.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

LOCAL_LOGGER_ENABLE(_T("chess_board"), LOGGER_LEVEL_DEBUGINFO);

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


///////////////////////////////////////////////////////////////////////////////
// -- CCrazyCatDlg 对话框
CCrazyCatDlg::CCrazyCatDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCrazyCatDlg::IDD, pParent)
	, m_cell_radius(15)
	, m_init(false)
	, m_player_blue(FALSE)
	, m_player_cat(FALSE)
	, m_txt_status(_T("setting"))
	, m_show_path(FALSE)
	, m_show_search(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCrazyCatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHESSBOARD, m_chessboard);
	DDX_Text(pDX, IDC_CELL_RADIUS, m_cell_radius);
	DDX_Control(pDX, IDC_TXT_MESSAGE, m_message_wnd);
	DDX_Check(pDX, IDC_PLAYER_BLUE, m_player_blue);
	DDX_Check(pDX, IDC_PLAYER_CAT, m_player_cat);
	DDX_Text(pDX, IDC_TXT_STATUS, m_txt_status);
	DDX_Check(pDX, IDC_SHOW_PATH, m_show_path);
	DDX_Check(pDX, IDC_SHOW_SEARCH, m_show_search);
}

BEGIN_MESSAGE_MAP(CCrazyCatDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
//	ON_WM_MOUSEMOVE()
	ON_EN_CHANGE(IDC_CELL_RADIUS, OnCellRadiusChanged)
//	ON_WM_LBUTTONUP()
	ON_BN_CLICKED(IDC_BT_SEARCH, &CCrazyCatDlg::OnBnClickedBtSearch)
	ON_STN_CLICKED(IDC_CHESSBOARD, &CCrazyCatDlg::OnStnClickedChessboard)
	ON_EN_CHANGE(IDC_TXT_MESSAGE, &CCrazyCatDlg::OnEnChangeTxtMessage)
	ON_BN_CLICKED(IDC_BT_STOP, &CCrazyCatDlg::OnBnClickedStop)
	ON_BN_CLICKED(IDC_BT_PLAY, &CCrazyCatDlg::OnClickedPlay)
	ON_BN_CLICKED(IDC_SHOW_PATH, &CCrazyCatDlg::OnClickedShowPath)
	ON_BN_CLICKED(IDC_SHOW_SEARCH, &CCrazyCatDlg::OnClickedShowPath)
	ON_BN_CLICKED(IDC_BT_RESET, &CCrazyCatDlg::OnClickedReset)
END_MESSAGE_MAP()


// CCrazyCatDlg 消息处理程序

BOOL CCrazyCatDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	m_init = true;

	m_chessboard.SetBoard(new CChessBoard);
	m_chessboard.GetClientRect(& m_board_rect);
	m_chessboard.SetListener(static_cast<IMessageListen*>(this));
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CCrazyCatDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CCrazyCatDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CCrazyCatDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


afx_msg void CCrazyCatDlg::OnCellRadiusChanged(void)
{
	if (m_init)
	{
		UpdateData();
		m_chessboard.SetCellRadius(m_cell_radius);
		//m_chessboard.m_cell_radius = m_cell_radius;
		//m_chessboard.RedrawWindow();
	}
}

//void CCrazyCatDlg::OnMouseMove(UINT flag, CPoint point)
//{
//	//LOG_STACK_TRACE();
//	LOG_DEBUG_(1, _T("(%d, %d)"), point.x, point.y);
//	ClientToScreen(&point);
//	m_chessboard.ScreenToClient(&point);
//	////if (m_board_rect.PtInRect(point) )	m_chessboard.OnMouseMove(flag, point);
//	LOG_DEBUG_(1, _T("-(%d, %d)"), point.x, point.y);
//}

//BOOL CCrazyCatDlg::PreTranslateMessage(MSG* pMsg)
//{
//	// TODO: 在此添加?用代?和/或?用基?
//	//BOOL br = m_chessboard.PreTranslateMessage(pMsg);
//	//if (br) return br;
//	return CDialog::PreTranslateMessage(pMsg);
//}

//void CCrazyCatDlg::OnLButtonUp(UINT flag, CPoint point)
//{
//	// TODO: 在此添加消息?理程序代?和/或?用默??
//	ClientToScreen(&point);
//	m_chessboard.ScreenToClient(&point);
//	//if (m_board_rect.PtInRect(point) )	m_chessboard.OnLButtonUp(flag, point);
//
//	CDialog::OnLButtonUp(flag, point);
//}

void CCrazyCatDlg::OnBnClickedBtSearch()
{
	// TODO: 在此添加控件通知?理程序代?
	m_chessboard.StartSearch();
}

void CCrazyCatDlg::OnStnClickedChessboard()
{
	// TODO: 在此添加控件通知?理程序代?
}

void CCrazyCatDlg::OnEnChangeTxtMessage()
{
	// TODO:  如果?控件是 RICHEDIT 控件，它将不
	// ?送此通知，除非重写 CDialog::OnInitDialog()
	// 函数并?用 CRichEditCtrl().SetEventMask()，
	// 同?将 ENM_CHANGE ?志“或”?算到掩?中。

	// TODO:  在此添加控件通知?理程序代?
}

void CCrazyCatDlg::SendTextMsg(const CJCStringT & txt)
{
	int start, end;
	m_message_wnd.SetSel(0, -1);
	m_message_wnd.GetSel(start, end);
	m_message_wnd.SetSel(end, end);

	m_message_wnd.ReplaceSel(txt.c_str());
	UpdateData(FALSE);
}

void CCrazyCatDlg::OnBnClickedStop()
{
	// TODO: 在此添加控件通知?理程序代?
}

void CCrazyCatDlg::SetTextStatus(const CJCStringT & txt)
{
	m_txt_status = txt.c_str();
	UpdateData(FALSE);
}

void CCrazyCatDlg::OnClickedPlay()
{
	// TODO: 在此添加控件通知?理程序代?
	UpdateData();
	m_chessboard.StartPlay(m_player_blue, m_player_cat);
}

void CCrazyCatDlg::OnClickedShowPath()
{
	UpdateData();
	m_chessboard.SetOption(m_show_path, m_show_search);
}

//void CCrazyCatDlg::OnClickedShowSearch()
//{
//	// TODO: 在此添加控件通知?理程序代?
//}

void CCrazyCatDlg::OnClickedReset()
{
	// TODO: 在此添加控件通知?理程序代?
	m_chessboard.Reset();
}
