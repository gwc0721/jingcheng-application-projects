#include "stdafx.h"
#include "robot_catcher.h"

#include <stdext.h>

#include <stdlib.h>
#include <search.h>

LOCAL_LOGGER_ENABLE(_T("search_engine"), LOGGER_LEVEL_NOTICE);

#define SORT_MOVEMENT

#define HISOTRY_HEURISTIC

///////////////////////////////////////////////////////////////////////////////
// -- cat movement generator
CCatMoveGenerator::CCatMoveGenerator(const UINT * hist_tab)
: m_history_tab (hist_tab)
{
}

CCatMoveGenerator::~CCatMoveGenerator(void)
{
}

CCrazyCatMovement * CCatMoveGenerator::GetMovement(JCSIZE index)
{
	JCASSERT(index < m_move_count);
	return m_index[index];
}


JCSIZE CCatMoveGenerator::Generate(const CChessBoard * board)
{
	JCASSERT(board);
	memset(m_array, 0, sizeof (CCrazyCatMovement) * MAX_MOVEMENT);
	memset(m_index, 0, sizeof (CCrazyCatMovement *) * MAX_MOVEMENT);

	char cat_c, cat_r;
	board->GetCatPosition(cat_c, cat_r);
	m_move_count = 0;

	for (int ww = 0; ww < MAX_MOVEMENT; ++ ww)
	{
		char cc, rr;
		::Move(cat_c, cat_r, ww, cc, rr);
		if (board->CheckPosition(cc, rr) != 0)	continue;

		CCrazyCatMovement & mv = m_array[m_move_count];
		mv.m_col = cc, mv.m_row = rr, mv.m_player = PLAYER_CAT;
		int r = BOARD_SIZE_COL - cc -1;
		int b = BOARD_SIZE_ROW - rr -1;
#if 0
		mv.m_score = m_history_tab[rr * BOARD_SIZE_COL + cc];
		m_index[m_move_count] = m_array + m_move_count;
#else
		mv.m_score = min( min(cc, r), min(rr, b) );
		if (0 == mv.m_score)
		{	// 如果遇到边，则优先搜索，一边裁减其它走法
			m_index[m_move_count] = m_index[0];
			m_index[0] = m_array + m_move_count;
		}
		else	m_index[m_move_count] = m_array + m_move_count;
#endif		
		m_move_count++;
	}
	// <WARNING> 线程不安全
#if 0
	qsort(m_index, m_move_count, sizeof(CCrazyCatMovement*), CCatcherMoveGenerator::MvCmpRise);
#endif		
	return m_move_count;
}


///////////////////////////////////////////////////////////////////////////////
// -- catcher movement generator
LOG_CLASS_SIZE(CCatcherMoveGenerator)


CCatcherMoveGenerator::CCatcherMoveGenerator(void)
	: m_sort(false)
	, m_history_tab(NULL)
{
}

CCatcherMoveGenerator::CCatcherMoveGenerator(const UINT * hist_tab)
	: m_sort(false)
	, m_history_tab(hist_tab)
{
}

CCatcherMoveGenerator::~CCatcherMoveGenerator(void)
{

}
	
JCSIZE CCatcherMoveGenerator::Generate(const CChessBoard * board)
{
	JCASSERT(board);
	memset(m_array, 0, sizeof (CCrazyCatMovement) * ARRAY_SIZE);
	memset(m_index, 0, sizeof (CCrazyCatMovement *) * ARRAY_SIZE);
#ifdef HISOTRY_HEURISTIC
	JCASSERT(m_history_tab)
#endif
	char cat_c, cat_r;
	board->GetCatPosition(cat_c, cat_r);
	m_move_count = 0;
	for (int rr = 0; rr < BOARD_SIZE_ROW; ++rr)
	{
		int dr = abs(cat_r - rr);
		int dr2 = dr >> 1;
		for (int cc = 0; cc < BOARD_SIZE_COL; ++cc)
		{
			if (board->CheckPosition(cc, rr) != 0) continue;
			int dc = abs(cat_c - cc);
			int dc2 = (dc > dr2) ? dc - dr2 : 0; 
			int dd = dr + dc;

			CCrazyCatMovement & mv = m_array[m_move_count];
			mv.m_col = cc, mv.m_row = rr, mv.m_player = PLAYER_CATCHER;
#ifdef HISOTRY_HEURISTIC
			mv.m_score = m_history_tab[rr * BOARD_SIZE_COL + cc] - dd;
#else
			mv.m_score = abs(cat_c - cc) + delta_r;
#endif
			m_index[m_move_count] = m_array + m_move_count;
			m_move_count ++;
		}
	}
	// <WARNING> 线程不安全
#ifdef HISOTRY_HEURISTIC
	qsort(m_index, m_move_count, sizeof(CCrazyCatMovement*), MvCmpFall);
#else
	if (m_sort)	qsort(m_index, m_move_count, sizeof(CCrazyCatMovement*), MovementCompare);
#endif
	return m_move_count;
}

