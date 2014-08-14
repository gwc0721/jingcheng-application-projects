
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

LOCAL_LOGGER_ENABLE(_T("dialog"), LOGGER_LEVEL_NOTICE);

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
	, m_cell_radius(CHESS_RADIUS)	, m_cell_size(CHESS_RADIUS)
	, m_init(false)
	, m_show_path(TRUE),	m_show_search(FALSE)
	, m_search_depth(5)
	, m_board(NULL)
	, m_turn(PLAYER_CATCHER)
	, m_status(PS_SETTING)
	, m_preset_chess(15)
	, m_prog_timer(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	memset(m_player, 0, sizeof(BOOL) * 3);
	m_seed =  (unsigned)time( NULL ) & RAND_MAX;

	m_search[0] = NULL;
	m_search[PLAYER_CATCHER] = new CBackgroundSearch;
	m_search[PLAYER_CAT] = new CFrontSearch;
}

CCrazyCatDlg::~CCrazyCatDlg(void)
{
	delete m_board;
	m_search[PLAYER_CATCHER]->Release();
	m_search[PLAYER_CAT]->Release();
}

void CCrazyCatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	CString str_status;
	if (!pDX->m_bSaveAndValidate)
	{	// update from member to gui
		switch (m_status)
		{
		case PS_SETTING:	str_status = _T("setting");	break;
		case PS_PLAYING:	str_status = _T("playing");	break;
		case PS_WIN:		str_status = _T("win");	break;
		}

		if (m_board)
		{
			int val = CRobotCatcher::Evaluate(m_board, &m_eval);
			m_checksum.Format(_T("%d"), val);
		}
	}

	DDX_Control(pDX, IDC_CHESSBOARD, m_board_ctrl);
	DDX_Control(pDX, IDC_TXT_MESSAGE, m_message_wnd);

	DDX_Check(pDX, IDC_PLAYER_CAT, m_player[PLAYER_CAT]);
	DDX_Control(pDX, IDC_PLAYER_CATCHER, m_ctrl_player_catcher);
	m_player[PLAYER_CATCHER] = m_ctrl_player_catcher.GetCurSel();

	DDX_Check(pDX, IDC_SHOW_PATH, m_show_path);
	DDX_Check(pDX, IDC_SHOW_SEARCH, m_show_search);

	DDX_Text(pDX, IDC_TXT_STATUS, str_status);

	DDX_Text(pDX, IDC_CHESS_LOCATION, m_txt_location);

	DDX_Text(pDX, IDC_CHECKSUM, m_checksum);

	DDX_Text(pDX, IDC_SEARCH_DEPTH, m_search_depth);
	DDV_MinMaxInt(pDX, m_search_depth, 1, MAX_DEPTH);

	DDV_MinMaxInt(pDX, m_search_depth, 1, 100);
	DDX_Control(pDX, IDC_BT_PLAY, m_btn_play);
	DDX_Control(pDX, IDC_BT_STOP, m_btn_stop);
	// 表示的turn和enum的turn差1
	int turn = m_turn -1;
	DDX_Radio(pDX, IDC_TURN_CATCHER, turn);
	m_turn = (PLAYER)(turn + 1);
	DDX_Control(pDX, IDC_TURN_CATCHER, m_ctrl_turn);

	DDX_Text(pDX, IDC_CELL_SIZE, m_cell_size);
	DDV_MinMaxInt(pDX, m_cell_size, 1, 100);
	DDX_Text(pDX, IDC_CELL_RADIUS, m_cell_radius);
	DDV_MinMaxInt(pDX, m_cell_radius, 1, m_cell_size);

	DDX_Text(pDX, IDC_GAME_CHESS, m_preset_chess);
	DDV_MinMaxInt(pDX, m_preset_chess, 0, 80);
	DDX_Text(pDX, IDC_SEED, m_seed);
	DDX_Control(pDX, IDC_BT_MAKEGAME, m_btn_makegame);
	DDX_Control(pDX, IDC_AI_PROGRESS, m_ai_progress);
	DDX_Control(pDX, IDC_BT_RESET, m_btn_reset);
	DDX_Control(pDX, IDC_BT_LOAD, m_btn_load);
	DDX_Control(pDX, IDC_BT_SAVE, m_btn_save);
	DDX_Control(pDX, IDC_BT_REDO, m_btn_redo);
	DDX_Control(pDX, IDC_BT_UNDO, m_btn_undo);
}

