#pragma once

#include "../include/iplugin.h"

#define JCIFBASE

#define LOG_SCRIPT(fmt, ...)		{\
	TCHAR str[128];	\
	stdext::jc_sprintf(str, fmt, __VA_ARGS__); \
	LOG_NOTICE( _T("+ %s [%08X] %s"), \
	m_name, (UINT)(static_cast<IAtomOperate*>(this)), str);	\
	}

#define LOG_SCRIPT_OUT(fmt, ...)		{\
	TCHAR str[128];	\
	stdext::jc_sprintf(str, fmt, __VA_ARGS__); \
	LOG_NOTICE( _T("- %s [%08X] %s"), \
	m_name, (UINT)(static_cast<IAtomOperate*>(this)), str);	\
	}

#define DUMMY_VALPTR (reinterpret_cast<jcparam::IValue*>(0xFFFFFFFF))

namespace jcscript
{
///////////////////////////////////////////////////////////////////////////////
// -- debug information support for IAtomOperate
	class IOperator : public IAtomOperate
	{
	public:
		virtual const void * GetValue(void) = 0;
		virtual jcparam::VALUE_TYPE GetValueType(void) = 0;
	};


///////////////////////////////////////////////////////////////////////////////
// -- debug information support for IAtomOperate
	template <class NAME, class INTERFACE>
	class COpSourceSupport0 : public INTERFACE
	{
	public:
		COpSourceSupport0(void) : m_ref(1) {};
		virtual ~COpSourceSupport0(void) {};
		virtual void SetSource(UINT src_id,  IAtomOperate * op)	{ JCASSERT(0); }
		virtual UINT GetProperty(void) const {return 0;};
		virtual UINT GetDependency(void) {return 0;};
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile)
		{
			stdext::jc_fprintf(outfile, _T("%s%s (%d) [%08X], "), indentation, NAME::name, GetDependency(), (UINT)(static_cast<IAtomOperate*>(this)) );
			DebugInfo(outfile);
			stdext::jc_fprintf(outfile, _T("\n") );
		}
		virtual void DebugInfo(FILE * outfile) {}

		inline virtual void AddRef() {++m_ref;}
		inline virtual void Release(void)		{	if ( (--m_ref) == 0) delete this;	}
		inline virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr) {return false;}
	protected:
		long m_ref;
	};

///////////////////////////////////////////////////////////////////////////////
// -- debug information support for IAtomOperate
	template <class INTERFACE, class SRC_TYPE, UINT src_num, class NAME = no_name >
	class COpSourceSupport 
		: public INTERFACE
	{
	public:
		COpSourceSupport(void) : m_dependency(0), m_ref(1)
		{
			memset(m_src, 0, sizeof(SRC_TYPE*) * src_num); 
		}
		virtual ~COpSourceSupport(void) { for (UINT ii = 0; ii < src_num; ++ii) if (m_src[ii]) m_src[ii]->Release(); }

	public:
		virtual void SetSource(UINT src_id,  IAtomOperate * op)
		{
			JCASSERT(src_id < src_num); JCASSERT(op); JCASSERT(NULL == m_src[src_id]);

			m_dependency = max(m_dependency, op->GetDependency());
			op->AddRef();
			m_src[src_id] = dynamic_cast<SRC_TYPE*>(op);
			JCASSERT(m_src[src_id]);
		}
		virtual UINT GetProperty(void) const {return 0;} ;
		virtual UINT GetDependency(void) {return m_dependency;};
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile)
		{
			stdext::jc_fprintf(outfile, _T("%s%s (%d) [%08X], "), 
				indentation, NAME::m_name, 
				GetDependency(), (UINT)(static_cast<INTERFACE*>(this)) );
			for (int ii=0; ii < src_num; ++ii)	stdext::jc_fprintf(outfile, _T("<%08X>, "), (UINT)(static_cast<IAtomOperate*>(m_src[ii])));
			DebugInfo(outfile);
			stdext::jc_fprintf(outfile, _T("\n") );
		}
		virtual void DebugInfo(FILE * outfile) {}

		inline virtual void AddRef() {++m_ref;}
		inline virtual void Release(void)		{	if ( (--m_ref) == 0) delete this;	}
		inline virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr) {return false;}
	protected:
		long m_ref;

	protected:
		UINT m_dependency;
		SRC_TYPE * m_src[src_num];
	};

	template <class SRC_TYPE, UINT scr_num, class NAME = no_name>
	class COperatorBase : public COpSourceSupport<IOperator, SRC_TYPE, scr_num, NAME> {};

	template <class SRC_TYPE, UINT scr_num, class NAME = no_name>
	class CAtomOpBase : public COpSourceSupport<IAtomOperate, SRC_TYPE, scr_num, NAME> {};
};
