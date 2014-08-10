
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

LPCTSTR CCrazyCatDlg::PLAYER_NAME[3] = {
	_T("wrong player"),
	_T("catcher"),
	_T("cat"),
};

CCrazyCatDlg::CCrazyCatDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCrazyCatDlg::IDD, pParent)
	, m_cell_radius(15)
	, m_init(false)
	, m_show_path(TRUE),	m_show_search(FALSE)
	, m_search_depth(5)
	, m_board(NULL)
	, m_turn(PLAYER_CATCHER)
	, m_status(PS_SETTING)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	memset(m_player, 0, sizeof(BOOL) * 3);
	memset(m_robot, 0, sizeof(IRobot*) * 3);
}

CCrazyCatDlg::~CCrazyCatDlg(void)
{
	delete m_board;
	delete m_robot[PLAYER_CATCHER];
	delete m_robot[PLAYER_CAT];
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

	DDX_Text(pDX, IDC_CELL_RADIUS, m_cell_radius);
	DDX_Text(pDX, IDC_CHESS_LOCATION, m_txt_location);

	DDX_Text(pDX, IDC_CHECKSUM, m_checksum);

	DDX_Text(pDX, IDC_SEARCH_DEPTH, m_search_depth);
	DDV_MinMaxInt(pDX, m_search_depth, 1, 100);
	DDX_Control(pDX, IDC_BT_PLAY, m_btn_play);
	DDX_Control(pDX, IDC_BT_STOP, m_btn_stop);
	// 表示的turn和enum的turn差1
	int turn = m_turn -1;
	DDX_Radio(pDX, IDC_TURN_CATCHER, turn);
	m_turn = (PLAYER)(turn + 1);
	DDX_Control(pDX, IDC_TURN_CATCHER, m_ctrl_turn);
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
	ON_BN_CLICKED(IDC_BT_REDO, &CCrazyCatDlg::OnClickedRedo)
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
}

void CCrazyCatDlg::OnClickedPlay()
{
	UpdateData();
	if (m_player[PLAYER_CAT])	m_robot[PLAYER_CAT] = new CRobotCat(static_cast<IRefereeListen*>(this) );
	switch (m_player[PLAYER_CATCHER])
	{
	case AI_NO_SORT:
		m_robot[PLAYER_CATCHER] = new CRobotCatcher(static_cast<IRefereeListen*>(this), false );
		break;
	case AI_SORT:
		m_robot[PLAYER_CATCHER] = new CRobotCatcher(static_cast<IRefereeListen*>(this), true );
		break;
	}

	m_board->SetTurn(m_turn);
	m_btn_play.EnableWindow(FALSE);
	m_btn_stop.EnableWindow(TRUE);
	m_ctrl_turn.EnableWindow(FALSE);
	m_move_count = 0;

	m_status = PS_PLAYING;
	UpdateData(FALSE);
	PassAllMessage();

	// 如果下一个棋手是AI，则消息启动AI下棋。否则返回等待PLAYER下棋。
	if ( m_robot[m_turn]) PostMessage(WM_MSG_ROBOTMOVE, m_turn, 0);
}

void CCrazyCatDlg::OnClickedStop()
{	// Change status to setting
	delete m_robot[PLAYER_CAT];	
	delete m_robot[PLAYER_CATCHER];
	memset(m_robot, 0, sizeof(IRobot) * 3);

	m_status = PS_SETTING;
	m_btn_play.EnableWindow(TRUE);
	m_btn_stop.EnableWindow(FALSE);
	m_ctrl_turn.EnableWindow(TRUE);
	UpdateData(FALSE);
}


void CCrazyCatDlg::OnClickedReset()
{
	OnClickedStop();

	delete m_board;
	m_board = new CChessBoard;
	m_board_ctrl.SetBoard(m_board);

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

///////////////////////////////////////////////////////////////////////////////
void CCrazyCatDlg::DoMovement(CCrazyCatMovement * mv)
{
	TCHAR str[256];

	bool br = m_board->IsValidMove(mv);
	if (!br)
	{
		_stprintf_s(str, _T("invalide move: %s to (%d,%d)"), PLAYER_NAME[mv->m_player], mv->m_col, mv->m_row);
		SendTextMsg(str);
		return;
	}

	_stprintf_s(str, _T("%s move to (%d,%d)"), PLAYER_NAME[mv->m_player], mv->m_col, mv->m_row);
	SendTextMsg(str);
	m_board->Move(mv);
	m_move_count ++;
	PostMessage(WM_MSG_COMPLETEMOVE, 0, 0);
}

LRESULT CCrazyCatDlg::OnChessClicked(WPARAM wp, LPARAM flag)
{
	char col, row;
	col = LOBYTE(wp & 0xFFFF), row = HIBYTE(wp & 0xFFFF);

	m_turn = m_board->GetTurn();

	switch (flag)
	{
	case CLICKCHESS_MOVE:
		m_txt_location.Format(_T("(%d,%d)"), col, row);
		UpdateData(FALSE);
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
				CCrazyCatMovement mv(col, row, m_turn);
				DoMovement(&mv);
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
	//m_checksum.Format(_T("%08X"), m_board->MakeHash());
	m_board_ctrl.Draw(0, 0, 0);

	if (1 == wp)
	{	// give up
		OnClickedStop();
		return 0;
	}

	PLAYER player = PLAYER_CATCHER;
	if ( m_board->IsWin( player ) )
	{
		TCHAR str[128];
		if (player == PLAYER_CATCHER)	SendTextMsg(_T("catcher win"));
		else							SendTextMsg(_T("cat win"));
		stdext::jc_sprintf(str, _T("total movement %d"), m_move_count);
		SendTextMsg(str);
		OnClickedStop();
		return 0;
	}

	// 转换选手
	m_turn = m_board->GetTurn();
	UpdateData(FALSE);
	PassAllMessage();

	// 如果下一个棋手是AI，则消息启动AI下棋。否则返回等待PLAYER下棋。
	if ( m_robot[m_turn]) PostMessage(WM_MSG_ROBOTMOVE, m_turn, 0);
	return 0;
}

LRESULT CCrazyCatDlg::OnRobotMove(WPARAM wp, LPARAM lp)
{
	LOG_STACK_TRACE();
	JCASSERT(m_robot[wp]);
	m_robot[wp]->StartSearch(m_board, m_search_depth);
	return 0;
}

void CCrazyCatDlg::SearchCompleted(MOVEMENT * move)
{
	// AI搜索完博弈树后，回叫此函数。此函书可能在后台线程中运行。!!注意同步!!
	LOG_STACK_TRACE();
	TCHAR str[256];
	CCrazyCatMovement * mv = reinterpret_cast<CCrazyCatMovement*>(move);

	if (mv->m_col < 0 && mv->m_row < 0)
	{	// 认输	
		_stprintf_s(str, _T("player %s give up"), PLAYER_NAME[mv->m_player]);
		SendTextMsg(str);
		// wparam = 1 means giveup
		PostMessage(WM_MSG_COMPLETEMOVE, 1, 0);
	}
	else
	{	// 下棋
		DoMovement(mv);
		if (mv->m_player == PLAYER_CATCHER)
		{
			CRobotCatcher * catcher = dynamic_cast<CRobotCatcher*>(m_robot[PLAYER_CATCHER]);
			JCASSERT(catcher);
			_stprintf_s(str, _T("node: % 8d, hash hit: % 6d, hash confilict: %d"),
				catcher->m_node, catcher->m_hash_hit, catcher->m_hash_conflict);
			SendTextMsg(str);
		}
	}
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