BEGIN_MESSAGE_MAP(CCrazyCatDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_EN_CHANGE(IDC_CELL_RADIUS, &CCrazyCatDlg::OnCellRadiusChanged)
	ON_EN_CHANGE(IDC_CELL_SIZE, &CCrazyCatDlg::OnCellRadiusChanged)

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
	ON_MESSAGE(WM_MSG_MOVECHESS, OnMoveChess)
	ON_BN_CLICKED(IDC_BT_SAVE, &CCrazyCatDlg::OnClickedSave)
	ON_BN_CLICKED(IDC_BT_LOAD, &CCrazyCatDlg::OnClickedLoad)
	ON_BN_CLICKED(IDC_BT_REDO, &CCrazyCatDlg::OnClickedRedo)
	ON_BN_CLICKED(IDC_BT_MAKEGAME, &CCrazyCatDlg::OnClickedMakeGame)

	ON_BN_CLICKED(IDC_CLEAR_SEED, &CCrazyCatDlg::OnClickedClearSeed)
	ON_WM_TIMER()
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

	//
	m_ctrl_player_catcher.AddString(_T("humen"));
	m_ctrl_player_catcher.AddString(_T("ai nosort"));
	m_ctrl_player_catcher.AddString(_T("ai sort"));
	m_ctrl_player_catcher.SetCurSel(0);

	//m_seed =  (unsigned)time( NULL );

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
		m_board_ctrl.SetCellSize(m_cell_size, m_cell_radius);
	}
}

void CCrazyCatDlg::OnClickedSearch()
{
}

void CCrazyCatDlg::OnClickedPlay()
{
	UpdateData();
	LOG_NOTICE(_T("start playing, CCH:%d, CAT:%d"), m_player[PLAYER_CATCHER], m_player[PLAYER_CAT]);
	LogBoard(m_board);

	if (m_player[PLAYER_CAT])
	{
		//m_robot[PLAYER_CAT] = new CRobotCat(static_cast<IRefereeListen*>(this) );
		IRobot * cat_ai = new CRobotCat(static_cast<IRefereeListen*>(this) );
		m_search[PLAYER_CAT]->SetRobot(cat_ai);
	}

	switch (m_player[PLAYER_CATCHER])
	{
	case AI_NO_SORT:	{
		IRobot * catcher_ai = new CRobotCatcher(static_cast<IRefereeListen*>(this), false );
		m_search[PLAYER_CATCHER]->SetRobot(catcher_ai);
		break;			}
	case AI_SORT:		{
		IRobot * catcher_ai = new CRobotCatcher(static_cast<IRefereeListen*>(this), true );
		m_search[PLAYER_CATCHER]->SetRobot(catcher_ai);
		break;			}
	}

	m_board->SetTurn(m_turn);
	m_btn_play.EnableWindow(FALSE);
	m_btn_stop.EnableWindow(TRUE);
	m_ctrl_turn.EnableWindow(FALSE);
	m_btn_makegame.EnableWindow(FALSE);
	m_btn_reset.EnableWindow(FALSE);
	m_btn_load.EnableWindow(FALSE);
	m_btn_save.EnableWindow(FALSE);
	m_btn_redo.EnableWindow(FALSE);
	m_btn_undo.EnableWindow(FALSE);

	m_move_count = 0;

	m_status = PS_PLAYING;
	UpdateData(FALSE);
	PassAllMessage();

	// 如果下一个棋手是AI，则消息启动AI下棋。否则返回等待PLAYER下棋。
	if ( m_player[m_turn] != AI_HUMEN) PostMessage(WM_MSG_ROBOTMOVE, m_turn, 0);
}

void CCrazyCatDlg::OnClickedStop()
{	// Change status to setting
	UpdateData();

	m_search[PLAYER_CAT]->DetachRobot();
	m_search[PLAYER_CATCHER]->DetachRobot();

	m_status = PS_SETTING;
	m_btn_play.EnableWindow(TRUE);
	m_btn_stop.EnableWindow(FALSE);
	m_ctrl_turn.EnableWindow(TRUE);
	m_btn_makegame.EnableWindow(TRUE);
	
	m_btn_reset.EnableWindow(TRUE);
	m_btn_load.EnableWindow(TRUE);
	m_btn_save.EnableWindow(TRUE);
	m_btn_redo.EnableWindow(TRUE);
	m_btn_undo.EnableWindow(TRUE);
	UpdateData(FALSE);
}

void CCrazyCatDlg::OnClickedReset()
{
	OnClickedStop();

	delete m_board;
	m_board = new CChessBoard;
	m_board_ctrl.SetBoard(m_board);

	m_board_ctrl.Draw(0, 0, 0);
}

void CCrazyCatDlg::OnClickedShowPath()
{
	UpdateData();
	m_board_ctrl.SetOption(m_show_path, m_show_search);
}

void CCrazyCatDlg::OnClickUndo()
{
	bool br=m_board->Undo();
	if (!br)
	{
		SendTextMsg(_T("Cannot undo!"));
		return;
	}
	m_turn = m_board->GetTurn();
	m_board_ctrl.Draw(0, 0, 0);
	UpdateData(FALSE);
}

