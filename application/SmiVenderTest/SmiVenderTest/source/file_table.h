#pragma once

#include <jcparam.h>

namespace jcparam
{
	// File Table: 将一个表格和一个csv文件相关联。
	//	csv文件的表头作为表格中列的定义，包括列名称，类型，宽度等
	//	row用来存储从csv中解析的内容。row根据csv表头的内容自适应生成相应的大小
	//	table要求支持大型文件。表中缓存所有行在文件中的位置，以及部分解析后的row
	//		以后考虑file view支持以提高效率。
	class CFileTableRow
		: virtual public ITableRow
		, virtual public IValueFormat
		, public CJCInterfaceBase
	{
	public:
		virtual JCSIZE GetRowID(void) const = 0;
		virtual int GetColumnSize() const = 0;

		// 从一个通用的行中取得通用的列数据
		virtual void GetColumnData(int field, IValue * & val)	const = 0;
		// 通过名称获取列的值
		virtual void GetSubValue(LPCTSTR name, IValue * & val);

		//virtual LPCTSTR GetColumnName(int field_id) const = 0;
		virtual const CColInfoBase * GetColumnInfo(int field) const = 0;
		// 从row的类型创建一个表格
		virtual bool CreateTable(ITable * & tab) = 0;
	};

	class CFileTable 
		: virtual public ITable
		, virtual public IValueFormat
		, public CJCInterfaceBase
	{
	public:
		virtual JCSIZE GetRowSize() const = 0;
		virtual void GetRow(JCSIZE index, IValue * & row) = 0;
		//virtual void NextRow(IValue * & row) = 0;
		virtual JCSIZE GetColumnSize() const = 0;
		virtual void Append(IValue * source) = 0;
		virtual void AddRow(ITableRow * row) = 0;

		virtual const CColInfoBase * GetColumnInfo(int field) const = 0;
	};
};