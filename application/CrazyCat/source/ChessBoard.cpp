// ChessBoard.cpp : 实现文件
//

#include "stdafx.h"
#include "CrazyCat.h"
#include "ChessBoard.h"
#include "robot_cat.h"

#include "stdext.h"

LOCAL_LOGGER_ENABLE(_T("chess_board"), LOGGER_LEVEL_DEBUGINFO);

// CChessBoardUi

IMPLEMENT_DYNAMIC(CChessBoardUi, CStatic)

CChessBoardUi::CChessBoardUi()
	: m_init(false)
	, m_chess_board(NULL)
	, m_hit(false), m_cx(0), m_cy(0)
	, m_evaluate(NULL)
	, m_listen(NULL)
	, m_draw_path(true), m_draw_search(false)
	, m_catcher_ai(false), m_cat_ai(false)
	, m_cat_robot(NULL)
{
	m_cr = CHESS_RADIUS;
	m_ca = (float)1.155 * m_cr;		// 2/squr(3)
}

CChessBoardUi::~CChessBoardUi()
{
	delete m_chess_board;
	delete m_evaluate;
	delete m_cat_robot;
}

void CChessBoardUi::SetBoard(CChessBoard * board)
{
	JCASSERT(board);
	delete m_chess_board;
	m_chess_board = board;
}

bool CChessBoardUi::Hit(int ux, int uy, int &cx, int &cy)
{
	bool hit = true;;
	LOG_DEBUG_(1, _T("ptr: (%d, %d)"), ux, uy)

	int yy = (int)(uy / (1.5 * m_ca));
	int xx = (int)(ux / m_cr);
	if (yy & 1)
	{
		if (xx == 0) hit = false;
		cx = (xx-1) /2;
	}
	else		cx = xx /2;
	LOG_DEBUG_(1, _T("xx=%d, cx=%d"), xx, cx);
	cy = yy;

	if (cx < 0 || cx >= BOARD_SIZE_COL) hit = false;
	if (cy < 0 || cy >= BOARD_SIZE_ROW) hit = false;


	return hit;
}

void CChessBoardUi::Reset(void)
{
	delete m_chess_board;
	m_chess_board = new CChessBoard;
	delete m_evaluate;
	m_evaluate = NULL;
	delete m_cat_robot;
	m_cat_robot = NULL;
	PostMessage(WM_MSG_COMPLETEMOVE);
}

void CChessBoardUi::Board2Gui(char col, char row, int & ux, int & uy)
{
	if (row & 1)	ux = (int)(2 * m_cr + col * 2 * m_cr);
	else			ux = (int)(m_cr + col * 2 * m_cr);
	uy = (int)(m_ca + row * 1.5 * m_ca);
}

BEGIN_MESSAGE_MAP(CChessBoardUi, CStatic)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_MESSAGE(WM_MSG_ROBOTMOVE, OnRobotMove)
	ON_MESSAGE(WM_MSG_COMPLETEMOVE, OnCompleteMove)
END_MESSAGE_MAP()

// CChessBoardUi 消息处理程序
int CChessBoardUi::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	LOG_STACK_TRACE();
	int ir = CWnd::OnCreate(lpCreateStruct);
	if (ir != 0) return ir;

	CPaintDC dc(this);
	return ir;
}

void CChessBoardUi::Initialize(CDC * dc)
{
	JCASSERT(dc);

	GetClientRect(&m_client_rect);
	LOG_DEBUG(_T("client rect: (%d,%d) - (%d,%d)"), m_client_rect.left, m_client_rect.top, m_client_rect.right, m_client_rect.bottom);

	m_memdc.CreateCompatibleDC(dc);
	m_membitmap.CreateCompatibleBitmap(dc, m_client_rect.Width(), m_client_rect.Height() );
	m_memdc.SelectObject(&m_membitmap);

	// prepare drawing object
	m_pen_blue.CreatePen(PS_SOLID | PS_COSMETIC, 1, RGB(0, 0, 255) );
	m_pen_red.CreatePen(PS_SOLID | PS_COSMETIC, 1, RGB(255, 0, 0) );
	m_brush_blue.CreateSolidBrush(RGB(0, 0, 255));
	m_brush_red.CreateSolidBrush(RGB(255, 0, 0));
	m_brush_white.CreateSolidBrush(RGB(255, 255, 255));
	m_pen_path.CreatePen(PS_SOLID | PS_COSMETIC, 3, RGB(0, 255, 0) );

	m_init = true;
	Draw(0, 0, 0);
}

