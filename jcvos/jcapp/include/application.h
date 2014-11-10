#pragma once

#include "../../jcparam/jcparam.h"

#define BEGIN_ARGU_DEF_TABLE()	\
	jcparam::CArguDefList jcapp::AppArguSupport::m_cmd_parser( jcparam::CArguDefList::RULE()	

#define ARGU_DEF_ITEM(name, abbr, type, var, ...)	\
	( new jcparam::CTypedArguDesc<type>(name, abbr, offsetof(_class_name_, var), __VA_ARGS__) )

#define ARGU_DEF_ENUM(name, abbr, e_type, var, ...)	\
	( new jcparam::CTypedArguDesc<e_type, jcapp::CEnumerate<e_type> >(name, abbr, offsetof(_class_name_, var), __VA_ARGS__) )


#define END_ARGU_DEF_TABLE()	);

// etype
#define BEGIN_ENUM_TABLE(e_type)	\
	jcapp::CEnumerate<e_type>::MAP	jcapp::CEnumerate<e_type>::m_string_map;	\
	template <> jcparam::VALUE_TYPE jcparam::type_id<e_type>::id(void)		{return VT_ENUM;}	\
	static jcapp::CEnumerate<e_type> __enum_table_##type = jcapp::CEnumerate<e_type>()

#define END_ENUM_TABLE		;

#define ARGU_SRC_FN		_T("source")
#define ABBR_SRC_FN		_T('i')
#define ARGU_DST_FN		_T("destination")
#define ABBR_DST_FN		_T('o')
#define ARGU_HELP		_T("help")
#define ABBR_HELP		_T('h')

namespace jcapp
{
///////////////////////////////////////////////////////////////////////////////
// -- enum type support
	template <typename ENUM_T>
	class CEnumerate
	{
	public:
		typedef std::map<CJCStringT, ENUM_T>	MAP;
		typedef std::pair<CJCStringT, ENUM_T>	PAIR;
		typedef ENUM_T	E_TYPE;

	public:
		static void T2S(const ENUM_T & t, CJCStringT & str)
		{
			MAP::iterator it = m_string_map.begin();
			MAP::iterator end_it = m_string_map.end();
			for (; it != end_it; ++it)	if (it->second == t) str = it->first, break;
		}
		static void S2T(LPCTSTR str, ENUM_T & t)
		{
			MAP::const_iterator it = m_string_map.find(str);
			if ( it == m_string_map.end() ) t = (ENUM_T) 0;
			else t = it->second;
		}
	public:
		CEnumerate<ENUM_T>() {};
		CEnumerate<ENUM_T>(const CEnumerate<ENUM_T> & t) {};
	public:
		CEnumerate<ENUM_T> & operator () (const CJCStringT & str, ENUM_T t)
		{
			std::pair<MAP::iterator, bool> rs = m_string_map.insert( PAIR(str, t) );
			if (!rs.second) THROW_ERROR(ERR_PARAMETER, _T("Item %s redefined "), str.c_str() );
			return *this;
		}
	protected:
		static MAP	m_string_map;
	};

///////////////////////////////////////////////////////////////////////////////
// -- support argument for application	

#define ARGU_SUPPORT_INFILE		0x80000000
#define ARGU_SUPPORT_OUTFILE	0x40000000
#define ARGU_SUPPORT_HELP		0x20000000

	class AppArguSupport
	{
	public:
		AppArguSupport(DWORD prop = (ARGU_SUPPORT_INFILE | ARGU_SUPPORT_OUTFILE | ARGU_SUPPORT_HELP) );
		static jcparam::CArguDefList m_cmd_parser;
		int Initialize(void) {return 0;};
		bool CmdLineParse(BYTE * base);

		FILE * OpenInputFile(void);
		FILE * OpenOutputFile(void);
		void ArguCleanUp(void);

	public:
		CJCStringT	m_src_fn;
		CJCStringT	m_dst_fn;
		FILE * m_src_file;
		FILE * m_dst_file;

	protected:
		DWORD m_prop;
	};

	class AppNoArguSupport
	{
		int Initialize(void) {return 0;};
		bool CmdLineParse(BYTE * base) {return false;};
		void ArguCleanUp(void) {};
	};

	template <typename ARGU> 
	class CJCAppBase : public ARGU
	{
	public:
		CJCAppBase(DWORD prop = (ARGU_SUPPORT_INFILE | ARGU_SUPPORT_OUTFILE | ARGU_SUPPORT_HELP) ) : ARGU(prop) {};
		void GetAppPath(CJCStringT & path);

	public:
		void CleanUp(void)
		{
			ARGU::ArguCleanUp();
		}
	};

	template <class BASE>
	class CJCApp : public BASE
	{
	public:
		CJCApp(void)
		{
			if (m_app == NULL) m_app = this;
			LOGGER_SELECT_COL( 0
				| CJCLogger::COL_TIME_STAMP
				| CJCLogger::COL_FUNCTION_NAME
				| CJCLogger::COL_REAL_TIME
				);
			LOGGER_CONFIG(_T("jclog.cfg"));
		}
		virtual ~CJCApp(void) {}

	public:
		int Initialize(void);
		void CleanUp(void) { BASE::CleanUp(); }

		template <class APP_TYPE>
		static APP_TYPE* GetApp(void)  { return dynamic_cast<APP_TYPE*>(m_app); }


	protected:
		BASE * m_app;
	};

	template<class BASE>
	int CJCApp<BASE>::Initialize(void)
	{
		BASE::CmdLineParse((BYTE*)(static_cast<BASE*>(this)) );
		return __super::Initialize();
	}

	template<class ARGU>
	void CJCAppBase<ARGU>::GetAppPath(CJCStringT &path)
	{
		TCHAR str[FILENAME_MAX];
		GetModuleFileName(NULL, str, FILENAME_MAX-1);
		LPTSTR _path = _tcsrchr(str, _T('\\'));
		if (_path) _path[0] = 0;
		path = str;
	}

};


