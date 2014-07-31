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
	virtual bool StartSearch(const CChessBoard * board, int depth);
	//
	//virtual bool GetMove(char & col, char & row);

protected:
	DWORD Run(void);
	bool InternalSearch(const CChessBoard * board, int depth);

protected:
	IRefereeListen	* m_referee;
	// 需要线程同步
	//const CChessBoard * m_board;

};
