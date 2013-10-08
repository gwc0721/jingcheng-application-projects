#pragma once
#include <treeBasepp.h>

class CNetyiAccRow :
	public CRecordRowImpl
{
public:
	CNetyiAccRow(CRecordRowImpl * row);
	virtual ~CNetyiAccRow(void);

public:
	enum FIELD_ID {
		FID_USER = 1, FID_PASSWORD, FID_POINT, FID_TODAY_CLICKED, FID_CLICKED, FID_MAX
	};

protected:
	static const CDBFieldMap m_field_map;
public:
	static const CDBFieldMap * GetFieldMap(void) {
		return & m_field_map;
	}

public:
	//int			m_id;
	CAtlString		m_user, m_password;
	int			m_clicked, m_today_clicked, m_point;
	float		m_sort_key;

	void SetData(const CRecordRowImpl * row);

	//virtual const CDBFieldMap * GetFieldMap(void) const {
	//	return & m_field_map;
	//}
};
