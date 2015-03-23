#pragma once

#include "../local_config.h"
#include "../../jcparam/jcparam.h"

#ifdef WIN32
#pragma comment (lib, "version.lib")
#endif

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
	// {D8AE8880-341F-4e8f-B93D-3A700B5E30AA}
	extern const GUID JCAPP_GUID;

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
#define ARGU_SUPPORT			0x10000000

	class CJCAppBase
	{
	public:
		CJCAppBase(void);
		virtual ~CJCAppBase() {};
		void GetAppPath(CJCStringT & path);
		bool LoadApplicationInfo(void);

		void ShowVersionInfo(FILE * out);
		void ShowAppInfo(FILE * out);
		virtual void ShowHelpInfo(FILE *out) = 0;

		virtual int Initialize(void) = 0;
		virtual int Run(void) = 0;
		virtual void CleanUp(void) = 0;
		virtual LPCTSTR AppDescription(void) const = 0;
		//virtual bool IsShowHelp(void) const = 0;

	protected:
		UINT	m_ver[4];
		CJCStringT	m_product_name;

	public:
		static CJCAppBase * GetApp(void);
	protected:
		static CJCAppBase * m_instance;
	};

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
		//bool	m_show_help;
	};

	class AppNoArguSupport
	{
		int Initialize(void) {return 0;};
		bool CmdLineParse(BYTE * base) {return false;};
		void ArguCleanUp(void) {};
	};


	template <typename ARGU> 
	class CJCAppSupport : public CJCAppBase, public ARGU
	{
	public:
		CJCAppSupport(DWORD prop = (ARGU_SUPPORT | ARGU_SUPPORT_INFILE 
			| ARGU_SUPPORT_OUTFILE | ARGU_SUPPORT_HELP) ) : ARGU(prop) {};
	public:
		virtual void CleanUp(void)
		{
			ARGU::ArguCleanUp();
		}
		virtual void ShowHelpInfo(FILE * out)
		{
			_ftprintf_s(out, _T("\t usage:\n"));
			m_cmd_parser.OutputHelpString(out);
		}
	};

	template <class BASE>
	class CJCApp : public BASE, public CSingleToneBase
	{
	public:
		CJCApp(void)
		{
#if APP_GLOBAL_SINGLE_TONE > 0
			// register app pointer to single tone
			CSingleToneEntry * entry = CSingleToneEntry::Instance();
			CSingleToneBase * ptr = NULL;
			entry->QueryStInstance(GetGuid(), ptr);
			JCASSERT(ptr == NULL);
			entry->RegisterStInstance(GetGuid(), static_cast<CSingleToneBase *>(this) );
#else
			// register app pointer to jcapp base
			JCASSERT(CJCAppBase::m_instance == NULL);
			CJCAppBase::m_instance = static_cast<CJCAppBase *>(this);
#endif

			LOGGER_SELECT_COL( 0
				| CJCLogger::COL_TIME_STAMP
				| CJCLogger::COL_FUNCTION_NAME
				| CJCLogger::COL_REAL_TIME
				);
			LOGGER_CONFIG(BASE::LOG_CONFIG_FN);
		}
		virtual ~CJCApp(void) {}

		// App作为静态变量存在，不需要delete
		virtual void Release(void)	{};
		virtual const GUID & GetGuid(void) const {return JCAPP_GUID;};
		static CJCApp<BASE> * Instance(void)
		{
#if APP_GLOBAL_SINGLE_TONE > 0
		// global single tone support
		static CJCApp<BASE> * instance = NULL;
		if (instance == NULL)	CSingleToneEntry::GetInstance< CJCApp<BASE> >(instance);
		return instance;
#else
 		// local single tone support
		static CJCApp<BASE> instance;
		return & instance;
#endif
		};
		static const GUID & Guid() {return JCAPP_GUID;};

	public:
		int Initialize(void);
		void CleanUp(void) { BASE::CleanUp(); }
		//virtual bool IsShowHelp(void) const {return m_show_help;}
	};


	template<class BASE>
	int CJCApp<BASE>::Initialize(void)
	{
		bool br = BASE::CmdLineParse((BYTE*)(static_cast<BASE*>(this)) );
		return __super::Initialize();
	}

	typedef CJCAppSupport<AppArguSupport>	CJCAppicationSupport;

	int local_main(int argc, _TCHAR* argv[]);
};