void CChessBoardUi::Draw(int level, char col, char row)
{
	if (!m_chess_board) return;
	// draw back ground
	m_memdc.FillSolidRect(&m_client_rect, GetSysColor(COLOR_3DFACE) );

	// draw cells
	float dx, dy;
	dy = m_ca;
	for ( int ii = 0; ii < BOARD_SIZE_ROW; ++ii)
	{
		if (ii & 1)	dx = 2 * m_cr;
		else		dx = m_cr;
		int y0 = (int)(dy - m_cr);
		int y1 = (int)(dy + m_cr);

		for ( int jj = 0; jj < BOARD_SIZE_COL; ++jj)
		{
			int x0 = (int)(dx - m_cr);
			int x1 = (int)(dx + m_cr);

			BYTE c = m_chess_board->CheckPosition(jj, ii);
			switch (c)
			{
			case 0:	// empty
				if ( (m_hit) && (ii == m_cy) && (jj == m_cx) )		m_memdc.SelectObject(&m_pen_red);
				else						m_memdc.SelectObject(&m_pen_blue);
				m_memdc.SelectObject(&m_brush_white);
				break;
			case 1: // blue
				m_memdc.SelectObject(&m_pen_blue);
				m_memdc.SelectObject(&m_brush_blue);
				break;
			case 2: // red
				m_memdc.SelectObject(&m_pen_red);
				m_memdc.SelectObject(&m_brush_red);
				break;
			}			
			m_memdc.Ellipse(x0, y0, x1, y1);
			dx += 2 * m_cr;
		}
		dy += (float)1.5 * m_ca;
	}

	if (m_draw_path  && m_evaluate)	DrawPath(&m_memdc);
	if (m_draw_search && m_evaluate) DrawSearch(&m_memdc);

	RedrawWindow();
}

void CChessBoardUi::DrawPath(CDC * dc)
{
	JCASSERT(m_evaluate);
	dc->SelectObject(&m_pen_path);
	
	int ix, iy;

	CSearchNode * node = m_evaluate->GetSuccess();
	if (node)	
	{
		Board2Gui(node->m_cat_col, node->m_cat_row, ix, iy);
		dc->MoveTo(ix, iy);
		node = node->m_father;
	}

	while (node)
	{
		Board2Gui(node->m_cat_col, node->m_cat_row, ix, iy);
		dc->LineTo(ix, iy);
		node = node->m_father;
	}
}

void CChessBoardUi::DrawSearch(CDC * dc)
{
	JCASSERT(m_evaluate);
	CSearchNode * node = m_evaluate->GetHead();

	UINT ii = 0;

	while (node)
	{
		int ix, iy;
		char col = node->m_cat_col, row = node->m_cat_row;
		Board2Gui(col, row, ix, iy);
		LOG_DEBUG(_T("(%d, %d) : (%d, %d)"), col, row, ix, iy);
		if (col >= 0 && row >= 0)
		{
			if (ii < m_evaluate->m_closed_qty)	
				dc->FillSolidRect(ix-3, iy-3, 6, 6, RGB(128, 0, 0));
			else	
				dc->FillSolidRect(ix-3, iy-3, 6, 6, RGB(128, 64, 64));
		}
		ii ++;
		node = node->m_next;
	}
}


void CChessBoardUi::OnLButtonUp(UINT flag, CPoint point)
{
	int cx, cy;
	bool hit = Hit(point.x, point.y, cx, cy);
	if (!hit) return;

	bool br = false;
	switch (m_chess_board->Status())
	{
	case PS_SETTING:	{
		BYTE ss = m_chess_board->CheckPosition(cx, cy);
		if ( 1 == ss ) ss = 0;
		else if ( 0 == ss ) ss = 1;
		else return;
		m_chess_board->SetPosition(cx, cy, ss);
		br = true;
		Draw(0, 0, 0);
		break;				}

	case PS_CATCHER_MOVE:
		if (!m_catcher_ai)
		{
			br = m_chess_board->Move(PLAYER_CATCHER, cx, cy);
			if (br)		PostMessage(WM_MSG_COMPLETEMOVE, 0, 0);
		}
		break;

	case PS_CAT_MOVE:
		if (!m_cat_robot) 
		{	// cat是player
			br = m_chess_board->Move(PLAYER_CAT, cx, cy);
			if (br)
			{
				StartSearch();
				PostMessage(WM_MSG_COMPLETEMOVE, 0, 0);
			}
		}
		break;
	}
	//if (br)
	//{
	//	UpdateStatus(m_chess_board->Status());
	//	Draw(0, 0, 0);
	//}
	//RedrawWindow();
}

