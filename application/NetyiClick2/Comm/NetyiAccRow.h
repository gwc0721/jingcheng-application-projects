#pragma once

#include <winsock2.h>
#include <jcdb.h>

class CNetyiAccRow :
	public CRecordRowImpl
{
public:
	CNetyiAccRow(IResultSet * res);
	CNetyiAccRow(CRecordRowImpl * row);
	virtual ~CNetyiAccRow(void);

public:
	enum FIELD_ID {
		FID_USER = 1, FID_PASSWORD, FID_POINT, FID_LAST_LOGIN, FID_TODAY_CLICKED, FID_LAST_CLICKED, FID_MAX
	};

protected:
	static const CDBFieldMap m_field_map;
public:
	static const CDBFieldMap * GetFieldMap(void) {
		return & m_field_map;
	}

public:
	CAtlString		m_user, m_password, m_email;
	int				m_point, m_today_clicked;
	COleDateTime	m_last_login, m_last_clicked;

	//float			m_sort_key;

	//void SetData(const CRecordRowImpl * row);

	//virtual const CDBFieldMap * GetFieldMap(void) const {
	//	return & m_field_map;
	//}
};