int CCatcherMoveGenerator::MvCmpRise(const void * mv1, const void * mv2 )
{
	const CCrazyCatMovement * _mv1 = *(CCrazyCatMovement **)mv1;
	const CCrazyCatMovement * _mv2 = *(CCrazyCatMovement **)mv2;
	if ( _mv1->m_score < _mv2->m_score) return -1;
	else if (_mv1->m_score > _mv2->m_score) return 1;
	else return 0;
}

int CCatcherMoveGenerator::MvCmpFall(const void * mv1, const void * mv2 )
{
	const CCrazyCatMovement * _mv1 = *(CCrazyCatMovement **)mv1;
	const CCrazyCatMovement * _mv2 = *(CCrazyCatMovement **)mv2;
	if ( _mv1->m_score > _mv2->m_score) return -1;
	else if (_mv1->m_score < _mv2->m_score) return 1;
	else return 0;
}

CCrazyCatMovement * CCatcherMoveGenerator::GetMovement(JCSIZE index)
{
	JCASSERT(index < m_move_count);
	return m_index[index];
}

///////////////////////////////////////////////////////////////////////////////
// -- 
LOG_CLASS_SIZE(CRobotCatcher)

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
#define MAX_SCORE	((MAX_DISTANCE + 50) * DIST_RATE)
#define MIN_SCORE	0

// 满分218分
const int CRobotCatcher::DSTAB_SIZE = 9;
const int CRobotCatcher::DIST_SCORE_TAB[DSTAB_SIZE] =
//dist:		0, 1, 2, 3, 4, 5, 6, 7, 8, ...
{			8, 8, 4, 3, 2, 1, 1, 1, 1, 	};

CRobotCatcher::CRobotCatcher(IRefereeListen * listener, bool sort)
	: m_referee(listener)
	, m_board(NULL)
	, m_sort(sort)
{
	m_log= false;
	m_eval = new CCrazyCatEvaluator;
	// 初始化哈希表
	memset(m_hashtab, 0, sizeof(HASHITEM) * HASH_SIZE);
	memset(m_history_tab, 0, sizeof(UINT) * BOARD_SIZE_COL * BOARD_SIZE_ROW * 2);
}

CRobotCatcher::~CRobotCatcher(void)
{
	delete m_eval;
	//delete m_board;
}

int CRobotCatcher::Evaluate(CChessBoard * board, CCrazyCatEvaluator * eval)
{
	JCASSERT(eval);
	JCASSERT(board);
	eval->Reset(board);
	// 最短套出距离: 值域：0~40
	int min_dist = eval->StartSearch() * DIST_RATE;
	// CATCHER对CAT的紧密程度
	int round = 0;

	char cat_c, cat_r;
	board->GetCatPosition(cat_c, cat_r);
#if 0
	for (int rr = 0; rr < BOARD_SIZE_ROW; ++rr)
	{
		int delta_r = abs(cat_r - rr);
		for (int cc = 0; cc < BOARD_SIZE_COL; ++cc)
		{
			if (board->CheckPosition(cc, rr) == PLAYER_CATCHER)
			{	// 离CAT越远，分数越低
				round += 16 - (delta_r + abs(cat_c - cc) );
			}
		}
	}
#else
	// 提高距离的计算精度. 一个近似距离的方法：
	//   由于y方向移动两格同时，最多可使x方向移动一格。如果x方向的距离小于y方向距离的一半，侧y方向移动的同时完成x方向的移动。
	//	否则，x方向还需移动y/2以外的步数。
	//  实际上由于奇偶行的坐标不对称，考虑出发点和目的地的行间奇偶性，可能存在正负1步的误差。但是这算法要比考虑奇偶性高效得多。

	for (int rr = 0; rr < BOARD_SIZE_ROW; ++rr)
	{
		int dr = abs(cat_r - rr);
		int dr2 = dr >> 1;	// dr/2
		for (int cc = 0; cc < BOARD_SIZE_COL; ++cc)		if (board->CheckPosition(cc, rr) == PLAYER_CATCHER)
		{
			int dc = abs(cat_c - cc);
			int dc2 = (dc > dr2) ? dc - dr2 : 0; 
			int dd = dr + dc;
			// 距离超过DSTAB_SIZE的得分为零
			if (dd < DSTAB_SIZE)	round += DIST_SCORE_TAB[dd];
		}
	}
#endif
	return (min_dist + round);
}

