#pragma once

#include "dbinterface.h"
#include "field_info.h"

namespace jcdb
{
	class CTypedRecordRowBase : virtual public IRecordRow, public CJCInterfaceBase
	{
	public:
		// Data Copy
		CTypedRecordRowBase(/*IResultSet * res*/CDBFieldMap *field_map);
		virtual ~CTypedRecordRowBase(void);

	public:
		virtual int GetItemID(void) const { return m_id; }
		virtual int GetFieldSize(void) const { return m_field_map->GetFieldSize(); }
	
		virtual void GetFieldData(int field, CJCStringT & data) const;
		virtual bool GetFieldData(const char * field_name, CJCStringT & data) const;
		virtual const CJCStringA & GetFieldName(int field_id) const;
		void SetData(const IRecordRow * row);

	protected:
		int					m_id;				// 记录编号，对应数据库的第一列'ID'，这必须是一个整数的，自动递增的，唯一的列
		CDBFieldMap *		m_field_map;
	};
};