
// CrazyCatDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "CrazyCat.h"
#include "CrazyCatDlg.h"

#include "robot_cat.h"
#include "robot_catcher.h"

#include "stdext.h"

#include <time.h>


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
	, m_player_catcher(FALSE), m_player_cat(FALSE)
	, m_txt_status(_T("setting"))
	, m_show_path(TRUE),	m_show_search(FALSE)
	, m_search_depth(5)
	, m_board(NULL),		m_evaluate(NULL)
	, m_cat_robot(NULL),	m_catcher_robot(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CCrazyCatDlg::~CCrazyCatDlg(void)
{
	delete m_board;
	delete m_evaluate;
	delete m_cat_robot;
	delete m_catcher_robot;
}

void CCrazyCatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHESSBOARD, m_board_ctrl);
	DDX_Text(pDX, IDC_CELL_RADIUS, m_cell_radius);
	DDX_Control(pDX, IDC_TXT_MESSAGE, m_message_wnd);
	DDX_Check(pDX, IDC_PLAYER_BLUE, m_player_catcher);
	DDX_Check(pDX, IDC_PLAYER_CAT, m_player_cat);
	DDX_Text(pDX, IDC_TXT_STATUS, m_txt_status);
	DDX_Check(pDX, IDC_SHOW_PATH, m_show_path);
	DDX_Check(pDX, IDC_SHOW_SEARCH, m_show_search);

	DDX_Text(pDX, IDC_SEARCH_DEPTH, m_search_depth);
	DDV_MinMaxInt(pDX, m_search_depth, 1, 100);
}

