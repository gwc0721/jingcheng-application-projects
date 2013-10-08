#include "dbexcep.h"

using namespace jcdb;

CMySQLException::CMySQLException(MYSQL * lpMySQL, LPCSTR msg, ...)
	: CJcdbException("", (int)CJcdbException::DBERR_MYSQL)
{
    va_list argptr;
    va_start(argptr, msg);
	LPSTR str = new char[512];
	vsprintf_s(str, 512, msg, argptr);

	m_err_msg += mysql_error(lpMySQL);
	m_err_msg += str;

	delete [] str;
}

