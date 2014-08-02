#pragma once
#include "ChessBoard.h"

enum ENTRY_TYPE
{	
	ET_EXACT, ET_LOWER, ET_UPPER,
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



class CRobotCatcher : public IRobot
	, public CGameTreeEngine
{
public:
	CRobotCatcher(IRefereeListen * listener);
	virtual ~CRobotCatcher(void);

public:
	// 搜索下一步棋
	virtual bool StartSearch(const CChessBoard * board, int depth);

protected:
	int AlphaBetaSearch(int depth, int alpha, int beta, CCrazyCatMovement * mv);
	int LookUpHashTab(int alpha, int beta, int depth, UINT key, int player);
	void EnterHashTab(ENTRY_TYPE entry, int score, int depth, UINT key, int player);

protected:
	IRefereeListen	* m_referee;
	CChessBoard * m_board;
	// 哈希表，前一般用于保存CATCHER，后一半保存CAT
	HASHITEM	m_hashtab[HASH_SIZE];

public:
	// 用于算法评估
	UINT m_node;



};
