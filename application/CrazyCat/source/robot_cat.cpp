#include "StdAfx.h"
#include "robot_cat.h"
#include "CrazyCatEvaluator.h"
#include <stdext.h>


void CreateRobotCat(IRefereeListen * listener, IRobot * & robot)
{
	JCASSERT(robot == NULL);
	CRobotCat * _r = new CRobotCat(listener);
	DWORD tid = 0;
	CreateThread(
		NULL, 1024 * 1024, 
		CRobotCat::StaticRun, 
		(LPVOID)(_r), 
		0, &tid);
	robot = _r;
}

CRobotCat::CRobotCat(IRefereeListen * listener)
	: m_referee(listener)
{
	// 
}

CRobotCat::~CRobotCat(void)
{
}

DWORD CRobotCat::StaticRun(LPVOID param)
{
	CRobotCat * robot = reinterpret_cast<CRobotCat*>(param);
	DWORD dr = robot->Run();
	return dr;
}

DWORD CRobotCat::Run(void)
{
	return 0;
}

bool CRobotCat::InternalSearch(const CChessBoard * board, int depth)
{
	return false;
}

//bool CRobotCat::SetBoard(CChessBoard * board) {}

// 搜索下一步棋
bool CRobotCat::StartSearch(const CChessBoard * board, int depth)
{
	JCASSERT(board);
	stdext::auto_ptr<CCrazyCatEvaluator> eval(new CCrazyCatEvaluator(board));
	int ir = eval->StartSearch();
	if (ir == INT_MAX)
	{	// 认输
		CCrazyCatMovement mv(-1, -1);
		if (m_referee)	m_referee->SearchCompleted(&mv);
		return true;
	}

	// 产生走法
	CSearchNode * node = eval->GetSuccess();
	CSearchNode * head = eval->GetHead();

	while (node && (node->m_father != head) ) node = node->m_father;
	JCASSERT(node->m_father == head);
	// 找到走法
	CCrazyCatMovement mv(node->m_cat_col, node->m_cat_row);
	m_referee->SearchCompleted(&mv);
	return true;
}

	//
	//virtual bool GetMove(char & col, char & row);
