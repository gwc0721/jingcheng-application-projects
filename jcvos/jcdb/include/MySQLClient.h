#pragma once
#include "../include/dbinterface.h"
#include <mysql.h>

#pragma warning ( disable : 4250)

//template class __declspec(dllexport) CStringT< wchar_t, StrTraitATL< wchar_t, ChTraitsCRT< wchar_t > > >;
//template class __declspec(dllexport) CStringT< char, StrTraitATL< char, ChTraitsCRT< char > > >;

namespace jcdb
{
	class /*_TREEBASEPP_EXT_CLASS*/ CMySQLClient 
		: virtual public IDataBaseDriver
		, public CJCInterfaceBase
	{
	public:
		CMySQLClient(void);
		virtual ~CMySQLClient(void);

	protected:
		//__declspec(align(4))	LONG	m_ref;
		//CAtlStringA		m_str_host, m_str_db, m_str_user, m_str_password;
		UINT			m_i_port;
		MYSQL			* m_mysql;

	public:
		//inline virtual void AddRef()			{
		//	InterlockedIncrement(&m_ref);
		//	//LOG(_T("CMySQLClient"), _T("addref"), CDataLog::LL_DEBUG, 
		//	//	_T("pointer = 0x%08X, ref = %d"), (DWORD)((DWORD_PTR)this), m_ref);
		//}
		//inline virtual void Release(void)		{
		//	InterlockedDecrement(&m_ref);
		//	//LOG(_T("CMySQLClient"), _T("release"), CDataLog::LL_DEBUG, 
		//	//	_T("pointer = 0x%08X, ref = %d"), (DWORD)((DWORD_PTR)this), m_ref);
		//	if ( m_ref == 0) delete this;
		//}
		virtual void Connect(LPCSTR host, UINT port, LPCSTR db, LPCSTR user, LPCSTR password);

		//virtual void Connect(
		//	LPCTSTR strServer, UINT iPort, 
		//	LPCTSTR strDataBase, LPCTSTR strUserName, LPCTSTR strPassword);
		virtual void Disconnect(void);
		virtual void Reconnect(void);

		virtual void Query(LPCTSTR strSQL, IResultSet * &);
		virtual void Query(LPCSTR strSQL, IResultSet * &);

		virtual LONGLONG Execute(LPCWSTR strSQL);
		virtual LONGLONG Execute(LPCSTR strSQL);
		virtual LONGLONG GetAutoIncreaseID(void);

		virtual BOOL IsConnected(void) {
			return m_mysql != NULL;
		}

		virtual bool Insert(IRecordRow * row) {return false;};
		virtual bool Update(IRecordRow * row) {return false;};
		virtual bool Delete(IRecordRow * row) {return false;};

	};
};
