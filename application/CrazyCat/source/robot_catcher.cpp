#include "StdAfx.h"
#include "robot_catcher.h"

#include <stdext.h>

#include <stdlib.h>
#include <search.h>

LOCAL_LOGGER_ENABLE(_T("search_engine"), LOGGER_LEVEL_DEBUGINFO -1);

#define SORT_MOVEMENT

///////////////////////////////////////////////////////////////////////////////
// -- 
CCatcherMoveGenerator::CCatcherMoveGenerator(void)
{
	//memset(m_array, 0, sizeof (CCrazyCatMovement) * ARRAY_SIZE);
	//memset(m_index, 0, sizeof (CCrazyCatMovement *) * ARRAY_SIZE);
	//m_move_count = 0;
}

CCatcherMoveGenerator::~CCatcherMoveGenerator(void)
{

}
	
JCSIZE CCatcherMoveGenerator::Generate(const CChessBoard * board)
{
	JCASSERT(board);
	memset(m_array, 0, sizeof (CCrazyCatMovement) * ARRAY_SIZE);
	memset(m_index, 0, sizeof (CCrazyCatMovement *) * ARRAY_SIZE);

	char cat_c, cat_r;
	board->GetCatPosition(cat_c, cat_r);
	m_move_count = 0;
	for (int rr = 0; rr < BOARD_SIZE_ROW; ++rr)
	{
		int delta_r = abs(cat_r - rr);
		for (int cc = 0; cc < BOARD_SIZE_COL; ++cc)
		{
			if (board->CheckPosition(cc, rr) != 0) continue;
			CCrazyCatMovement & mv = m_array[m_move_count];
			mv.m_col = cc, mv.m_row = rr, mv.m_player = PLAYER_CATCHER;
			mv.m_score = abs(cat_c - cc) + delta_r;
			m_index[m_move_count] = m_array + m_move_count;
			m_move_count ++;
		}
	}
#ifdef SORT_MOVEMENT
	// 注意！！线程不安全
	qsort(m_index, m_move_count, sizeof(CCrazyCatMovement*), MovementCompare);
#endif
	return m_move_count;
}

int CCatcherMoveGenerator::MovementCompare(const void * mv1, const void * mv2 )
{
	const CCrazyCatMovement * _mv1 = *(CCrazyCatMovement **)mv1;
	const CCrazyCatMovement * _mv2 = *(CCrazyCatMovement **)mv2;
	if ( _mv1->m_score < _mv2->m_score) return -1;
	else if (_mv1->m_score > _mv2->m_score) return 1;
	else return 0;
}

CCrazyCatMovement * CCatcherMoveGenerator::GetMovement(JCSIZE index)
{
	JCASSERT(index < m_move_count);
	return m_index[index];
}


#define ENABLE_HASHTAB

//#define BREAK_POINT

#define BREAK_POINT_DEPTH_MAX 10

struct BREAK_POINT_NODE
{
	char col, row;
	bool match;
};


static BREAK_POINT_NODE break_point[BREAK_POINT_DEPTH_MAX] = 
{
	{15,15,false},		//0
	{15,15,false},		//1
	{15,15,false},		//2
	{2,0,false},		//3
	{3,3,false},		//4
	{1,0,false},		//5
	{-1,-1,false},		//6
	{-1,-1,false},		//7
	{-1,-1,false},		//8
	{-1,-1,false},		//9
};

// 猫逃走的距离在评价中的比重
#define DIST_RATE	10
#define MAX_SCORE	((MAX_DISTANCE + 5) * DIST_RATE)
#define MIN_SCORE	0


CRobotCatcher::CRobotCatcher(IRefereeListen * listener)
	: m_referee(listener)
	, m_board(NULL)
	, m_movement(NULL)
{
	m_log= false;
	m_eval = new CCrazyCatEvaluator;
	// 初始化哈希表
	memset(m_hashtab, 0, sizeof(HASHITEM) * HASH_SIZE);
}

CRobotCatcher::~CRobotCatcher(void)
{
	delete m_eval;
	delete m_board;
	delete [] m_movement;
}

