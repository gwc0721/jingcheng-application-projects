#pragma once


#include "ivalue.h"

#include <vector>
#include <map>

#define EMPTY _T("")

namespace jcparam
{
	// VALUE_TYPE的值表示类型的宽度，用于判断拓展转换的方向
	// bit 7~4表示长度（字节），bit 3~0：类型区分
	enum VALUE_TYPE
	{
		VT_UNKNOW = 0x00,
		VT_BOOL = 1,
		VT_CHAR = 2,		VT_UCHAR = 3,
		VT_SHORT= 4,		VT_USHORT =5,
		VT_INT =  6,		VT_UINT =  7,
		VT_INT64 =8,		VT_UINT64 =9,
		VT_FLOAT =10,		VT_DOUBLE =11,
		VT_STRING =12,		VT_MAXNUM,
		VT_HEX, VT_OTHERS,
	};

	template <typename T>
	class type_id
	{
	public:
		static VALUE_TYPE id();
	};

	IValue * CreateTypedValue(VALUE_TYPE vt, void * data=NULL);
	VALUE_TYPE StringToType(LPCTSTR str);

	// 用于基础类型的IValue对象，用类型和值两个域表示。值域的宽度为4字节，如果超过4字节的类型，则使用指针。
	//class CBasicValue
	//	: virtual public IValue
	//{
	//protected:
	//	VALUE_TYPE	m_type;
	//	union 
	//	{
	//		bool	m_bool;
	//		char	m_char;
	//		BYTE	m_uchar;
	//		short	m_short;
	//		WORD	m_ushort;
	//		int		m_int;
	//		DWORD	m_uint;
	//		
	//	}
	//};

	class ClassNameNull
	{
	public:
		static LPCTSTR classname() {return EMPTY;};
	};

	template <typename DATATYPE, typename CONVERTOR = CConvertor<DATATYPE> >
	class CTypedValue 
		: virtual public IValue
		, virtual public IValueFormat
		, virtual public IValueConvertor
		, public CJCInterfaceBase
		, public CTypedValueBase
	{
	public:
		static CTypedValue * Create()	{return new CTypedValue();};
		static CTypedValue * Create(const DATATYPE &d)	{return new CTypedValue(d);};
	protected:
		CTypedValue(void) {};
		CTypedValue(const DATATYPE &d) : m_val(d) {}
		CTypedValue(const CTypedValue<DATATYPE, CONVERTOR> & p) : m_val(p.m_val) 	{}
		virtual ~CTypedValue(void) {}

	public:
		virtual void GetValueText(CJCStringT & str) const 	
		{
			CONVERTOR::T2S(m_val, str);
		}

		virtual void SetValueText(LPCTSTR str) 
		{
			CONVERTOR::S2T(str, m_val);
		}

		CTypedValue<DATATYPE, CONVERTOR> & operator = (const DATATYPE p)	
		{ 
			m_val = p; return (*this);
		}

		operator DATATYPE & () {	return m_val; };
		operator const DATATYPE & () const { return m_val;}

		CTypedValue<DATATYPE, CONVERTOR> & operator = ( const CTypedValue<DATATYPE, CONVERTOR> & p) 
		{
			m_val = p.m_val; 
			return (*this);
		}
		void SetData(DATATYPE * pdata) 	{ if (pdata) m_val = *pdata; }
	public:
		virtual void WriteHeader(FILE *) {};
		virtual void Format(FILE * file, LPCTSTR format)
		{
			CJCStringT str;
			GetValueText(str);
			stdext::jc_fprintf(file, _T("%s"), str.c_str());
		}

		virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr)
		{
			JCASSERT(NULL == if_ptr);
			bool br = false;
			if ( FastCmpA(IF_NAME_VALUE_FORMAT, if_name) )
			{
				if_ptr = static_cast<IJCInterface*>(this);
				if_ptr->AddRef();
				br = true;
			}
			else br = __super::QueryInterface(if_name, if_ptr);
			return br;
		}
	protected:
		DATATYPE m_val;
	};

	class CParamSet : virtual public IValue, public CJCInterfaceBase
	{
	public:
		typedef std::map<CJCStringT, IValue *>	PARAM_MAP;
		typedef PARAM_MAP::iterator ITERATOR;

	public:
		static void Create(CParamSet * &val) 
		{ 
			JCASSERT(NULL==val);
			val = new CParamSet(); 
		}

	protected:
		CParamSet(void);
		virtual ~CParamSet(void);

	public:
		virtual void GetSubValue(LPCTSTR param_name, IValue * & value);
		virtual void SetSubValue(LPCTSTR name, IValue * val);

		bool InsertValue(const CJCStringT & param_name, IValue* value);
		bool RemoveValue(LPCTSTR param_name);

		// enumator
		ITERATOR Begin(void)	{ return m_param_map.begin(); };
		ITERATOR End(void)		{ return m_param_map.end();	};

	protected:
		PARAM_MAP m_param_map;
	};

	template <typename DATATYPE>
	class CSimpleArray :
		virtual public IValue, public CJCInterfaceBase, public CTypedValueBase
	{
	protected:
		CSimpleArray(JCSIZE count) : m_size(count) { m_array = new DATATYPE[count]; };
		virtual ~CSimpleArray(void) { delete[] m_array; };

	public:
		DATATYPE* Lock(void)	{ return m_array; }
		const DATATYPE* Lock(void) const {return m_array;}
		void Unlock(void) const {};
		operator const DATATYPE * () const {return m_array;}
		JCSIZE GetSize(void) const { return m_size; }

	protected:
		DATATYPE *	 m_array;
		JCSIZE		m_size;
	};

	// 元素不能为NULL
	class CValueArray : virtual public IValue, public CJCInterfaceBase, public CTypedValueBase
	{
	protected:
		typedef std::vector<IValue *> VALUE_VECTOR;
		typedef VALUE_VECTOR::iterator VALUE_ITERATOR;

	public:
		CValueArray(void) {};
		virtual ~CValueArray(void);

	public:
		virtual bool GetValueAt(int index, IValue * &value);
		virtual bool PushBack(IValue * value);
		JCSIZE GetSize()	{ return (JCSIZE)m_value_vector.size(); }

		VALUE_VECTOR	m_value_vector;
	};

	typedef std::pair<CJCStringT, IValue*>	KVP;

	template <typename T>
	inline bool GetSimpleValue(jcparam::IValue * val, T & t)
	{
		CTypedValue<T> * v = dynamic_cast<CTypedValue<T> * >(val);
		if (v)	{ t = (*v); return true; }

		jcparam::IValueConvertor * c = dynamic_cast<jcparam::IValueConvertor *>(val);
		if ( !c)  return false;

		CJCStringT str;
		c->GetValueText(str);
		jcparam::CConvertor<T>::S2T(str.c_str(), t);
		return true;
	}

	template <typename T>
	inline bool GetSubVal(jcparam::IValue * v, LPCTSTR name, T & t)
	{
		JCASSERT(v);
		stdext::auto_interface<jcparam::IValue> sub;
		v->GetSubValue(name, sub);
		if ( !sub ) return false;

		return jcparam::GetSimpleValue<T>(sub, t);
	}
};