bool CRobotCatcher::StartSearch(CChessBoard * board, int depth)
{
	// 复制棋盘
	JCASSERT(m_referee);
	JCASSERT(depth < MAX_DEPTH);

	//bd0->Dupe(m_board);
	m_board = board;
	InterlockedExchange(&m_progress, 0);
	InterlockedExchange(&m_max_prog, BOARD_SIZE_ROW * BOARD_SIZE_COL);
	m_node = 0, m_hash_hit=0, m_hash_conflict = 0;
	m_init_depth = depth;
	InterlockedExchange(&m_terminate, 0);

	// 初始化哈希表
	memset(m_hashtab, 0, sizeof(HASHITEM) * HASH_SIZE);
#ifdef BREAK_POINT
	for (int ii = 0; ii< BREAK_POINT_DEPTH_MAX; ++ii)	break_point[ii].match = false;
	break_point[depth +1].match = true;
#endif
	// 
	memset(m_movement, 0, sizeof(CCrazyCatMovement) * MAX_DEPTH);
	// 初始化历史启发表
	//memset(m_history_tab, 0, sizeof(UINT) * BOARD_SIZE_ROW * BOARD_SIZE_COL);

	double search_time = 0;
	int alpha = 0;
	CJCStackPerformance perf (_T(""));
	// 调用搜索引擎
	alpha = AlphaBetaSearch(depth, MIN_SCORE-1, MAX_SCORE+1, m_movement);
	search_time = perf.GetDeltaTime();

	InterlockedExchange(&m_progress, m_max_prog);

	LOG_DEBUG(_T("find movement: "));
	// output movement recorder
	for (int ii = depth; ii >= 0; -- ii)
	{
		CCrazyCatMovement * mm = m_movement + ii;
		if (mm->m_score < 0)	{ LOG_NOTICE(_T("\t%d:%s lost"), ii, CChessBoard::PLAYER_SNAME[mm->m_player]) }
		else {
			LOG_NOTICE(_T("\t%d:%s->(%d,%d):%d"), ii, 
				CChessBoard::PLAYER_SNAME[mm->m_player], mm->m_col, mm->m_row, mm->m_score);
		}
	}
	if (alpha >= MAX_DISTANCE * DIST_RATE)
	{
		LOG_NOTICE(_T("catcher win!"));
	}
	else if (alpha <= DIST_RATE)	
	{	//认输
		LOG_NOTICE(_T("catcher lost!"));
		JCASSERT(m_movement[depth].m_player > 0 && m_movement[depth].m_score > 0);
		//m_movement[depth].m_col = -1, m_movement[depth].m_row = -1, m_movement[depth].m_player = PLAYER_CATCHER;
	}

	LOG_NOTICE( _T("node: % 8d, search time: %.1f ms"), m_node, search_time / 1000);
	LOG_DEBUG(_T("hash hit: % 6d, confilict: %6d"), m_hash_hit, m_hash_conflict);
	m_referee->SearchCompleted(m_movement + depth);

	//delete m_board;
	m_board = NULL;
	return true;
}

