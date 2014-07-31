#pragma once

#include "configuration.h"
#include "CrazyCatEvaluator.h"

class CChessBoard;


///////////////////////////////////////////////////////////////////////////////
// -- 用于定义走法的数据结构，可以被继承
struct MOVEMENT
{
};

///////////////////////////////////////////////////////////////////////////////
// -- 主界面监听
class IMessageListen
{
public:
	virtual void SendTextMsg(const CJCStringT & txt) = 0;
	virtual void SetTextStatus(const CJCStringT & txt) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// -- 裁判监听
class IRefereeListen
{
public:
	virtual void SearchCompleted(MOVEMENT * move) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// -- AI 接口
class IRobot
{
public:
	virtual bool StartSearch(const CChessBoard * board, int depth) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// -- Crazy Cat走法
struct CCrazyCatMovement : public MOVEMENT
{
	CCrazyCatMovement(char col, char row) : m_col(col), m_row(row) {};
	char m_col, m_row;
};

class IChessBoard
{
};

enum PLAY_STATUS
{	
	PS_SETTING,
	PS_CATCHER_MOVE,	PS_CAT_MOVE,
	PS_CATCHER_WIN,		PS_CAT_WIN,
};

enum PLAYER
{
	PLAYER_CATCHER = 1,
	PLAYER_CAT = 2,
};


// CChessBoardUi

// 定义消息，轮到AI下棋
#define WM_MSG_ROBOTMOVE		(WM_USER + 100)
// 定义消息，AI或者PLAYER下完棋，更新装态
#define WM_MSG_COMPLETEMOVE		(WM_USER + 101)

class CChessBoardUi : public CStatic
	, public IRefereeListen
{
	DECLARE_DYNAMIC(CChessBoardUi)

public:
	CChessBoardUi();
	virtual ~CChessBoardUi();

protected:
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	DECLARE_MESSAGE_MAP()

	void Initialize(CDC * dc);

	// 从GUI坐标转化为棋盘坐标
	bool Hit(int ux, int uy, int &cx, int &cy);
	// 计算从棋盘坐标到GUI坐标
	void Board2Gui(char col, char row, int & ux, int & uy);

public:
	void SetBoard(CChessBoard * board);
	afx_msg void OnMouseMove(UINT flag, CPoint point);
	afx_msg void OnLButtonUp(UINT flag, CPoint point);

	void StartSearch(void);

	void SetListener(IMessageListen * l)	{m_listen = l;}

	void StartPlay(bool player_blue, bool player_cat);

	void SetCellRadius(int r)
	{
		m_cr = (float)r, m_ca = (float)1.155 * m_cr;		// 2/squr(3)
		Draw(0, 0, 0);
	}

	void SetOption(bool path, bool search)
	{
		m_draw_path = path;
		m_draw_search = search;
		Draw(0, 0, 0);
	}
	virtual void SearchCompleted(MOVEMENT * move);

	void Reset(void);



protected:
	// 指定重画层次，相应层次的参数(棋子，路经等)
	void Draw(int level, char col, char row);
	void DrawPath(CDC * dc);
	void DrawSearch(CDC * dc);

	bool m_draw_path, m_draw_search;
	CPen	m_pen_blue, m_pen_red, m_pen_path;
	CBrush	m_brush_blue, m_brush_red, m_brush_white;

protected:
	afx_msg LRESULT OnCompleteMove(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnRobotMove(WPARAM wp, LPARAM lp);
	bool m_catcher_ai, m_cat_ai;
	IRobot * m_cat_robot;

protected:
	CChessBoard * m_chess_board;
	CCrazyCatEvaluator * m_evaluate;

	CDC		m_memdc;
	CBitmap	m_membitmap;
	CRect	m_client_rect;
	bool	m_init;

	bool	m_hit;
	int		m_cx, m_cy;

	IMessageListen * m_listen;
	// 棋子半径，六角形边长
	float m_cr, m_ca;
};


///////////////////////////////////////////////////////////////////////////////
// -- 棋盘类，
//  管理棋盘布局，下棋操作等
class CChessBoard : public IChessBoard
{
public:
	CChessBoard(void);
	virtual ~CChessBoard(void);

public:
	friend class CCrazyCatEvaluator;

public:
	// 走棋：
	//	player，选手，1
	//  x, y, 走法：
	virtual bool Move(BYTE player, int x, int y);
	// 判断走棋是否合法
	virtual bool IsValidMove(BYTE player, int x, int y) {return false;};
	// 撤销走棋
	virtual void Undo(void) {};

	virtual PLAY_STATUS StartPlay(void);

	virtual void GiveUp(BYTE player)
	{
		if (player == PLAYER_CAT) m_status = PS_CATCHER_WIN;
		else m_status = PS_CAT_WIN;
	}

	void GetCatPosition(char & col, char & row) const;
	BYTE CheckPosition(char col, char row) const;
	void SetPosition(char col, char row, BYTE ss);
	PLAY_STATUS Status(void) const {return m_status;}

protected:
	bool IsCatcherWin(void);
	bool IsCatWin(void);

protected:
	// 棋盘: 0，空；1，黑方（墙）；2，红方（猫)
	BYTE m_board[BOARD_SIZE_COL][BOARD_SIZE_ROW];

	// 猫所在的位置
	char m_cat_pos_col, m_cat_pos_row;
	
	PLAY_STATUS		m_status;	// 轮到哪方走 
	//BYTE m_

};