LRESULT CChessBoardUi::OnRobotMove(WPARAM wp, LPARAM lp)
{
	LOG_STACK_TRACE();
	if (wp == PLAYER_CAT)
	{	// AI CAT下棋
		JCASSERT(m_cat_robot);
		m_cat_robot->StartSearch(m_chess_board, 0);
	}
	else
	{	// AI CATCHER下棋
	}
	return 0;
}

LRESULT CChessBoardUi::OnCompleteMove(WPARAM wp, LPARAM lp)
{
	// PLAYER或者AI下完棋，更新状态。此函数一定在GUI线程中运行
	LOG_STACK_TRACE();

	JCASSERT(m_chess_board);

	PLAY_STATUS ss = m_chess_board->Status();

	CJCStringT str;
	switch (ss)
	{
	case PS_SETTING:		str = _T("setting");	break;
	case PS_CATCHER_MOVE:	str = _T("catcher moving");	break;
	case PS_CAT_MOVE:		
		str = _T("cat moving");	
		// 轮到CAT下棋，如果是AI，则消息启动AI下棋。否则返回等待PLAYER下棋。
		if (m_cat_robot)	PostMessage(WM_MSG_ROBOTMOVE, PLAYER_CAT, 0);
		break;
	case PS_CATCHER_WIN:	str = _T("catcher win");	break;
	case PS_CAT_WIN:		str = _T("cat win");	break;
	}
	if (m_listen) m_listen->SetTextStatus(str);
	Draw(0, 0, 0);
	return 0;
}

//void CChessBoardUi::UpdateStatus(PLAY_STATUS ss)
//{
//	LOG_STACK_TRACE();
//
//	if (!m_listen) return;
//	CJCStringT str;
//	switch (ss)
//	{
//	case PS_SETTING:		str = _T("setting");	break;
//	case PS_CATCHER_MOVE:	str = _T("catcher moving");	break;
//	case PS_CAT_MOVE:		
//		str = _T("cat moving");	
//		if (m_cat_robot) 
//		{	// 轮到Cat走棋
//			m_cat_robot->StartSearch(m_chess_board, 0);
//		}
//		break;
//	case PS_CATCHER_WIN:	str = _T("catcher win");	break;
//	case PS_CAT_WIN:		str = _T("cat win");	break;
//	}
//	if (m_listen) m_listen->SetTextStatus(str);
//}

void CChessBoardUi::SearchCompleted(MOVEMENT * move)
{
	// AI搜索完博弈树后，回叫此函数。此函书可能在后台线程中运行。!!注意同步!!
	LOG_STACK_TRACE();
	CCrazyCatMovement * mv = reinterpret_cast<CCrazyCatMovement*>(move);
	switch (m_chess_board->Status() )
	{
	case PS_CAT_MOVE:
		if (mv->m_col < 0 && mv->m_row < 0)		m_chess_board->GiveUp(PLAYER_CAT);	// 认输
		else
		{
			m_chess_board->Move(PLAYER_CAT, mv->m_col, mv->m_row);
			StartSearch();
		}
		break;
	}
	//UpdateStatus(m_chess_board->Status());
	PostMessage(WM_MSG_COMPLETEMOVE, 0, 0);
}





void CChessBoardUi::OnMouseMove(UINT flag, CPoint point)
{
	//LOG_STACK_TRACE();
	m_hit = Hit(point.x, point.y, m_cx, m_cy);
	LOG_DEBUG_(1, _T("(%d, %d), %d"), point.x, point.y, m_hit);
	//if (m_hit) RedrawWindow();
	if (m_hit) Draw(0,0,0);
}

void CChessBoardUi::OnPaint(void)
{
	//LOG_STACK_TRACE();
	if (!m_chess_board) return;

	CPaintDC dc(this);
	if (!m_init) Initialize(&dc);
	dc.BitBlt(0, 0, m_client_rect.Width(), m_client_rect.Height(), &m_memdc, 0, 0, SRCCOPY);
}

