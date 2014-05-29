#pragma once


#include "ivalue.h"

#include <vector>
#include <map>

#define EMPTY _T("")

namespace jcparam
{
	template <typename T>
	class type_id
	{
	public:
		static VALUE_TYPE id();
	};

	IValue * CreateTypedValue(VALUE_TYPE vt, void * data=NULL);
	VALUE_TYPE StringToType(LPCTSTR str);

	class ClassNameNull
	{
	public:
		static LPCTSTR classname() {return EMPTY;};
	};

	template <typename DATATYPE, typename CONVERTOR = CConvertor<DATATYPE> >
	class CTypedValue 
		: virtual public IValueFormat
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

		virtual jcparam::VALUE_TYPE GetType(void) const
		{
			return type_id<DATATYPE>::id();
		}

		virtual const void * GetData(void) const
		{
			return (void*)(&m_val);
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
		virtual void GetValueText(CJCStringT & str) const {};
		virtual void SetValueText(LPCTSTR str)  {};

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

	typedef std::vector<IValue *> VALUE_VECTOR;
	typedef VALUE_VECTOR::iterator VALUE_ITERATOR;

	class CVector
		: virtual public IVector
		, virtual public IValueFormat
		, public CJCInterfaceBase
	{
	public:
		CVector(void);
		~CVector(void);

	public:
		virtual void GetValueText(CJCStringT & str) const {};
		virtual void SetValueText(LPCTSTR str)  {};
	
		virtual void PushBack(IValue * val);
		virtual void GetRow(JCSIZE index, IValue * & val);
		virtual JCSIZE GetRowSize() const;

		virtual void Format(FILE * file, LPCTSTR format);
		virtual void WriteHeader(FILE * file);
	public:
		virtual void GetSubValue(LPCTSTR name, IValue * & val) {};
		virtual void SetSubValue(LPCTSTR name, IValue * val) {};

	protected:
		VALUE_VECTOR m_vector;
	};


	typedef std::pair<CJCStringT, IValue*>	KVP;

	template <typename T>
	inline bool GetSimpleValue(jcparam::IValue * val, T & t)
	{
		CTypedValue<T> * v = dynamic_cast<CTypedValue<T> * >(val);
		if (v)	{ t = (*v); return true; }

		//jcparam::IValueConvertor * c = dynamic_cast<jcparam::IValueConvertor *>(val);
		//if ( !c)  return false;

		CJCStringT str;
		val->GetValueText(str);
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

	template <typename T>
	inline void GetVal(IValue * val, T & t)
	{
		//IValueConvertor * conv = dynamic_cast<IValueConvertor *>(val);
		//if (NULL == conv) THROW_ERROR(ERR_APP, _T("ivalue do not support convertor"));
		CJCStringT str;
		val->GetValueText(str);
		CConvertor<T>::S2T(str.c_str(), t);
	}
};
