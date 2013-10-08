#pragma once

#include "table.h"


namespace jcparam
{

	// this class can support a very huge table, the data is saved in disk

	class CHugeTable : virtual public ITable, public CJCInterfaceBase
	{
	protected:
		CHugeTable(JCSIZE row_struct_size, JCSIZE rows);
		virtual ~CHugeTable(void);
		
	public:
		virtual void GetValueText(CJCStringT & str) const {};
		virtual void SetValueText(LPCTSTR str)  {} ;

		// 列存取
		virtual void GetSubValue(LPCTSTR name, IValue * & val);

		// 如果name不存在，则插入，否则修改name的值
		virtual void SetSubValue(LPCTSTR name, IValue * val);

		virtual JCSIZE GetRowSize() const;

		virtual void GetRow(JCSIZE index, IValue * & ptr_row);

		virtual JCSIZE GetColumnSize() const;

		virtual void Format(FILE * file, LPCTSTR format);

		virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr);

		virtual void Append(IValue * source);
		//virtual void push_back(const ROW_BASE_TYPE & row_base);

	protected:
		JCSIZE	m_row_struct_size;		// 一行data所占空间大小，BYTE
		JCSIZE  m_row_size;				// 总行数
		// 在内存窗口中的起始行，总行数
		
		
	};
};