bool CRobotCatcher::StartSearch(const CChessBoard * bd0, int depth)
{
	// 复制棋盘
	JCASSERT(m_referee);
	bd0->Dupe(m_board);
	m_node = 0, m_hash_hit=0, m_hash_conflict = 0;
	// 初始化哈希表
	memset(m_hashtab, 0, sizeof(HASHITEM) * HASH_SIZE);
#ifdef BREAK_POINT
	for (int ii = 0; ii< BREAK_POINT_DEPTH_MAX; ++ii)	break_point[ii].match = false;
	break_point[depth +1].match = true;
#endif

	// 调用搜索引擎
	delete [] m_movement;
	m_movement = new CCrazyCatMovement[depth + 1];
	memset(m_movement, 0, sizeof(CCrazyCatMovement) * (depth + 1));

	int alpha = AlphaBetaSearch(depth, MIN_SCORE-1, MAX_SCORE+1, m_movement);

	LOG_DEBUG(_T("find movement: "));
	// output movement recorder
	for (int ii = depth; ii >= 0; -- ii)
	{
		CCrazyCatMovement * mm = m_movement + ii;
		LOG_NOTICE(_T("\t%d:%d->(%d,%d):%d"), ii, mm->m_player, mm->m_col, mm->m_row, mm->m_score);
	}
	if (alpha >= MAX_DISTANCE * DIST_RATE)
	{
		LOG_NOTICE(_T("catcher win!"));
	}
	else if (alpha <= DIST_RATE)	
	{	//认输
		LOG_NOTICE(_T("catcher lost!"));
		m_movement[depth].m_col = -1, m_movement[depth].m_row = -1, m_movement[depth].m_player = PLAYER_CATCHER;
	}
	m_referee->SearchCompleted(m_movement + depth);

	delete m_board;
	m_board = NULL;
	return true;
}

int CRobotCatcher::AlphaBetaSearch(int depth, int alpha, int beta, CCrazyCatMovement * bestmv)
{
	PLAY_STATUS ss = m_board->Status();
	// 判断是否分出胜负
	if (PS_CATCHER_WIN == ss)	
	{
		return MAX_DISTANCE * DIST_RATE + depth;		// CATCHER胜
	}
	else if (PS_CAT_WIN == ss)	
	{
		return DIST_RATE - depth;
	}

	// 
	UINT hash_key = 0;
	int score = 0;
#ifdef ENABLE_HASHTAB
	hash_key = m_board->MakeHash();
	if (0 == hash_key) LOG_WARNING(_T("hashkey = 0"))
	score = LookUpHashTab(alpha, beta, depth, hash_key, (ss==PS_CATCHER_MOVE)?0:1);	// player 0 for CATCHER
	if (score != INT_MAX)
	{
		m_hash_hit ++;
		return score;
	}
#endif

	if (depth <= 0)
	{	// 已达到叶子节点，评估
		LOG_STACK_PERFORM(_T("#Evaluation"));
		m_eval->Reset(m_board);
		int val = m_eval->StartSearch() * DIST_RATE + depth;
		return val;
	}

	// save best mv
	stdext::auto_array<CCrazyCatMovement> currentmv(depth+1);
	memset(currentmv, 0, sizeof(CCrazyCatMovement) * (depth + 1));

	// 搜索所有走法
	if (PS_CATCHER_MOVE == ss)
	{	// CATCHER下, 取最大值（评价越大，CAT逃出所要的步数越多）
		bool exact = false;
		CCatcherMoveGenerator gen;
		JCSIZE move_count = gen.Generate(m_board);
		for (JCSIZE ii = 0; ii < move_count; ++ii)
		{
			CCrazyCatMovement * mv = gen.GetMovement(ii);		JCASSERT(mv);
			char cc = mv->m_col, rr = mv->m_row;
			bool br = m_board->Move(PLAYER_CATCHER, cc, rr);
			JCASSERT(br);
		//for (char rr = 0; rr<BOARD_SIZE_ROW; ++rr)
		//{
		//	for (char cc = 0; cc < BOARD_SIZE_COL; ++cc)
		//	{
		//		if ( m_board->Move(PLAYER_CATCHER, cc, rr) )
		//		{
					m_node ++;
					LOG_DEBUG_(1, _T("CCH, [%d], moveto (%d,%d), node=%d"), depth, cc, rr, m_node);
#ifdef BREAK_POINT
					BREAK_POINT_NODE &bp = break_point[depth];
					if ( (bp.col < 0 || bp.col == cc) && (bp.row < 0 || bp.row == rr) )
					{
						bp.match = break_point[depth+1].match;
						LOG_DEBUG_(0, _T("pre-move[%d]: CCH->(%d,%d):??, a=%d,b=%d"), depth, cc, rr, alpha, beta);
					}
					else bp.match = false;
					//JCASSERT(bp.match == false);
#endif
					score = AlphaBetaSearch(depth -1, alpha, beta, currentmv);
					br = m_board->Undo();		JCASSERT(br);
					if (score >= beta)
					{	// 剪枝
						EnterHashTab(ET_LOWER, score, depth, hash_key, 0);
						return score;
					}

					if (score > alpha)
					{
						// 保存当前走法
						for (int ii = 0; ii < depth; ++ii)	bestmv[ii] = currentmv[ii];
						bestmv[depth].m_col = cc, bestmv[depth].m_row = rr;
						bestmv[depth].m_player = PLAYER_CATCHER;
						bestmv[depth].m_score = score;

						alpha = score;
						exact = true;
					}
				//}
			//}
		}
		if (exact)	EnterHashTab(ET_EXACT, alpha, depth, hash_key, 0);
		else		EnterHashTab(ET_UPPER, alpha, depth, hash_key, 0);
		return alpha;
	}
	else if (PS_CAT_MOVE == ss)
	{	// CAT走，取最小值
		bool exact = false;
		for (char ww = 0; ww < MAX_MOVEMENT; ++ww)
		{
			char c0, r0, cc, rr;
			m_board->GetCatPosition(c0, r0);
			::Move(c0, r0, ww, cc, rr);
			if ( m_board->Move(PLAYER_CAT, cc, rr) )
			{
				m_node ++;
				LOG_DEBUG_(1, _T("CAT, [%d], moveto (%d,%d), node=%d"), depth, cc, rr, m_node);
#ifdef BREAK_POINT
				BREAK_POINT_NODE &bp = break_point[depth];
				if ( (bp.col < 0 || bp.col == cc) && (bp.row < 0 || bp.row == rr) )
				{
					bp.match = break_point[depth+1].match;
					LOG_DEBUG_(0, _T("pre-move[%d]: CAT->(%d,%d):??, a=%d,b=%d"), depth, cc, rr, alpha, beta);
				}
				else bp.match = false;
				//JCASSERT(bp.match == false);
#endif
				int score = AlphaBetaSearch(depth -1, alpha, beta, currentmv);
				bool br = m_board->Undo();
				JCASSERT(br);
				if ( score <= alpha)
				{	//剪枝
					EnterHashTab(ET_UPPER, score, depth, hash_key, 1);
					return score;
				}

				if ( score < beta)
				{
					// 保存当前走法
					for (int ii = 0; ii < depth; ++ii)	bestmv[ii] = currentmv[ii];
					bestmv[depth].m_col = cc, bestmv[depth].m_row = rr;
					bestmv[depth].m_player = PLAYER_CATCHER;
					bestmv[depth].m_score = score;

					beta = score;
					exact = true;
				}
			}
		}
		if (exact)	EnterHashTab(ET_EXACT, beta, depth, hash_key, 1);
		else		EnterHashTab(ET_LOWER, beta, depth, hash_key, 1);

		return beta;
	}

	JCASSERT(0);
	return alpha;
}

