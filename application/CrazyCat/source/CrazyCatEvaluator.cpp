#include "StdAfx.h"
#include "CrazyCatEvaluator.h"

#include "configuration.h"
#include "ChessBoard.h"

LOCAL_LOGGER_ENABLE(_T("evaluate"), LOGGER_LEVEL_NOTICE);

///////////////////////////////////////////////////////////////////////////////
// -- move table
const char MOVE[2][MAX_MOVEMENT][2] = {
	{ {-1,-1}, {0,-1}, {-1,0}, {1,0}, {-1,1}, {0,1}, },
	{ {0,-1},  {1,-1}, {-1,0}, {1,0}, {0,1},  {1,1}, },
};

void Move(char col, char row, char way, char & new_col, char & new_row)
{
	JCASSERT(way < MAX_MOVEMENT);
	char dx = MOVE[row & 1][way][0];
	char dy = MOVE[row & 1][way][1];
	new_col = col + dx, new_row = row + dy;
}

///////////////////////////////////////////////////////////////////////////////
// -- A*搜索节点类，
LOG_CLASS_SIZE(CSearchNode);

CSearchNode::CSearchNode(char col, char row, char move, CSearchNode * father)
	: m_cat_col(col), m_cat_row(row), m_move(move)
	, m_father(father), m_next(NULL), m_depth(0), m_evaluate(0)
{
	Init(col, row, move, father);
}

void CSearchNode::Init(char col, char row, char move, CSearchNode * father)
{
	m_cat_col = col, m_cat_row = row, m_move = move;
	m_father = father, m_next = NULL;
	m_depth = 0, m_evaluate = 0;

	char x0 = -1, y0=-1;
	if (m_father)
	{
		x0 = m_father->m_cat_col, y0= m_father->m_cat_row;
		m_depth = m_father->m_depth + 1;
	}
	int r = BOARD_SIZE_COL - col -1;
	int b = BOARD_SIZE_ROW - row -1;
	m_evaluate = m_depth + min( min(col, r), min(row, b) );
	
	LOG_DEBUG_(1, _T("exp node: (%d,%d) -%d-> (%d,%d), d: %d, e: %d"), x0, y0, m_move,
		m_cat_col, m_cat_row, m_depth, m_evaluate);
}

///////////////////////////////////////////////////////////////////////////////
// -- 评价类，
LOG_CLASS_SIZE(CCrazyCatEvaluator);

CCrazyCatEvaluator::CCrazyCatEvaluator(void)
	: m_head(NULL), m_closed(NULL)
	, m_succeeded(NULL)
	, m_board(NULL)
{
	m_node_array = new CSearchNode[BOARD_SIZE_COL * BOARD_SIZE_ROW];

}

CCrazyCatEvaluator::CCrazyCatEvaluator(const CChessBoard * board)
	: m_head(NULL), m_closed(NULL)
	, m_succeeded(NULL)
	, m_board(board)
{
	JCASSERT(m_board);
	m_node_array = new CSearchNode[BOARD_SIZE_COL * BOARD_SIZE_ROW];
	Reset(board);
}

CCrazyCatEvaluator::~CCrazyCatEvaluator(void)
{
	delete [] m_node_array;
}

void CCrazyCatEvaluator::Reset(const CChessBoard * board)
{
	JCASSERT(m_node_array);
	m_closed_qty = 0, m_open_qty = 0;
	m_head = NULL, m_closed = NULL, m_succeeded = NULL;
	m_board = board;
	memset(m_node_array, 0xFF, sizeof(CSearchNode) * BOARD_SIZE_COL * BOARD_SIZE_ROW);
}


int CCrazyCatEvaluator::StartSearch(void)
{
	JCASSERT(m_board);
	LOG_STACK_PERFORM(_T(""));

	// 初始化指针，添加第一个节点
	char col = 0, row = 0;
	m_board->GetCatPosition(col, row);
	m_head = NewNode(col, row, -1, NULL);
	JCASSERT(m_head);
	m_closed = m_head;
	if (m_head->m_evaluate == 0) return m_head->m_depth;

	// 扩展closed节点，排序插入open队列
	while (1)
	{
		m_closed_qty ++;
		col = m_closed->m_cat_col, row = m_closed->m_cat_row;
		for (char ww = 0; ww < MAX_MOVEMENT; ++ww)
		{
			// 扩展节点
			char new_col, new_row;
			Move(col, row, ww, new_col, new_row);
			if (m_board->CheckPosition(new_col, new_row) != 0) continue;
			// 根据位置申请节点，如果节点已分配（返回NULL）表示该节点已被搜索。
			CSearchNode * node = NewNode(new_col, new_row, ww, m_closed);
			if (!node) continue;	// node 已经搜索过了

			// 插入节点
			CSearchNode * nm = m_closed;
			CSearchNode * nn = nm->m_next;
			while (nn)
			{
				if (nn->m_evaluate > node->m_evaluate) break;	// 找到位置
				nm = nn;
				nn = nm->m_next;
			}
			//插入 nn 之前，nm之后
			node->m_next = nn;
			nm->m_next = node;
			// 性能评估
			m_open_qty ++;

			if (node->m_depth == node->m_evaluate)
			{	// found
				m_succeeded = node;
				break;
			}
		}
		if (m_succeeded) break;
		m_closed = m_closed->m_next;
		if (NULL == m_closed) break;	// 未找到节点
	}

	if (m_succeeded)		return m_succeeded->m_depth;
	else					return MAX_DISTANCE;
}