int CRobotCatcher::AlphaBetaSearch(int depth, int alpha, int beta, CCrazyCatMovement * bestmv)
{
	if (m_terminate) throw 0;
	PLAYER turn = m_board->GetTurn();
	// 判断是否分出胜负
	if ( m_board->IsWinPlayer(turn) )
	{
		bestmv[depth].m_player = turn;
		bestmv[depth].m_score = -1;
		// cat win
		if (turn == PLAYER_CATCHER)	return DIST_RATE - depth;
		// catcher win
		else						return MAX_DISTANCE * DIST_RATE + depth;
	}

	// 
	UINT hash_key = 0;
	int score = 0;
#ifdef ENABLE_HASHTAB
	hash_key = m_board->MakeHash();
	if (0 == hash_key) LOG_WARNING(_T("hashkey = 0"))
	score = LookUpHashTab(alpha, beta, depth, hash_key, (turn-1) );	// player 0 for CATCHER
	if (score != INT_MAX)
	{
		m_hash_hit ++;
		return score;
	}
#endif

	// 已达到叶子节点，评估
	if (depth <= 0)	return Evaluate(m_board, m_eval);

	// save best mv
	CCrazyCatMovement * currentmv = m_stack[depth].m_mv_track;
	memset(currentmv, 0, sizeof(CCrazyCatMovement) * (depth + 1));

	// 搜索所有走法
	if (PLAYER_CATCHER == turn)
	{	// CATCHER下, 取最大值（评价越大，CAT逃出所要的步数越多）
		bool exact = false;
		CCatcherMoveGenerator gen(m_history_tab);
		gen.m_sort = m_sort;
		JCSIZE move_count = gen.Generate(m_board);
		if (depth == m_init_depth) InterlockedExchange(&m_max_prog, move_count);
		for (JCSIZE ii = 0; ii < move_count; ++ii)
		{
			CCrazyCatMovement * mv = gen.GetMovement(ii);		JCASSERT(mv);
			char cc = mv->m_col, rr = mv->m_row;
			bool br = m_board->Move(mv);		JCASSERT(br);

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
			if ( depth == m_init_depth) InterlockedIncrement(&m_progress);

			br = m_board->Undo();		JCASSERT(br);
			if (score >= beta)
			{	// 剪枝
				EnterHashTab(ET_LOWER, score, depth, hash_key, 0);
				EnterHistoryTab(*mv, depth);
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
		}
		if (exact)	EnterHashTab(ET_EXACT, alpha, depth, hash_key, 0);
		else		EnterHashTab(ET_UPPER, alpha, depth, hash_key, 0);
		EnterHistoryTab(bestmv[depth], depth);
		return alpha;
	}
	else/* if (PS_CAT_MOVE == ss)*/
	{	// CAT走，取最小值
		bool exact = false;
		CCatMoveGenerator gen(m_history_tab + HH_SIZE);
		JCSIZE move_count = gen.Generate(m_board);
		for (JCSIZE ii = 0; ii < move_count; ++ii)
		{
			CCrazyCatMovement * mv = gen.GetMovement(ii);		JCASSERT(mv);
			char cc = mv->m_col, rr = mv->m_row;

			m_board->Move( mv );
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
				EnterHistoryTab(*mv, depth);
				return score;
			}

			if ( score < beta)
			{
				// 保存当前走法
				for (int ii = 0; ii < depth; ++ii)	bestmv[ii] = currentmv[ii];
				bestmv[depth].m_col = cc, bestmv[depth].m_row = rr;
				bestmv[depth].m_player = PLAYER_CAT;
				bestmv[depth].m_score = score;

				beta = score;
				exact = true;
			}
		}
		if (exact)	EnterHashTab(ET_EXACT, beta, depth, hash_key, 1);
		else		EnterHashTab(ET_LOWER, beta, depth, hash_key, 1);
		EnterHistoryTab(bestmv[depth], depth);
		return beta;
	}

	JCASSERT(0);
	return alpha;
}

int CRobotCatcher::LookUpHashTab(int alpha, int beta, int depth, UINT key, int player)
{
	UINT index = (key & HASH_SIZE_MASK) | player;
	HASHITEM & hash = m_hashtab[index];
	// 哈希未命中（不同的棋盘哈希）
	if (hash.m_checksum != key)		return INT_MAX;

	if (hash.m_depth >= depth)
	{
		switch (hash.m_entry)
		{
		case ET_EXACT:	return hash.m_score;
		case ET_LOWER:	// 棋盘的实际值 > m_score
			if (hash.m_score >= beta) 	return hash.m_score;
			else	break;
		case ET_UPPER:	// 棋盘的实际值 < m_score
			if (hash.m_score <= alpha) 	return hash.m_score;
			else	break;
		}
	}
	return INT_MAX;
}

void CRobotCatcher::EnterHashTab(ENTRY_TYPE entry, int score, int depth, UINT key, int player)
{
	UINT index = (key & HASH_SIZE_MASK) | player;
	HASHITEM & hash = m_hashtab[index];
	if (hash.m_checksum == key)
	{	// 相同的局面，覆盖原来值
		if (entry == ET_EXACT)
		{
			hash.m_depth = depth, hash.m_score = score, hash.m_entry = entry;	
		}
	}
	else
	{
		if (hash.m_checksum != 0)
		{	// 哈希冲突，覆盖原来得值
			m_hash_conflict ++;
			if ( (entry == ET_EXACT) && (hash.m_entry != ET_EXACT) )
			{	// 更新为精确值
				hash.m_checksum = key,	hash.m_depth = depth, hash.m_score = score;
				hash.m_entry = entry;	
			}
		}
		else
		{	// 没有冲突
			hash.m_checksum = key,			hash.m_depth = depth;
			hash.m_score = score,			hash.m_entry = entry;	
		}
	}
}

void CRobotCatcher::EnterHistoryTab(const CCrazyCatMovement &mv, int depth)
{
	if (mv.m_player != 0)
	{	// player = 0时mv无效
		m_history_tab[HH_SIZE * (mv.m_player-1) + mv.m_row * BOARD_SIZE_COL + mv.m_col] += (2 << depth); 
	}
}
