#pragma once

#include <stdext.h>
#include "configuration.h"

#define MAX_MOVEMENT	6

#define MAX_SCORE	200
#define MIN_SCORE	0

class CChessBoard;


///////////////////////////////////////////////////////////////////////////////
// -- move table
//extern const char MOVE[2][MAX_MOVEMENT][2]; 

void Move(char col, char row, char way, char & new_col, char & new_row);


///////////////////////////////////////////////////////////////////////////////
// -- A*搜索节点类，

class CSearchNode
{
public:
	CSearchNode(void) {memset(this, 0, sizeof(CSearchNode));}
	CSearchNode(char col, char row, char move, CSearchNode * farther);

	CSearchNode * m_father;		// 上一步搜索结果。
	CSearchNode * m_next;		// 搜索链表的下一个。
	char m_move;					// 从上一步走过来的方法。
	char m_cat_col, m_cat_row;				// 当前猫的位置
	int m_depth;				// 搜索深度
	int m_evaluate;				// 评价结果 = 当前深度 + 到最近边缘的距离
};



///////////////////////////////////////////////////////////////////////////////
// -- 评价类，
//  找到红方走到边远的最短路径，A*搜索。

class CCrazyCatEvaluator
{
public:
	CCrazyCatEvaluator(const CChessBoard * board);
	virtual ~CCrazyCatEvaluator(void);

public:
	// 开始搜索
	int StartSearch(void);
	CSearchNode * GetSuccess(void) { return m_succeeded; }
	CSearchNode * GetHead(void) { return m_head; }

protected:
	const CChessBoard * m_board;

	CSearchNode * m_head;
	CSearchNode * m_closed;
	CSearchNode * m_succeeded;

	// 哈希表，用于标记已经搜索过的位置。
	CSearchNode * m_hash[BOARD_SIZE_COL * BOARD_SIZE_ROW];

public:
	// 用于性能测试
	UINT	m_closed_qty;		// closed队列数量
	UINT	m_open_qty;			// open队列数量

};
