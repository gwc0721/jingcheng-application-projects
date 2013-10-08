#pragma once

#include "../include/dbinterface.h"
#include "../include/field_info.h"
#include <mysql.h>

namespace jcdb
{
	class CMySQLRow : virtual public IRecordRow, public CJCInterfaceBase
	{
	public:
		CMySQLRow(CDBFieldMap * field, MYSQL_ROW row, unsigned long * lengths);
		~CMySQLRow();

	public:
		virtual int GetItemID(void) const { return m_id; };

		virtual int GetFieldSize() const { return m_field_map->GetFieldSize(); }
		virtual DWORD GetFieldLen(int field_id) const
		{ 
			JCASSERT(field_id < GetFieldSize());
			return m_lengths[field_id];
		}

		// 从一个通用的行中取得通用的列数据
		virtual void GetFieldData(int field, CJCStringT & data)	const;
		
		virtual bool GetFieldData(const char * field_name, CJCStringT & data) const;

		virtual const CJCStringA & GetFieldName(int field_id) const;


	//	virtual const CFieldInfo * GetFieldInfo() const = 0;

	//	virtual void GetFieldName(int field_num, CAtlString & name)/* const*/ = 0;

		// 从一个类型化的行得到列的文本数据
		//virtual void GetFieldDataText(int field_num, CAtlStringW & data)/* const*/ = 0;
		//virtual void GetFieldDataText(int field_num, CJCStringT & data)/* const*/ = 0;
		//virtual void SetFieldDataText(int field_num, LPCSTR data) = 0;
		//virtual /*const*/ IResultSet * GetResultSet(void) const = 0;


	protected:
		MYSQL_ROW			m_row;
		unsigned long *		m_lengths;
		int					m_id;
		CDBFieldMap *		m_field_map;
	};
};