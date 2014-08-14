#pragma once
#include "ChessBoard.h"

enum ENTRY_TYPE
{	
	ET_EXACT,	// 棋盘的实际值 = score
	ET_LOWER,	// 棋盘的实际值 > m_score
	ET_UPPER,	// 棋盘的实际值 < m_score
};

struct HASHITEM
{
	UINT	m_checksum;
	int		m_depth;
	int		m_score;
	ENTRY_TYPE	m_entry;
};


class CGameTreeEngine
{
};

class CCatMoveGenerator
{
public:
	CCatMoveGenerator(const UINT * hist_tab);
	~CCatMoveGenerator(void);

public:
	JCSIZE Generate(const CChessBoard * board);
	CCrazyCatMovement * GetMovement(JCSIZE index);

protected:
	// Move的指针数组，用于排序。
	CCrazyCatMovement * m_index[MAX_MOVEMENT];
	// 移动方法数组
	CCrazyCatMovement m_array[MAX_MOVEMENT];
	JCSIZE	m_move_count;
	const UINT * m_history_tab;
};

// 搜索用堆栈。为避免调用堆栈消耗太大，以及动态分配内存的低效。
struct SEARCH_STACK
{
	CCrazyCatMovement m_mv_track[MAX_DEPTH];
};

class CCatcherMoveGenerator
{
public:
	CCatcherMoveGenerator(void);
	CCatcherMoveGenerator(const UINT * hist_tab);
	~CCatcherMoveGenerator(void);

public:
	JCSIZE Generate(const CChessBoard * board);
	CCrazyCatMovement * GetMovement(JCSIZE index);

	static int MvCmpRise(const void * mv1, const void * mv2 );
	static int MvCmpFall(const void * mv1, const void * mv2 );


public:
	bool m_sort;

protected:
	static const int ARRAY_SIZE = BOARD_SIZE_ROW * BOARD_SIZE_COL;
	// Move的指针数组，用于排序。
	CCrazyCatMovement * m_index[BOARD_SIZE_ROW * BOARD_SIZE_COL];
	// 移动方法数组
	CCrazyCatMovement m_array[BOARD_SIZE_ROW * BOARD_SIZE_COL];

	JCSIZE	m_move_count;
	const UINT * m_history_tab;
};

#define HH_SIZE	(BOARD_SIZE_COL * BOARD_SIZE_ROW)


class CRobotCatcher : public IRobot
	, public CGameTreeEngine
{
public:
	CRobotCatcher(IRefereeListen * listener, bool sort = false);
	virtual ~CRobotCatcher(void);

public:
	// 搜索下一步棋
	virtual bool StartSearch(CChessBoard * board, int depth);
	virtual void Release(void) { delete this; };
	virtual void GetProgress(long & prog, long & max_prog)
	{	prog = m_progress;	max_prog = m_max_prog;	}
	static int Evaluate(CChessBoard * board, CCrazyCatEvaluator * eval);
	virtual void CancelSearch(void)
	{	InterlockedExchange(&m_terminate, 1); }


protected:
	int AlphaBetaSearch(int depth, int alpha, int beta, CCrazyCatMovement * mv);
	int LookUpHashTab(int alpha, int beta, int depth, UINT key, int player);
	void EnterHashTab(ENTRY_TYPE entry, int score, int depth, UINT key, int player);
	void EnterHistoryTab(const CCrazyCatMovement &mv, int depth);

protected:
	IRefereeListen	* m_referee;
	CChessBoard * m_board;
	// 哈希表，前一般用于保存CATCHER，后一半保存CAT
	HASHITEM	m_hashtab[HASH_SIZE];
	// 历史启发表
	UINT		m_history_tab[HH_SIZE * 2];

	CCrazyCatEvaluator * m_eval;

	// 评估函数中，距离和得分换算表；
	static const int DIST_SCORE_TAB[];
	// 换算表大小
	static const int DSTAB_SIZE;

public:
	// 用于算法评估
	JCSIZE m_node, m_hash_hit, m_hash_conflict;
	volatile long m_progress;
	volatile long m_max_prog;
	// 记录走法
	CCrazyCatMovement m_movement[MAX_DEPTH];
	SEARCH_STACK		m_stack[MAX_DEPTH];

protected:
	// for debug
	bool m_log;
	bool m_sort;
	long	m_terminate;
	volatile int m_init_depth;
};
