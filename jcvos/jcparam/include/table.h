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

		virtual void ToStream(void * row, IJCStream * stream, jcparam::VAL_FORMAT fmt) const
		{
			JCASSERT(stream);
			CJCStringT str;
			VAL_TYPE * val = reinterpret_cast<VAL_TYPE*>((BYTE*)row + m_offset);
			CONVERTOR::T2S(*val, str);
			stream->Put(str.c_str(), str.length() );
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

		virtual void ToStream(void * row, IJCStream * stream, jcparam::VAL_FORMAT fmt) const
		{
			JCASSERT(stream);
			CJCStringT * str = (reinterpret_cast<CJCStringT*>((BYTE*)row + m_offset));
			stream->Put(str->c_str(), str->length() );
		}
		virtual void CreateValue(BYTE * src, IValue * & val) const
		{
			JCASSERT(NULL == val);
			BYTE * p = src + m_offset;
			val = CTypedValue<CJCStringT>::Create(*(reinterpret_cast<CJCStringT*>(p)));
		}
	};	

};