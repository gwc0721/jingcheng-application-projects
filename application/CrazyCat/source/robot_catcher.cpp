#include "StdAfx.h"
#include "robot_catcher.h"

#include <stdext.h>

LOCAL_LOGGER_ENABLE(_T("search_engine"), LOGGER_LEVEL_DEBUGINFO);



CRobotCatcher::CRobotCatcher(IRefereeListen * listener)
	: m_referee(listener)
	, m_board(NULL)
{
}

CRobotCatcher::~CRobotCatcher(void)
{
	delete m_board;
}

bool CRobotCatcher::StartSearch(const CChessBoard * bd0, int depth)
{
	// 复制棋盘
	JCASSERT(m_referee);
	bd0->Dupe(m_board);
	// 初始化哈希表
	memset(m_hashtab, 0, sizeof(HASHITEM) * HASH_SIZE);

	// 调用搜索引擎
	m_node = 0;
	CCrazyCatMovement mv(-1, -1);
	int alpha = AlphaBetaSearch(depth, MIN_SCORE, MAX_SCORE, &mv);
	LOG_DEBUG(_T("find movement: (%d, %d), alpha=%d"), mv.m_col, mv.m_row, alpha);

	JCASSERT(mv.m_col >= 0 && mv.m_row >= 0);
	m_referee->SearchCompleted(&mv);
	

	delete m_board;
	m_board = NULL;
	return true;
}

int CRobotCatcher::AlphaBetaSearch(int depth, int alpha, int beta, CCrazyCatMovement * mv)
{
	JCASSERT(m_board);
	JCASSERT(mv);
	PLAY_STATUS ss = m_board->Status();
	// 判断是否分出胜负

	if (PS_CATCHER_WIN == ss)	return MAX_SCORE;		// CATCHER胜
	else if (PS_CAT_WIN == ss)	return MIN_SCORE;

	// 
	if (depth <= 0)
	{	// 已达到叶子节点，评估
		LOG_STACK_PERFORM(_T("#Evaluation"));
		CCrazyCatEvaluator * eval = new CCrazyCatEvaluator(m_board);
		int val = eval->StartSearch();
		delete eval;
		return val;
	}

	// 搜索所有走法
	if (PS_CATCHER_MOVE == ss)
	{	// CATCHER下, 去最大值（评价越大，CAT逃出所要的步数越多）
		UINT hash_key = m_board->MakeHash();
		if (0 == hash_key) LOG_WARNING(_T("hashkey = 0"));
		int score = LookUpHashTab(alpha, beta, depth, hash_key, 0);	// player 0 for CATCHER
		if (score != INT_MAX) return score;

		bool exact = false;
		for (char rr = 0; rr<BOARD_SIZE_ROW; ++rr)
		{
			for (char cc = 0; cc < BOARD_SIZE_COL; ++cc)
			{
				if ( m_board->Move(PLAYER_CATCHER, cc, rr) )
				{
					m_node ++;
					LOG_DEBUG_(1, _T("CCH, [%d], moveto (%d,%d), node=%d"), depth, cc, rr, m_node);

					score = AlphaBetaSearch(depth -1, alpha, beta, mv);
					//LOG_DEBUG(_T("move, [%03d], CCH, (%d,%d), %d, alpha=%03d")
					//	, depth, cc, rr, score, alpha);
					bool br = m_board->Undo();
					JCASSERT(br);
					//if (!br) LOG_ERROR(_T("undo failed"));
					if (score >= beta)
					{	// 剪枝
						EnterHashTab(ET_LOWER, score, depth, hash_key, 0);
						return score;
					}

					if (score > alpha)
					{
						// 保存当前走法
						mv->m_col = cc, mv->m_row = rr;
						alpha = score;
						exact = true;
						//if (alpha >= beta)
						//{	
						//	return beta;
						//}
					}

				}
			}
		}
		if (exact)	EnterHashTab(ET_EXACT, alpha, depth, hash_key, 0);
		else		EnterHashTab(ET_UPPER, alpha, depth, hash_key, 0);
		return alpha;
	}
	else if (PS_CAT_MOVE == ss)
	{	// CAT走，去最小值
			// Hash查表
		UINT hash_key = m_board->MakeHash();
		if (0 == hash_key) LOG_WARNING(_T("hashkey = 0"));
		int score = LookUpHashTab(alpha, beta, depth, hash_key, 1);	// player 0 for CATCHER
		if (score != INT_MAX) return score;

		bool exact = false;
		for (char ww = 0; ww < MAX_MOVEMENT; ++ww)
		{

			char c0, r0, c1, r1;
			m_board->GetCatPosition(c0, r0);
			::Move(c0, r0, ww, c1, r1);
			if ( m_board->Move(PLAYER_CAT, c1, r1) )
			{
				m_node ++;
				LOG_DEBUG_(1, _T("CAT, [%d], moveto (%d,%d), node=%d"), depth, c1, r1, m_node);
				int score = AlphaBetaSearch(depth -1, alpha, beta, mv);
				//LOG_DEBUG(_T("move, [%03d], CAT, (%d,%d), %d, beta=%03d")
				//	, depth, c1, r1, score, beta);
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
					mv->m_col = c1, mv->m_row = r1;
					beta = score;
					exact = true;
					//if (alpha > beta) return alpha;
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
	if (hash.m_checksum != key)
	{	// 哈希表冲突
		//LOG_WARNING(_T("hash confilicted in searching"));
		return INT_MAX;
	}
	if (hash.m_depth > depth)
	{
		switch (hash.m_entry)
		{
		case ET_EXACT:	return hash.m_score;
		case ET_LOWER:
			if (hash.m_score >= beta) return hash.m_score;
			else	break;
		case ET_UPPER:
			if (hash.m_score <= alpha) return hash.m_score;
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
	{
		if (entry == ET_EXACT)
		{
			hash.m_depth = depth;
			hash.m_score = score;
			hash.m_entry = entry;	
		}
	}
	else
	{
		if (hash.m_checksum != 0/* && hash.m_checksum != key*/)
		{
			LOG_WARNING(_T("!! hash confilicted, key=%08X, sum=%08X, index=%04X"), key, hash.m_checksum, index);
		}
		hash.m_checksum = key;
		hash.m_depth = depth;
		hash.m_score = score;
		hash.m_entry = entry;	
	}
}
