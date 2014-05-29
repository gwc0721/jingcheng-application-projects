#pragma once

#include "ivalue.h"

namespace jcparam
{
	template <typename VAL_TYPE, class CONVERTOR = CConvertor<VAL_TYPE> >
	class CTypedColInfo : public COLUMN_INFO_BASE
	{
	public:
		CTypedColInfo (int id, JCSIZE offset, LPCTSTR name)
			: COLUMN_INFO_BASE(id, type_id<VAL_TYPE>::id(), offset, name)
		{	JCASSERT(1);
		};
		virtual void GetText(void * row, CJCStringT & str) const
		{
			VAL_TYPE * val = reinterpret_cast<VAL_TYPE*>((BYTE*)row + m_offset);
			CONVERTOR::T2S(*val, str);
		}
		virtual void CreateValue(BYTE * src, IValue * & val) const
		{
			JCASSERT(NULL == val);
			BYTE * p = src + m_offset;
			VAL_TYPE & vsrc= *((VAL_TYPE*)p);
			val = CTypedValue<VAL_TYPE>::Create(vsrc);
		}
		virtual void GetColVal(BYTE * src, void * val) const
		{
			JCASSERT(src);
			JCASSERT(val);

			BYTE * p = src + m_offset;
			VAL_TYPE * _v = (VAL_TYPE *)(val);
			*_v = *((VAL_TYPE *)p);
		}
	};

	class CStringColInfo : public COLUMN_INFO_BASE
	{
	public:
		CStringColInfo (int id, JCSIZE offset, LPCTSTR name)
			: COLUMN_INFO_BASE(id, VT_STRING, offset, name)
		{};
		virtual void GetText(void * row, CJCStringT & str) const
		{
			str = *(reinterpret_cast<CJCStringT*>((BYTE*)row + m_offset));
		}
		virtual void CreateValue(BYTE * src, IValue * & val) const
		{
			JCASSERT(NULL == val);
			BYTE * p = src + m_offset;
			val = CTypedValue<CJCStringT>::Create(*(reinterpret_cast<CJCStringT*>(p)));
		}
	};	

};