BEGIN_MESSAGE_MAP(CCrazyCatDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_EN_CHANGE(IDC_CELL_RADIUS, OnCellRadiusChanged)
	ON_BN_CLICKED(IDC_BT_SEARCH, &CCrazyCatDlg::OnClickedSearch)
	ON_BN_CLICKED(IDC_BT_STOP, &CCrazyCatDlg::OnClickedStop)
	ON_BN_CLICKED(IDC_BT_PLAY, &CCrazyCatDlg::OnClickedPlay)
	ON_BN_CLICKED(IDC_SHOW_PATH, &CCrazyCatDlg::OnClickedShowPath)
	ON_BN_CLICKED(IDC_SHOW_SEARCH, &CCrazyCatDlg::OnClickedShowPath)
	ON_BN_CLICKED(IDC_BT_RESET, &CCrazyCatDlg::OnClickedReset)
	ON_BN_CLICKED(IDC_BT_UNDO, &CCrazyCatDlg::OnClickUndo)
	ON_BN_CLICKED(IDC_BT_RANDOMTABLE, &CCrazyCatDlg::OnClidkMakeRndTab)
	ON_MESSAGE(WM_MSG_CLICKCHESS, OnChessClicked)
	ON_MESSAGE(WM_MSG_ROBOTMOVE, OnRobotMove)
	ON_MESSAGE(WM_MSG_COMPLETEMOVE, OnCompleteMove)
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

	m_board = new CChessBoard;

	m_board_ctrl.SetBoard(m_board);
	//m_board_ctrl.GetClientRect(& m_board_rect);
	//m_board_ctrl.SetListener(static_cast<IMessageListen*>(this));
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

///////////////////////////////////////////////////////////////////////////////
// -- 用户事件响应
afx_msg void CCrazyCatDlg::OnCellRadiusChanged(void)
{
	if (m_init)
	{
		UpdateData();
		m_board_ctrl.SetCellRadius(m_cell_radius);
	}
}

void CCrazyCatDlg::OnClickedSearch()
{
	if (m_evaluate)	delete m_evaluate;
	m_evaluate = new CCrazyCatEvaluator(m_board);
	int ir = m_evaluate->StartSearch();

	if (ir < MAX_SCORE)
	{
		LOG_DEBUG_(0, _T("route found. depth = %d, expanded %d, closed %d"),
			ir, m_evaluate->m_open_qty, m_evaluate->m_closed_qty);
	}
	else
	{
		LOG_DEBUG_(0, _T("no route found. expanded %d, closed %d"),
			m_evaluate->m_open_qty, m_evaluate->m_closed_qty);
	}
	m_board_ctrl.SetPath(m_evaluate);
	m_board_ctrl.Draw(0,0,0);
}

void CCrazyCatDlg::OnClickedPlay()
{
	UpdateData();
	delete m_cat_robot;
	m_cat_robot = NULL;
	if (m_player_cat)	m_cat_robot = new CRobotCat(static_cast<IRefereeListen*>(this) );

	delete m_catcher_robot;
	if (m_player_catcher)	m_catcher_robot = new CRobotCatcher(static_cast<IRefereeListen*>(this) );

	//m_search_depth = search_depth;

	PLAY_STATUS ps = m_board->StartPlay();
	PostMessage(WM_MSG_COMPLETEMOVE, 0, 0);
	//m_board_ctrl.StartPlay(m_player_catcher, m_player_cat, m_search_depth);
}

void CCrazyCatDlg::OnClickedReset()
{
	delete m_board;
	m_board = new CChessBoard;
	delete m_cat_robot;		m_cat_robot = NULL;
	delete m_catcher_robot;	m_catcher_robot = NULL;
	RedrawWindow();
	//m_board_ctrl.Reset();
}

void CCrazyCatDlg::OnClickedShowPath()
{
	UpdateData();
	m_board_ctrl.SetOption(m_show_path, m_show_search);
}

void CCrazyCatDlg::OnClickUndo()
{
	bool br=m_board->Undo();
	/*if (!br && m_listen) */SendTextMsg(_T("Cannot undo!\n"));
	OnClickedSearch();
	m_board_ctrl.Draw(0, 0, 0);
	//m_board_ctrl.Undo();
}

///////////////////////////////////////////////////////////////////////////////

void CCrazyCatDlg::SearchCompleted(MOVEMENT * move)
{
	// AI搜索完博弈树后，回叫此函数。此函书可能在后台线程中运行。!!注意同步!!
	LOG_STACK_TRACE();
	CCrazyCatMovement * mv = reinterpret_cast<CCrazyCatMovement*>(move);
	switch (m_board->Status() )
	{
	case PS_CAT_MOVE:
		if (mv->m_col < 0 && mv->m_row < 0)		m_board->GiveUp(PLAYER_CAT);	// 认输
		else
		{
			bool br = m_board->Move(PLAYER_CAT, mv->m_col, mv->m_row);
			JCASSERT(br);
			OnClickedSearch();
		}
		break;

	case PS_CATCHER_MOVE:
		if (mv->m_col < 0 && mv->m_row < 0)		m_board->GiveUp(PLAYER_CATCHER);	// 认输
		else
		{
			bool br = m_board->Move(PLAYER_CATCHER, mv->m_col, mv->m_row);
			JCASSERT(br);
			TCHAR str[256];
			_stprintf_s(str, _T("searched node: %d\n"), m_catcher_robot->m_node);
			/*if (m_listen) m_listen->*/SendTextMsg(str);
			OnClickedSearch();	// 搜索CAT的逃逸路经
		}
		break;
	}
	PostMessage(WM_MSG_COMPLETEMOVE, 0, 0);
}

LRESULT CCrazyCatDlg::OnChessClicked(WPARAM wp, LPARAM lp)
{
	char col, row;
	col = LOBYTE(wp & 0xFFFF), row = HIBYTE(wp & 0xFFFF);

	PLAY_STATUS status = m_board->Status();

	bool br = false;
	switch (status)
	{
	case PS_SETTING:	{
		BYTE ss = m_board->CheckPosition(col, row);
		if ( 1 == ss ) ss = 0;
		else if ( 0 == ss ) ss = 1;
		else return 0;
		m_board->SetPosition(col, row, ss);
		br = true;
		m_board_ctrl.Draw(0, 0, 0);
		break;				}

	case PS_CATCHER_MOVE:
		if (!m_player_catcher)
		{
			br = m_board->Move(PLAYER_CATCHER, col, row);
			if (br)		
			{
				OnClickedSearch();
				PostMessage(WM_MSG_COMPLETEMOVE, 0, 0);
			}
		}
		break;

	case PS_CAT_MOVE:
		if (!m_player_cat) 
		{	// cat是player
			br = m_board->Move(PLAYER_CAT, col, row);
			if (br)
			{
				OnClickedSearch();
				PostMessage(WM_MSG_COMPLETEMOVE, 0, 0);
			}
		}
		break;
	}
	return 0;
}

LRESULT CCrazyCatDlg::OnCompleteMove(WPARAM wp, LPARAM lp)
{
	// PLAYER或者AI下完棋，更新状态。此函数一定在GUI线程中运行
	LOG_STACK_TRACE();

	JCASSERT(m_board);

	PLAY_STATUS ss = m_board->Status();

	CJCStringT str;
	switch (ss)
	{
	case PS_SETTING:		str = _T("setting");	break;
	case PS_CATCHER_MOVE:	
		str = _T("catcher moving");	
		// 轮到CATCHER下棋，如果是AI，则消息启动AI下棋。否则返回等待PLAYER下棋。
		if (m_catcher_robot) PostMessage(WM_MSG_ROBOTMOVE, PLAYER_CATCHER, 0);
		break;

	case PS_CAT_MOVE:		
		str = _T("cat moving");	
		// 轮到CAT下棋，如果是AI，则消息启动AI下棋。否则返回等待PLAYER下棋。
		if (m_cat_robot)	PostMessage(WM_MSG_ROBOTMOVE, PLAYER_CAT, 0);
		break;
	case PS_CATCHER_WIN:	str = _T("catcher win");	break;
	case PS_CAT_WIN:		str = _T("cat win");	break;
	}
	/*if (m_listen) m_listen->*/SetTextStatus(str);
	m_board_ctrl.Draw(0, 0, 0);
	return 0;
}

LRESULT CCrazyCatDlg::OnRobotMove(WPARAM wp, LPARAM lp)
{
	LOG_STACK_TRACE();
	if (wp == PLAYER_CAT)
	{	// AI CAT下棋
		JCASSERT(m_cat_robot);
		m_cat_robot->StartSearch(m_board, 0);
	}
	else
	{	// AI CATCHER下棋
		JCASSERT(m_catcher_robot);
		m_catcher_robot->StartSearch(m_board, m_search_depth);
	}
	return 0;
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



void CCrazyCatDlg::SetTextStatus(const CJCStringT & txt)
{
	m_txt_status = txt.c_str();
	UpdateData(FALSE);
}


void CCrazyCatDlg::OnClickedStop()
{
	// TODO: 在此添加控件通知?理程序代?
}






void CCrazyCatDlg::OnClidkMakeRndTab()
{
	FILE * file = NULL;
	_tfopen_s(& file, _T("randon_table.txt"), _T("w+t"));
	srand( (unsigned)time( NULL ) );
	for (int ii = 0; ii < 20; ++ ii)
	{
		for (int jj = 0; jj < 20; ++ jj)
		{
#if 1
			UINT r1 = rand(), r2 = rand();
			UINT r3 = rand();
			UINT r = ( (r1 << 15 | r2) << 15 | r3 );
			fprintf_s(file, ("0x%08X,"),r);
#else		
			UINT64 r1 = rand(), r2 = rand();
			UINT64 r3 = rand(), r4 = rand();
			UINT64 r5 = rand();
			UINT64 r = (( (r1 << 15 | r2) << 15 | r3 ) << 15 | r4 ) << 15 |	r5;
			fprintf_s(file, ("0x%016I64X,"),r);
#endif
		}
		fprintf_s(file, ("\n"));
	}
	fclose(file);
}