void CChessBoardUi::StartSearch(void)
{
	if (m_evaluate)	delete m_evaluate;
	m_evaluate = new CCrazyCatEvaluator(m_chess_board);
	int ir = m_evaluate->StartSearch();

	TCHAR str[256];
	if (ir < INT_MAX)
	{
		_stprintf_s(str, _T("route found. depth = %d, expanded %d, closed %d\n"),
			ir, m_evaluate->m_open_qty, m_evaluate->m_closed_qty);
	}
	else
	{
		_stprintf_s(str, _T("no route found. expanded %d, closed %d\n"),
			m_evaluate->m_open_qty, m_evaluate->m_closed_qty);
	}

	if (m_listen) m_listen->SendTextMsg(str);

	//RedrawWindow();
	Draw(0,0,0);
}

void CChessBoardUi::StartPlay(bool player_blue, bool player_cat)
{
	//<TODO>
	if (player_cat)
	{
		//m_cat_ai = true;
		delete m_cat_robot;
		m_cat_robot = new CRobotCat(static_cast<IRefereeListen*>(this) );
	}
	else
	{
		delete m_cat_robot;
		m_cat_robot = NULL;
	}
	PLAY_STATUS ps = m_chess_board->StartPlay();
	//UpdateStatus(ps);
	PostMessage(WM_MSG_COMPLETEMOVE, 0, 0);

}

///////////////////////////////////////////////////////////////////////////////
// -- 棋盘类，

CChessBoard::CChessBoard(void)
	: m_status(PS_SETTING)
{
	memset(m_board, 0, sizeof(m_board));

	m_cat_pos_col = 4;
	m_cat_pos_row = 4;

	m_board[m_cat_pos_col][m_cat_pos_row] = 2;
}

CChessBoard::~CChessBoard(void)
{
}


void CChessBoard::GetCatPosition(char & col, char & row) const
{
	col = m_cat_pos_col;
	row = m_cat_pos_row;
}

BYTE CChessBoard::CheckPosition(char col, char row) const
{
	if (col < 0 || row < 0 || col >= BOARD_SIZE_COL || row >= BOARD_SIZE_ROW)	
		return 0xFF;	// out of range
	return m_board[col][row];
}

void CChessBoard::SetPosition(char col, char row, BYTE ss)
{
	if (col < 0 || row < 0 || col >= BOARD_SIZE_COL || row >= BOARD_SIZE_ROW)	
		return;
	m_board[col][row] = ss;
}

PLAY_STATUS CChessBoard::StartPlay(void)
{
	m_status = PS_CATCHER_MOVE;
	return m_status;
}

bool CChessBoard::Move(BYTE player, int col, int row) 
{
	if (col < 0 || col >= BOARD_SIZE_COL || row < 0 || row >= BOARD_SIZE_ROW) return false;

	if (player == PLAYER_CATCHER)
	{
		if (m_status != PS_CATCHER_MOVE) return false;
		if (m_board[col][row] != 0)	return false;
		m_board[col][row] = 1;
		if ( IsCatcherWin() )	m_status = PS_CATCHER_WIN;
		else					m_status = PS_CAT_MOVE;
	}
	else
	{
		if (m_status != PS_CAT_MOVE) return false;
		// 检查是否在周围六格之内
		int ww = 0;
		for (; ww < MAX_MOVEMENT; ++ww)
		{
			char c1, r1;
			::Move(m_cat_pos_col, m_cat_pos_row, ww, c1, r1);
			if (c1 == col && r1 == row) break;
		}
		if (ww >= MAX_MOVEMENT) return false;
		m_board[m_cat_pos_col][m_cat_pos_row] = 0;
		m_cat_pos_col = col, m_cat_pos_row = row;
		m_board[m_cat_pos_col][m_cat_pos_row] = 2;
		
		if ( IsCatWin() )		m_status = PS_CAT_WIN;
		else					m_status = PS_CATCHER_MOVE;
	}
	return true;
}

bool CChessBoard::IsCatcherWin(void)
{
	for (int ww = 0; ww < MAX_MOVEMENT; ++ww)
	{
		char c1, r1;
		::Move(m_cat_pos_col, m_cat_pos_row, ww, c1, r1);
		if (m_board[c1][r1] != 1) return false;
	}
	return true;
}

bool CChessBoard::IsCatWin(void)
{
	if (m_cat_pos_col == 0 || m_cat_pos_col == (BOARD_SIZE_COL-1) ) return true;
	if (m_cat_pos_row == 0 || m_cat_pos_row == (BOARD_SIZE_ROW-1) ) return true;
	return false;
}