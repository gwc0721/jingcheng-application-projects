
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
// -- global
void PassAllMessage()
{
	MSG msg;
	while( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
	{
		TranslateMessage( &msg );      /* Translates virtual key codes.  */
		DispatchMessage( &msg );       /* Dispatches message to window.  */
	}
}

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
	, m_first_player(0)
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
	DDX_Control(pDX, IDC_TXT_MESSAGE, m_message_wnd);
	DDX_Check(pDX, IDC_PLAYER_BLUE, m_player_catcher);
	DDX_Check(pDX, IDC_PLAYER_CAT, m_player_cat);
	DDX_Check(pDX, IDC_SHOW_PATH, m_show_path);
	DDX_Check(pDX, IDC_SHOW_SEARCH, m_show_search);

	DDX_Text(pDX, IDC_TXT_STATUS, m_txt_status);
	DDX_Text(pDX, IDC_CELL_RADIUS, m_cell_radius);
	DDX_Text(pDX, IDC_CHESS_LOCATION, m_txt_location);
	DDX_Text(pDX, IDC_CHECKSUM, m_checksum);

	DDX_Text(pDX, IDC_SEARCH_DEPTH, m_search_depth);
	DDV_MinMaxInt(pDX, m_search_depth, 1, 100);
	DDX_Control(pDX, IDC_BT_PLAY, m_btn_play);
	DDX_Control(pDX, IDC_BT_STOP, m_btn_stop);
	DDX_Radio(pDX, IDC_FIRST_CATCHER, m_first_player);
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
	ON_BN_CLICKED(IDC_BT_SAVE, &CCrazyCatDlg::OnClickedSave)
	ON_BN_CLICKED(IDC_BT_LOAD, &CCrazyCatDlg::OnClickedLoad)
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
#ifdef _DEBUG
	SetWindowTextW(_T("CrazyCat (DEBUG)"));
#endif
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
	/*
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
	*/
}

void CCrazyCatDlg::OnClickedPlay()
{
	UpdateData();
	if (m_player_cat)	m_cat_robot = new CRobotCat(static_cast<IRefereeListen*>(this) );

	if (m_player_catcher)	m_catcher_robot = new CRobotCatcher(static_cast<IRefereeListen*>(this) );

	if (0 == m_first_player)	m_board->SetStatus(PS_CATCHER_MOVE);
	else						m_board->SetStatus(PS_CAT_MOVE);
	m_btn_play.EnableWindow(FALSE);
	m_btn_stop.EnableWindow(TRUE);
	PostMessage(WM_MSG_COMPLETEMOVE, 0, 0);
}

void CCrazyCatDlg::OnClickedReset()
{
	delete m_board;
	m_board = new CChessBoard;
	m_board_ctrl.SetBoard(m_board);

	delete m_cat_robot;		m_cat_robot = NULL;
	delete m_catcher_robot;	m_catcher_robot = NULL;
	m_board_ctrl.Draw(0, 0, 0);
	RedrawWindow();
}

void CCrazyCatDlg::OnClickedShowPath()
{
	UpdateData();
	m_board_ctrl.SetOption(m_show_path, m_show_search);
}

void CCrazyCatDlg::OnClickUndo()
{
	bool br=m_board->Undo();
	SendTextMsg(_T("Cannot undo!"));
	OnClickedSearch();
	m_board_ctrl.Draw(0, 0, 0);
}

void CCrazyCatDlg::OnClickedStop()
{	// Change status to setting

	delete m_cat_robot;		m_cat_robot = NULL;
	delete m_catcher_robot;	m_catcher_robot = NULL;

	m_board->SetStatus(PS_SETTING);
	m_btn_play.EnableWindow(TRUE);
	m_btn_stop.EnableWindow(FALSE);
	SetTextStatus(_T("setting"));
}

static const TCHAR FILTER[] = _T("Text Files|*.txt|All Files|*.*||");

void CCrazyCatDlg::OnClickedSave()
{
	CFileDialog dlg(FALSE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, FILTER);
	if ( dlg.DoModal() != IDOK) return;
	CString fn = dlg.GetPathName();
	if (dlg.GetFileExt() == _T(""))	fn += _T(".txt");

	FILE * file = NULL;
	stdext::jc_fopen(&file, fn, _T("w+t"));
	//if (NULL == file) THROW_ERROR(ERR_USER, _T("failure on openning file %s"), fn);
	if (NULL == file)
	{
		LOG_ERROR(_T("failure on openning file %s"), fn);
		return;
	}
	m_board->SaveToFile(file);
	fclose(file);
}

void CCrazyCatDlg::OnClickedLoad()
{
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, FILTER);
	if ( dlg.DoModal() != IDOK) return;

	delete m_board;
	m_board = new CChessBoard;
	m_board_ctrl.SetBoard(m_board);

	CString fn = dlg.GetPathName();
	FILE * file = NULL;
	stdext::jc_fopen(&file, fn, _T("r"));
	//if (NULL == file) THROW_ERROR(ERR_USER, _T("failure on openning file %s"), fn);
	if (NULL == file)
	{
		LOG_ERROR(_T("failure on openning file %s"), fn);
		return;
	}
	m_board->LoadFromFile(file);
	fclose(file);
	m_board_ctrl.Draw(0, 0, 0);
	RedrawWindow();
}