int CRobotCatcher::LookUpHashTab(int alpha, int beta, int depth, UINT key, int player)
{
	UINT index = (key & 0xFFFE) | player;
	HASHITEM & hash = m_hashtab[index];
	// 哈希未命中（不同的棋盘哈希）
	if (hash.m_checksum != key)		return INT_MAX;

	if (hash.m_depth >= depth)
	{
		switch (hash.m_entry)
		{
		case ET_EXACT:	return hash.m_score;
		case ET_LOWER:	// 棋盘的实际值 > m_score
			if (hash.m_score >= beta) 
			{
				//m_hash_hit++; 
				return hash.m_score;
			}
			else	break;
		case ET_UPPER:	// 棋盘的实际值 < m_score
			if (hash.m_score <= alpha) 
			{
				//m_hash_hit++; 
				return hash.m_score;
			}
			else	break;
		}
	}
	return INT_MAX;
}

void CRobotCatcher::EnterHashTab(ENTRY_TYPE entry, int score, int depth, UINT key, int player)
{
	UINT index = (key & 0xFFFE) | player;
	HASHITEM & hash = m_hashtab[index];
	if (hash.m_checksum == key)
	{	// 相同的局面，覆盖原来值
		if (entry == ET_EXACT)
		{
			hash.m_depth = depth;
			hash.m_score = score;
			hash.m_entry = entry;	
		}
	}
	else
	{
		if (hash.m_checksum != 0)
		{	// 哈希冲突，覆盖原来得值
			m_hash_conflict ++;
			if ( (entry == ET_EXACT) && (hash.m_entry != ET_EXACT) )
			{	// 更新为精确值
				hash.m_checksum = key;
				hash.m_depth = depth;
				hash.m_score = score;
				hash.m_entry = entry;	
			}
		}
		else
		{	// 没有冲突
			hash.m_checksum = key;
			hash.m_depth = depth;
			hash.m_score = score;
			hash.m_entry = entry;	
		}
	}
}
