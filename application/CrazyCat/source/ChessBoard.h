#pragma once

#include "configuration.h"
#include "CrazyCatEvaluator.h"

#include <vector>

class CChessBoard;

class CRobotCatcher;


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
	// board 由Wrapper类提供，保证线程安全，不需要复制
	virtual bool StartSearch(CChessBoard * board, int depth) = 0;
	virtual void Release(void) = 0;
	virtual void GetProgress(long & prog, long & max_prog) = 0;
	virtual void CancelSearch(void) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// -- Crazy Cat走法
struct CCrazyCatMovement : public MOVEMENT
{
	CCrazyCatMovement(char col, char row, char player) : m_col(col), m_row(row), m_player(player) {};
	CCrazyCatMovement(void) {};
	char m_col, m_row;
	char m_player;
	int m_score;
};

typedef std::vector<CCrazyCatMovement> MOVE_STACK;


class IChessBoard
{
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
//	wp=1 giveup,
#define WM_MSG_COMPLETEMOVE		(WM_USER + 101)
// 定义消息，按下旗子，
#define WM_MSG_CLICKCHESS		(WM_USER + 102)
// 定义消息，走子
#define WM_MSG_MOVECHESS		(WM_USER + 103)

// 移动到棋子上，左键单击，右键单击
#define CLICKCHESS_MOVE			0
#define CLICKCHESS_LEFT			1
#define CLICKCHESS_RIGHT		2

class CChessBoardUi : public CStatic
{
	DECLARE_DYNAMIC(CChessBoardUi)

public:
	CChessBoardUi();
	virtual ~CChessBoardUi();

protected:
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnMouseMove(UINT flag, CPoint point);
	afx_msg void OnLButtonUp(UINT flag, CPoint point);
	afx_msg void OnRButtonUp(UINT flag, CPoint point);

	DECLARE_MESSAGE_MAP()

protected:
	void Initialize(CDC * dc);
	// 从GUI坐标转化为棋盘坐标
	bool Hit(int ux, int uy, int &cx, int &cy);
	// 计算从棋盘坐标到GUI坐标
	void Board2Gui(char col, char row, int & ux, int & uy);

public:
	void SetBoard(CChessBoard * board);

public:
	void SetCellSize(int size, int r)
	{
		m_cs = (float)size, m_ca = (float)1.155 * m_cs;		// 2/squr(3)
		m_cr = r;
		Draw(0, 0, 0);
	}

	void SetOption(bool path, bool search)
	{
		m_draw_path = path;
		m_draw_search = search;
		Draw(0, 0, 0);
	}
	void Draw(int level, char col, char row);


protected:
	// 指定重画层次，相应层次的参数(棋子，路经等)
	void DrawPath(CDC * dc);
	void DrawSearch(CDC * dc);

	bool m_draw_path, m_draw_search;
	CPen	m_pen_blue, m_pen_red, m_pen_path;
	CBrush	m_brush_blue, m_brush_red, m_brush_white;

protected:

protected:
	CChessBoard * m_chess_board;
	CCrazyCatEvaluator * m_evaluate;

	CDC		m_memdc;
	CBitmap	m_membitmap;
	CRect	m_client_rect;
	bool	m_init;

	bool	m_hit;

	int		m_cx, m_cy;

	// 棋子大小（六边形大小），棋子半径，六角形边长(辅助变量)
	float m_cs, m_ca;
	int m_cr;
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
	virtual bool Move(CCrazyCatMovement * mv);
	// 判断走棋是否合法
	virtual bool IsValidMove(CCrazyCatMovement * mv);
	// 撤销走棋
	virtual bool Undo(void);
	virtual bool Redo(void);

	void SetTurn(PLAYER turn)  {JCASSERT(turn ==1 || turn==2); m_turn = turn;}
	PLAYER GetTurn(void)  {return m_turn;}

	// 复制棋盘，用于博弈树搜索等。
	virtual void Dupe(CChessBoard * & board) const;

	virtual UINT MakeHash(void)	const;

	void SaveToFile(FILE * file) const;
	bool LoadFromFile(FILE * file);

	bool GetLastMovement(CCrazyCatMovement & mv);


	void GetCatPosition(char & col, char & row) const;
	BYTE CheckPosition(char col, char row) const;
	bool SetChess(BYTE chess, char col, char row);
	bool IsCatcherWin(void);
	bool IsCatWin(void);
	bool IsWin(PLAYER & player);
	bool IsWinPlayer(const PLAYER player);

protected:
	void PushMovement(const CCrazyCatMovement & mv);

protected:
	static const UINT RANDOM_TAB_CATCHER[BOARD_SIZE_COL][BOARD_SIZE_ROW];
	static const UINT RANDOM_TAB_CAT[BOARD_SIZE_COL][BOARD_SIZE_ROW];

	// 棋盘: 0，空；1，黑方（墙）；2，红方（猫)
	BYTE m_board[BOARD_SIZE_COL][BOARD_SIZE_ROW];
	// 猫所在的位置
	char m_cat_col, m_cat_row;
	//PLAY_STATUS		m_status;	
	// 轮到哪方走 
	PLAYER	m_turn;
	// 记录当前走法，用于恢复
	//MOVE_STACK	m_recorder;
	static const JCSIZE STACK_SIZE = BOARD_SIZE_COL * BOARD_SIZE_ROW + 10;
	CCrazyCatMovement	m_recorder[STACK_SIZE];
	JCSIZE m_stack_point, m_stack_top;
	// 用于判断catcher是否胜。
	CCrazyCatEvaluator	* m_eval;

public:
	static LPCTSTR PLAYER_NAME[3];
	static LPCTSTR PLAYER_SNAME[3];
};