///////////////////////////////////////////////////////////////////////////////

void CCrazyCatDlg::SearchCompleted(MOVEMENT * move)
{
	// AI搜索完博弈树后，回叫此函数。此函书可能在后台线程中运行。!!注意同步!!
	LOG_STACK_TRACE();
	TCHAR str[256];
	CCrazyCatMovement * mv = reinterpret_cast<CCrazyCatMovement*>(move);

	if (mv->m_col < 0 && mv->m_row < 0)
	{	// 认输	
		m_board->GiveUp(mv->m_player);
		_stprintf_s(str, _T("player %d give up"), mv->m_player);
		SendTextMsg(str);
	}
	else
	{	// 下棋
		_stprintf_s(str, _T("%s move to (%d,%d)"), 
			mv->m_player==PLAYER_CAT?_T("cat"):_T("cch"), mv->m_col, mv->m_row);
		SendTextMsg(str);

		bool br = m_board->Move(mv->m_player, mv->m_col, mv->m_row);
		JCASSERT(br);
		if (mv->m_player == PLAYER_CATCHER)
		{
			_stprintf_s(str, _T("node: % 8d, hash hit: % 6d, hash confilict: %d"),
				m_catcher_robot->m_node, m_catcher_robot->m_hash_hit, m_catcher_robot->m_hash_conflict);
			SendTextMsg(str);
		}
	}
	PostMessage(WM_MSG_COMPLETEMOVE, 0, 0);
}

LRESULT CCrazyCatDlg::OnChessClicked(WPARAM wp, LPARAM flag)
{
	char col, row;
	col = LOBYTE(wp & 0xFFFF), row = HIBYTE(wp & 0xFFFF);

	PLAY_STATUS status = m_board->Status();

	switch (flag)
	{
	case CLICKCHESS_MOVE:
		m_txt_location.Format(_T("(%d,%d)"), col, row);
		UpdateData(FALSE);
		break;

	case CLICKCHESS_LEFT:
		switch (status)
		{
		case PS_SETTING:
			m_board->SetChess(PLAYER_CATCHER, col, row);
			m_board_ctrl.Draw(0, 0, 0);
			break;
		case PS_CATCHER_MOVE:
			if ( (!m_player_catcher) )
			{
				if ( m_board->Move(PLAYER_CATCHER, col, row) )
				{
					PostMessage(WM_MSG_COMPLETEMOVE, 0, 0);
				}
			}
			break;

		case PS_CAT_MOVE:
			if ( (!m_player_cat) ) 
			{	// cat是player
				if ( m_board->Move(PLAYER_CAT, col, row) )
				{
					PostMessage(WM_MSG_COMPLETEMOVE, 0, 0);
				}
			}
			break;
		}
		break;

	case CLICKCHESS_RIGHT:
		if ( PS_SETTING == status)
		{
			m_board->SetChess(PLAYER_CAT, col, row);
			m_board_ctrl.Draw(0, 0, 0);
		}
		break;
	}

	m_checksum.Format(_T("%08X"), m_board->MakeHash());
	UpdateData(FALSE);
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
		SetTextStatus(str);
		m_board_ctrl.Draw(0, 0, 0);
		// 轮到CATCHER下棋，如果是AI，则消息启动AI下棋。否则返回等待PLAYER下棋。
		if (m_catcher_robot) PostMessage(WM_MSG_ROBOTMOVE, PLAYER_CATCHER, 0);
		return 0;

	case PS_CAT_MOVE:		
		str = _T("cat moving");	
		SetTextStatus(str);
		m_board_ctrl.Draw(0, 0, 0);
		// 轮到CAT下棋，如果是AI，则消息启动AI下棋。否则返回等待PLAYER下棋。
		if (m_cat_robot)	PostMessage(WM_MSG_ROBOTMOVE, PLAYER_CAT, 0);
		return 0;

	case PS_CATCHER_WIN:
		SendTextMsg(_T("catcher win"));
		//str = _T("catcher win");	
		OnClickedStop();
		break;
	case PS_CAT_WIN:		
		SendTextMsg(_T("cat win"));
		//str = _T("cat win");	
		OnClickedStop();
		break;
	}
	m_checksum.Format(_T("%08X"), m_board->MakeHash());
	SetTextStatus(str);
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

	m_message_wnd.GetSel(start, end);
	m_message_wnd.SetSel(end, end);
	m_message_wnd.ReplaceSel(_T("\n"));

	UpdateData(FALSE);
}



void CCrazyCatDlg::SetTextStatus(const CJCStringT & txt)
{
	m_txt_status = txt.c_str();
	UpdateData(FALSE);
	PassAllMessage();
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

