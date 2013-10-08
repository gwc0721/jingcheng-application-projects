#include "../include/MySQLClient.h"
#include "dbexcep.h"
#include "MySQLStoreRes.h"


LOCAL_LOGGER_ENABLE(_T("JcdbMySQL"), LOGGER_LEVEL_DEBUGINFO);

using namespace jcdb;

CMySQLClient::CMySQLClient(void)
	: m_mysql(NULL)
	//, m_ref(1)
{
	LOG_DEBUG(_T("Constructed. pointer = 0x%08X"), (DWORD)this);
}

CMySQLClient::~CMySQLClient(void)
{
	Disconnect();
	LOG_DEBUG(_T("Deconstructed. pointer = 0x%08X"), (DWORD)((DWORD_PTR)this));
}

//void CMySQLClient::Connect(
//	LPCTSTR strServer, UINT iPort, 
//	LPCTSTR strDataBase, LPCTSTR strUserName, LPCTSTR strPassword)
//{
//	if (strServer)		m_str_host = CW2A(strServer, CP_UTF8);
//	else				m_str_host = "localhost";
//
//	m_str_user = CW2A(strUserName, CP_UTF8);
//	m_str_password = CW2A(strPassword, CP_UTF8);
//	m_str_db = CW2A(strDataBase, CP_UTF8);
//	m_i_port = iPort;
//
//	ConnectA(m_str_host, m_i_port, m_str_db, m_str_user, m_str_password);
//}

void CMySQLClient::Connect(LPCSTR host, UINT port, LPCSTR db, LPCSTR user, LPCSTR password)
{
	MYSQL * mysql = mysql_init(NULL);
	if (!mysql)	THROW_DB_ERR("Failed initializing MySQL!");

	char charset[] = "utf8";
	int ir = mysql_options(mysql, MYSQL_SET_CHARSET_NAME, charset);
	if (ir) THROW_MYSQL_ERR(mysql, "Set character set error!");

	MYSQL * r = mysql_real_connect(mysql, host, user, password, db, port, NULL, 0);
	if (!r) THROW_MYSQL_ERR(mysql, "");
	m_mysql = mysql;
}


void CMySQLClient::Reconnect(void)
{
}

void CMySQLClient::Disconnect(void)
{
	if (m_mysql) mysql_close(m_mysql); 
	m_mysql = NULL;
}

void CMySQLClient::Query(LPCTSTR strSQL, IResultSet * & result_set)
{
	JCASSERT(m_mysql);
	JCASSERT(NULL == result_set);

}

void CMySQLClient::Query(LPCSTR str_sql, IResultSet * &result_set)
{
	JCASSERT(m_mysql);
	JCASSERT(NULL == result_set);
	// issue query command
	int ir = mysql_real_query(m_mysql, str_sql, (unsigned long)strlen(str_sql));
	if (ir) THROW_MYSQL_ERR(m_mysql, "");
	// retrieve result set
	MYSQL_RES * mysql_result = mysql_store_result(m_mysql);	
	if (!mysql_result)	THROW_MYSQL_ERR(m_mysql, "");

	CMySQLStoreRes * resset = new CMySQLStoreRes(mysql_result);

	result_set = static_cast<IResultSet*>(resset);

	// get field info
	//return resset;
}

LONGLONG CMySQLClient::Execute(LPCWSTR str_sql)
{
	//JCASSERT(m_mysql);
	//CW2A _sql(str_sql, CP_UTF8);
	//return Execute((LPCSTR)_sql);
	return NULL;
}

LONGLONG CMySQLClient::Execute(LPCSTR str_sql)
{
	JCASSERT(m_mysql);
	int ir = mysql_real_query(m_mysql, str_sql, (unsigned long)strlen(str_sql));
	if (ir) THROW_MYSQL_ERR(m_mysql, "");
	return (LONGLONG)mysql_affected_rows(m_mysql);
}

LONGLONG CMySQLClient::GetAutoIncreaseID(void)
{
	JCASSERT(m_mysql);
	return (LONGLONG)mysql_insert_id(m_mysql);
}