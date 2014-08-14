#pragma once
#include "ChessBoard.h"


void CreateRobotCat(IRefereeListen * listener, IRobot * & robot);


// 红方AI类
class CRobotCat: public IRobot
{
public:
	friend void CreateRobotCat(IRefereeListen * listener, IRobot * & robot);
public:
	CRobotCat(IRefereeListen * listener);
	virtual ~CRobotCat(void);
public:
	static DWORD WINAPI StaticRun(LPVOID lpParameter);

public:
	//virtual void SetBoard(CChessBoard * board);
	// 搜索下一步棋
	virtual bool StartSearch(CChessBoard * board, int depth);
	virtual void Release(void) { delete this; };
	virtual void GetProgress(long & prog, long & max_prog)
	{	prog = 100, max_prog = 100;		}
	virtual void CancelSearch(void) {}

protected:
	DWORD Run(void);
	bool InternalSearch(const CChessBoard * board, int depth);

protected:
	IRefereeListen	* m_referee;
};
