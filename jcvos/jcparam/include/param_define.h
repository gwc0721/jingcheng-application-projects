#pragma once

#include "../../stdext/stdext.h"
#include "value.h"
#include <vector>
#include <map>

namespace jcparam
{
	class CArguSet : public CParamSet
	{
	public:
		template <typename TYPE>
		bool GetValT(LPCTSTR arg, TYPE & val)
		{
			stdext::auto_interface<IValue> ptr_val;
			GetSubValue(arg, ptr_val);
			if ( ! ptr_val ) return false;
			CJCStringT str_tmp;
			ptr_val->GetValueText(str_tmp);
			CConvertor<TYPE>::S2T(str_tmp.c_str(), val);
			return true; 
		}

		bool GetCommand(JCSIZE index, CJCStringT & cmd);
		bool GetCommand(JCSIZE index, IValue * & val);
		void AddValue(const CJCStringT & name, IValue * value);
		bool Exist(const CJCStringT & name);
	};

	class CArguDescBase	// Argument Description
	{
	public:
		CArguDescBase(LPCTSTR name, TCHAR abbrev, jcparam::VALUE_TYPE vt, LPCTSTR desc = NULL)
			: mName(name), mAbbrev(abbrev), mValueType(vt), mDescription(desc), m_offset(0) {};
		CArguDescBase(const CArguDescBase & arg)
			: mName(arg.mName), mAbbrev(arg.mAbbrev)
			, mValueType(arg.mValueType), mDescription(arg.mDescription), m_offset(0) {};

		virtual ~CArguDescBase(void) {};

		virtual void SetValue(BYTE * base, LPCTSTR val) const {};

		CJCStringT		mName;			// 参数名称
		TCHAR			mAbbrev;		// 略称
		VALUE_TYPE		mValueType;		// 值类型，降值解释为数字或者是字符串。对于COMMAND和SWITCH，忽略此项。
		JCSIZE			m_offset;		
		LPCTSTR			mDescription;
	};	

	template <typename ARG_TYPE, typename CONVERTOR = CConvertor<ARG_TYPE> >
	class CTypedArguDesc : public CArguDescBase
	{
	public:
		CTypedArguDesc(LPCTSTR name, TCHAR abbrev, JCSIZE offset, LPCTSTR desc = NULL)
			: CArguDescBase(name, abbrev, ( type_id<ARG_TYPE>::id() ), desc)
		{
			m_offset = offset;
		}

		virtual void SetValue(BYTE * base, LPCTSTR str_val) const
		{
			BYTE * p = base + m_offset;
			ARG_TYPE & ref_val = *( reinterpret_cast<ARG_TYPE*>(p) );
			CONVERTOR::S2T(str_val, ref_val);
		}
	};

	class CArguDefList
	{
	public:
		enum PROPERTY
		{
			PROP_MATCH_PARAM_NAME = 0x80000000,
		};

		class RULE;

	public:
		CArguDefList(void);
		CArguDefList(const RULE & rule, DWORD properties = 0);
		virtual ~CArguDefList(void);
		void OutputHelpString(FILE * output) const;

		bool CheckParameterName(const CJCStringT & param_name) const;
		const CArguDescBase * GetParamDef(TCHAR abbrev) const
		{
			JCASSERT(m_abbr_map);
			return m_abbr_map[(abbrev & 0x7f)];
		}

		bool AddParamDefine(const CArguDescBase *);
		bool Parse(LPCTSTR cmd, BYTE * base);

	// 
		//template <typename ARG_SET_TYPE>
		//static bool ParseCmdLineEx(const CArguDefList & param_def, LPCTSTR cmd, ARG_SET_TYPE * arg_set, CArguSet & arg)
		//{
		//	bool br = ParseCommandLine(param_def, cmd, arg);

		//}
	protected:
		bool ParseToken(LPCTSTR token, JCSIZE len, BYTE * base);
		bool OnToken(const CJCStringT & argu_name, LPCTSTR val, BYTE * base);


	protected:
		typedef std::map<CJCStringT, const CArguDescBase *>	PARAM_MAP;
		typedef PARAM_MAP::const_iterator				PARAM_ITERATOR;
		typedef std::pair<CJCStringT, const CArguDescBase*>	ARG_DESC_PAIR;
		typedef const CArguDescBase *						PTR_ARG_DESC;

		PARAM_MAP			*m_param_map;
		PTR_ARG_DESC		*m_abbr_map;
		DWORD				m_properties;
		int					m_command_index;

	public:
		CArguSet			m_remain;
	};

	class CArguDefList::RULE
	{
	public:
		RULE();
		RULE & operator () (LPCTSTR name, TCHAR abbrev, jcparam::VALUE_TYPE vt, LPCTSTR desc = NULL);
		RULE & operator () (CArguDescBase * def);
		friend class CArguDefList;

	protected:
		PARAM_MAP			* m_param_map;
		PTR_ARG_DESC		* m_abbr_map;
	};



	class CCmdLineParser
	{
	public:
		static bool ParseCommandLine(CArguDefList & param_def, LPCTSTR cmd, CArguSet & arg)
		{
			param_def.Parse(cmd, NULL);
			CParamSet::ITERATOR it = param_def.m_remain.Begin();
			CParamSet::ITERATOR endit = param_def.m_remain.End();
			for ( ; it != endit; ++it)
			{
				IValue * val = it->second;
				JCASSERT(val);
				arg.AddValue(it->first, val);
			}
			return true;
		}
	


	//protected:
	//	static bool OnToken(const CArguDefList & param_def, LPCTSTR token, JCSIZE len, CArguSet & arg);

	//protected:
	//	static int		m_command_index;
	};
};