void CCrazyCatDlg::OnClickedRedo()
{
	bool br = m_board->Redo();
	if (!br)
	{
		SendTextMsg(_T("Cannot redo!"));
		return;
	}
	m_turn = m_board->GetTurn();
	m_board_ctrl.Draw(0, 0, 0);
	UpdateData(FALSE);
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

void CCrazyCatDlg::OnClickedMakeGame()
{	// 随机产生一些catcher
	UpdateData();

	srand(m_seed);
	for (int ii=0; ii < m_preset_chess;)
	{
		char cc = rand() % BOARD_SIZE_COL;
		char rr = rand() % BOARD_SIZE_ROW;
		if (m_board->CheckPosition(cc, rr) != 0) continue;
		m_board->SetChess(PLAYER_CATCHER, cc, rr);
		ii ++;
	}
	m_board_ctrl.Draw(0, 0, 0);
	UpdateData(FALSE);
}

void CCrazyCatDlg::OnClickedClearSeed()
{
	UpdateData();
	m_seed =  (unsigned)time( NULL ) & RAND_MAX;
	UpdateData(FALSE);
}

///////////////////////////////////////////////////////////////////////////////
//void CCrazyCatDlg::DoMovement(CCrazyCatMovement * mv)
LRESULT	CCrazyCatDlg::OnMoveChess(WPARAM wp, LPARAM lp)
{
	CCrazyCatMovement _mv(LOBYTE(LOWORD(wp)), HIBYTE(LOWORD(wp)), HIWORD(wp));
	CCrazyCatMovement * mv = &_mv;

	if (_mv.m_col < 0 && _mv.m_row < 0)
	{	// 认输	
		SendTextMsg(_T("player %s give up"), CChessBoard::PLAYER_NAME[_mv.m_player]);
		OnClickedStop();
		return 0;
	}

	bool br = m_board->IsValidMove(mv);
	if (!br)
	{
		SendTextMsg(_T("invalide move: %s to (%d,%d)"), CChessBoard::PLAYER_NAME[mv->m_player], mv->m_col, mv->m_row);
		return 0;
	}
	LOG_NOTICE(_T("play %s -> (%d,%d)"), CChessBoard::PLAYER_SNAME[mv->m_player], mv->m_col, mv->m_row);

	if (m_prog_timer != 0)
	{
		KillTimer(m_prog_timer);
		m_prog_timer = 0;
		m_ai_progress.SetPos(100);
	}
	SendTextMsg(_T("%s move to (%d,%d)"), CChessBoard::PLAYER_NAME[mv->m_player], mv->m_col, mv->m_row);
	m_board->Move(mv);
	m_move_count ++;
	PostMessage(WM_MSG_COMPLETEMOVE, 0, 0);
	return 0;
}

LRESULT CCrazyCatDlg::OnChessClicked(WPARAM wp, LPARAM flag)
{
	char col, row;
	col = LOBYTE(wp & 0xFFFF), row = HIBYTE(wp & 0xFFFF);

	UpdateData();
	m_turn = m_board->GetTurn();

	switch (flag)
	{
	case CLICKCHESS_MOVE:
		m_txt_location.Format(_T("(%d,%d)"), col, row);
		break;

	case CLICKCHESS_LEFT:	{
		switch (m_status)
		{
		case PS_SETTING:
			m_board->SetChess(PLAYER_CATCHER, col, row);
			m_board_ctrl.Draw(0, 0, 0);
			break;
		case PS_PLAYING:	{
			// 是否有是用户下棋
			if ( !m_player[m_turn] )
			{	
				DWORD move = MAKELONG(MAKEWORD(col, row), m_turn);
				PostMessage(WM_MSG_MOVECHESS, move, 0);
			}
			break;	}
		}
		break;	}

	case CLICKCHESS_RIGHT:
		if ( PS_SETTING == m_status)
		{
			m_board->SetChess(PLAYER_CAT, col, row);
			m_board_ctrl.Draw(0, 0, 0);
		}
		break;
	}

	//m_checksum.Format(_T("%08X"), m_board->MakeHash());
	UpdateData(FALSE);
	return 0;
}

LRESULT CCrazyCatDlg::OnCompleteMove(WPARAM wp, LPARAM lp)
{
	// PLAYER或者AI下完棋，更新状态。此函数一定在GUI线程中运行
	LOG_STACK_TRACE();
	JCASSERT(m_board);
	m_board_ctrl.Draw(0, 0, 0);

	PLAYER player = PLAYER_CATCHER;
	if ( m_board->IsWin( player ) )
	{
		if (player == PLAYER_CATCHER)	SendTextMsg(_T("catcher win"));
		else							SendTextMsg(_T("cat win"));
		SendTextMsg(_T("total movement %d"), m_move_count);

		LOG_NOTICE(_T("finished, %s win, movement%d"), CChessBoard::PLAYER_SNAME[player], m_move_count);
		OnClickedStop();
		return 0;
	}

	// 转换选手
	m_turn = m_board->GetTurn();
	UpdateData(FALSE);
	PassAllMessage();

	// 如果下一个棋手是AI，则消息启动AI下棋。否则返回等待PLAYER下棋。
	if ( m_player[m_turn] != AI_HUMEN ) PostMessage(WM_MSG_ROBOTMOVE, m_turn, 0);
	return 0;
}

LRESULT CCrazyCatDlg::OnRobotMove(WPARAM wp, LPARAM lp)
{
	LOG_STACK_TRACE();
	PLAYER player = (PLAYER)(wp);
	SendTextMsg(_T("player %s is thinking ..."), CChessBoard::PLAYER_NAME[player]);
	JCASSERT(m_search[player]);
	// start a timer to update progress
	m_prog_timer = SetTimer(TIMER_ID_PROG, TIMER_INT_PROG, NULL);
	//
	m_search[player]->StartSearch(m_board, m_search_depth);
	return 0;
}

void CCrazyCatDlg::OnTimer(UINT_PTR id)
{
	if (id == TIMER_ID_PROG)
	{
		if (m_player[m_turn] != AI_HUMEN) 
		{
			int prog = m_search[m_turn]->GetProgress();
			m_ai_progress.SetPos(prog);
		}
	}
}

void CCrazyCatDlg::SearchCompleted(MOVEMENT * move)
{
	// AI搜索完博弈树后，回叫此函数。此函书可能在后台线程中运行。!!注意同步!!
	LOG_STACK_TRACE();
	//TCHAR str[256];
	CCrazyCatMovement * mv = reinterpret_cast<CCrazyCatMovement*>(move);

	// 下棋
	DWORD imv = MAKELONG(MAKEWORD(mv->m_col, mv->m_row), mv->m_player);
	PostMessage(WM_MSG_MOVECHESS, imv, 0);

	// 复制
	//m_last_move = *mv;

	//if (mv->m_col < 0 && mv->m_row < 0)
	//{	// 认输	
	//	_stprintf_s(str, _T("player %s give up"), CChessBoard::PLAYER_NAME[mv->m_player]);
	//	SendTextMsg(str);
	//	// wparam = 1 means giveup
	//	PostMessage(WM_MSG_COMPLETEMOVE, 1, 0);
	//}
	//else
	//{	

		//DoMovement(mv);
		//if (mv->m_player == PLAYER_CATCHER)
		//{
		//	CRobotCatcher * catcher = dynamic_cast<CRobotCatcher*>(m_robot[PLAYER_CATCHER]);
		//	JCASSERT(catcher);
		//	_stprintf_s(str, _T("node: % 8d, hash hit: % 6d, hash confilict: %d"),
		//		catcher->m_node, catcher->m_hash_hit, catcher->m_hash_conflict);
		//	SendTextMsg(str);
		//}
	//}
}

void CCrazyCatDlg::LogBoard(const CChessBoard * board)
{
	TCHAR str[256];
	for (int rr = 0; rr < BOARD_SIZE_ROW; ++rr)
	{
		wmemset(str, _T(' '), 256);
		int x = (rr & 1);		// 单数行缩进
		for (int cc = 0; cc < BOARD_SIZE_COL; ++cc)
		{
			BYTE chess = board->CheckPosition(cc, rr);
			if (chess == 0) str[x] = _T('-');
			else if (chess == PLAYER_CATCHER) str[x] = _T('*');
			else if (chess == PLAYER_CAT) str[x] = _T('0');
			x += 2;
		}
		str[x] = 0;
		LOG_NOTICE(str);
	}
}

void CCrazyCatDlg::SendTextMsg(LPCTSTR fmt, ...)
{
	TCHAR str[MAX_MESSAGE_LENGTH];
	va_list argptr;
	va_start(argptr, fmt);
	stdext::jc_vsprintf(str, MAX_MESSAGE_LENGTH, fmt, argptr);
	_tcscat_s(str, _T("\n"));

	int start, end;
	m_message_wnd.SetSel(0, -1);
	m_message_wnd.GetSel(start, end);
	m_message_wnd.SetSel(end, end);
	m_message_wnd.ReplaceSel(str);
}

//void CCrazyCatDlg::SendTextMsg(const CJCStringT & txt)
//{
//	int start, end;
//	m_message_wnd.SetSel(0, -1);
//	m_message_wnd.GetSel(start, end);
//	m_message_wnd.SetSel(end, end);
//	m_message_wnd.ReplaceSel(txt.c_str());
//
//	m_message_wnd.GetSel(start, end);
//	m_message_wnd.SetSel(end, end);
//	m_message_wnd.ReplaceSel(_T("\n"));
//
//	UpdateData(FALSE);
//}

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



