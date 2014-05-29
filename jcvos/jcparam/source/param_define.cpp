
#include "../../stdext/stdext.h"
#include "../include/value.h"
#include "../include/param_define.h"

using namespace jcparam;

//int CCmdLineParser::m_command_index = 0;


///////////////////////////////////////////////////////////////////////////////
//-- CArguSet
bool CArguSet::GetCommand(JCSIZE index, CJCStringT & str_cmd)
{
	stdext::auto_interface<IValue> cmd;
	bool br = GetCommand(index, cmd);
	if ( !br ) return false;
	cmd->GetValueText(str_cmd);
	return true;
}

bool CArguSet::GetCommand(JCSIZE index, IValue * & val)
{
	JCASSERT(NULL == val);
	CJCStringT param_name;
	CConvertor<int>::T2S(index, param_name);
	GetSubValue(param_name.c_str(), val);
	return (val != NULL);
}

void CArguSet::AddValue(const CJCStringT & name, IValue * value)
{
	SetSubValue(name.c_str(), value);
}

bool CArguSet::Exist(const CJCStringT & name)
{
	stdext::auto_interface<IValue> ptr_val;
	GetSubValue(name.c_str(), ptr_val);
	return ( ptr_val.valid() );
}

///////////////////////////////////////////////////////////////////////////////
//-- CArguDefList::RULE
CArguDefList::RULE::RULE()
	: m_param_map(NULL)
	, m_abbr_map(NULL)
{
	m_param_map = new PARAM_MAP;
	m_abbr_map = new PTR_ARG_DESC[128];
	memset(m_abbr_map, 0, sizeof(CArguDescBase*)*128);
}

CArguDefList::RULE & CArguDefList::RULE::operator() (
	LPCTSTR name, TCHAR abbrev, jcparam::VALUE_TYPE vt, LPCTSTR desc)
{
	JCASSERT(m_param_map);
	JCASSERT(m_abbr_map);

	// add descriptions to map
	stdext::auto_ptr<CArguDescBase> ptr_arg( new CArguDescBase(name, abbrev, vt, desc) );
	CArguDefList::RULE & rule = operator() (ptr_arg);
	ptr_arg.detatch();
	return rule;

	//const CArguDescBase * & _arg = m_abbr_map[abbrev];
	//
	//if (abbrev) 
	//{
	//	if (_arg)  THROW_ERROR( ERR_PARAMETER, 		// 略称重复定义
	//			_T("Abbreviation %c has already been used by argument %s"), abbrev, 
	//			_arg->mName.c_str());
	//	_arg = ptr_arg;
	//}

	//std::pair<PARAM_ITERATOR, bool> rs = m_param_map->insert(ARG_DESC_PAIR(name, ptr_arg));
	//if (!rs.second)
	//{	// 重复定义
	//	_arg = NULL;
	//	THROW_ERROR(ERR_PARAMETER, _T("Argument %s has already defined"), ptr_arg->mName.c_str());
	//}
	//ptr_arg.detatch();

	//return *this;
}

CArguDefList::RULE & CArguDefList::RULE::operator () (CArguDescBase * def)
{
	JCASSERT(def);

	TCHAR abbrev = def->mAbbrev;
	const CArguDescBase * & _arg = m_abbr_map[abbrev];
	
	if (abbrev) 
	{
		if (_arg)  THROW_ERROR( ERR_PARAMETER, 		// 略称重复定义
				_T("Abbreviation %c has already been used by argument %s"), abbrev, 
				_arg->mName.c_str());
		_arg = def;
	}

	std::pair<PARAM_ITERATOR, bool> rs = m_param_map->insert(ARG_DESC_PAIR(def->mName, def));
	if (!rs.second)
	{	// 重复定义
		_arg = NULL;
		THROW_ERROR(ERR_PARAMETER, _T("Argument %s has already defined"), def->mName.c_str());
	}
	return *this;
}


///////////////////////////////////////////////////////////////////////////////
//-- CArguDefList
CArguDefList::CArguDefList(void)
	: m_properties(0)
	, m_param_map(NULL)
	, m_abbr_map(NULL)
{
	m_param_map = new PARAM_MAP;
	m_abbr_map = new PTR_ARG_DESC[128];
	memset(m_abbr_map, 0, sizeof(CArguDescBase*)*128);
}

CArguDefList::CArguDefList(const RULE & rule, DWORD properties)
	: m_properties(properties)
	, m_param_map(rule.m_param_map)
	, m_abbr_map(rule.m_abbr_map)
{
	JCASSERT(m_param_map);
	JCASSERT(m_abbr_map);
}
		
CArguDefList::~CArguDefList(void)
{
	JCASSERT(m_param_map);

	PARAM_ITERATOR param_end_it = m_param_map->end();
	for (PARAM_ITERATOR it = m_param_map->begin(); it != param_end_it;  ++it)
	{
		delete (it->second);
	}

	delete m_param_map;
	delete [] m_abbr_map;
}

bool CArguDefList::AddParamDefine(const CArguDescBase * arg_desc)
{
	JCASSERT(arg_desc);

	TCHAR abbrev = arg_desc->mAbbrev;
	if ( abbrev && m_abbr_map[abbrev] )
	{
		THROW_ERROR( ERR_PARAMETER, 		// 略称重复定义
			_T("Abbreviation %c has already been used by argument %s"), abbrev, 
			arg_desc->mName.c_str());
	}

	std::pair<PARAM_ITERATOR, bool> rs = m_param_map->insert(ARG_DESC_PAIR(arg_desc->mName, arg_desc));
	if (!rs.second)
	{	// 重复定义
		THROW_ERROR(ERR_PARAMETER, _T("Argument %s has already defined"), arg_desc->mName.c_str());
	}
	if ( abbrev ) m_abbr_map[abbrev] = arg_desc;
	return true;
}

void CArguDefList::OutputHelpString(FILE * output) const
{
	JCASSERT(m_param_map);

	PARAM_ITERATOR it;
	PARAM_ITERATOR endit = m_param_map->end();

	for ( it = m_param_map->begin(); it != endit; ++it)
	{
		const CArguDescBase* desc = it->second;
		stdext::jc_fprintf(output, _T("\t"));
		if ( desc->mAbbrev )	stdext::jc_fprintf(output, _T("-%c "), desc->mAbbrev);
		else					stdext::jc_fprintf(output, _T("   "));
		if ( _T('#') != desc->mName[0] )	stdext::jc_fprintf(output, _T("--%s "), desc->mName.c_str());
		else								stdext::jc_fprintf(output, _T("     ") );
		stdext::jc_fprintf(output, _T("\t: "));
		if ( desc->mDescription )
			stdext::jc_fprintf(output, desc->mDescription);
		stdext::jc_fprintf(output, _T("\n"));
	}
}

bool CArguDefList::CheckParameterName(const CJCStringT & param_name) const
{
	JCASSERT(m_param_map);

	if ( m_properties & PROP_MATCH_PARAM_NAME )
	{
		PARAM_ITERATOR it = m_param_map->find(param_name); 
		if ( it == m_param_map->end() ) return false;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//-- CCmdLineParser
bool CArguDefList::Parse(LPCTSTR cmd, BYTE * base) 
{
	m_command_index = 0;
	if (NULL == cmd || 0 == *cmd ) return true;
	enum STATUS
	{
		WHITE_SPACE,
		PARAM_WORD,
		QUOTATION,
		//ESCAPE,
	};

	STATUS ss=WHITE_SPACE;
	LPCTSTR str = cmd, start = NULL;
	CJCStringT token;
	do
	{
		switch (ss)
		{
		case WHITE_SPACE:
			if ( _tcschr(_T(" \t\r\n"), *str) == 0)
			{
				// not white space
				start = str;
				if ( _T('\"') == *str )	++ start, ss = QUOTATION;	// skip "
				else						ss = PARAM_WORD;
			}
			break;

		case PARAM_WORD:
			if ( _T('\"') == *str )
			{
				// copy current string and skie "
				if ( start && str > start)		token += CJCStringT(start, 0, (str - start));
				start = str+1;
				ss = QUOTATION;
			}
			else if ( (0 == *str) || _tcschr(_T(" \t\r\n"), *str) != 0)
			{
				// end of token
				if (start && str > start)		token += CJCStringT(start, 0, (str - start));
				ParseToken(token.c_str(), 0, base);
				token.clear();
				ss = WHITE_SPACE;
			}
			break;

		case QUOTATION:
			if ( 0 == *str) THROW_ERROR(ERR_PARAMETER, _T("Missing close quotator") );
			if ( _T('\"') == *str )
			{
				// end of quotation
				if (start && str > start) 	token += CJCStringT(start, 0, (str - start));
				start = str+1;
				ss = PARAM_WORD;
			}
			break;
		}
		if (0 == * str) break;
		++str;
	}
	while (1);
	return true;
}

// 根据Defination的指示，如果base!=NULL，并且arg_def的offset！=0，
//	将parse到的argument填入base结构中。无法填入的argument填入argset中
bool CArguDefList::ParseToken(LPCTSTR token, JCSIZE len, /*CArguSet & argset,*/ BYTE * base)
{
	//			如果有，这是一个以全名定义的parameter
	//			否则，这是一个以全名定义的swich
	//		否则，检查第二个字符的描述
	//			swich：检查下一个描述
	//			parameter：处理参数
	//	否则，这是一个command

	CJCStringT param_name;
	LPCTSTR arg = token;
	const CArguDescBase * arg_desc = NULL;

	// 检查第一个字符，
	if (_T('-') == arg[0])
	{
		//	如果是'-'，检查第二个字符
		if (_T('-') == arg[1])
		{
			//		如果是'-'，检查是否有等号存在
			LPCTSTR dev = _tcschr(arg, _T('='));
			JCSIZE len = 0;
			LPCTSTR str_val = NULL;
			if (dev)	len = (JCSIZE)((dev - arg)-2), str_val = dev+1;
			else		len = (JCSIZE)(_tcslen(arg)-2);

			if (!len)	THROW_ERROR(ERR_PARAMETER, _T("Uncorrect argument %s."), arg);
			param_name = CJCStringT(arg+2, 0, len);
			OnToken(param_name, str_val, base);
			//PARAM_MAP::const_iterator it = m_param_map->find(param_name);
			//if ( it != m_param_map->end() )
			//{
			//	if (base && arg_desc->m_offset > 0)
			//	{
			//		arg_desc->SetValue(base, 
			//	}
			//}
			//else arg = str_val;
		}
		else
		{
			++arg;
			LPCTSTR org = arg;
			while (*arg)
			{
				const CArguDescBase * arg_desc = GetParamDef(*arg);
				if (!arg_desc) THROW_ERROR(ERR_PARAMETER, 
					_T("Unknow abbreviate option: -%c at %s"), (char)(*arg), org);

				++arg;
				if (jcparam::VT_BOOL == arg_desc->mValueType)
				{
					OnToken(arg_desc->mName, _T("true"), base);
					//if (base && arg_desc->m_offset > 0)
					//{
					//	arg_desc->SetValue(base, _T("true"));
					//}
					//else
					//{
					//	IValue * val = static_cast<IValue*>(CTypedValue<bool>::Create(true));
					//	argset.SetSubValue(arg_desc->mName.c_str(), val);
					//	val->Release();
					//}
				}
				else
				{
					//param_name = arg_desc->mName;
					OnToken(arg_desc->mName, arg, base);
					break;
				}
			}
		}
	}
	else
	{
		// No name parameter
		//CConvertor<int>::T2S(m_command_index++, param_name);
		TCHAR _name[16];
		stdext::jc_sprintf(_name, _T("#%02d"), m_command_index);
		OnToken(_name, arg, base);
	}

	//if (arg && !param_name.empty() )
	//{
	//	IValue * val = NULL;
	//	val = static_cast<IValue*>(CTypedValue<CJCStringT>::Create(arg));
	//	argset.SetSubValue(param_name.c_str(), val);
	//	val->Release();
	//}
	return false;
}

bool CArguDefList::OnToken(const CJCStringT & argu_name, LPCTSTR argu_val, BYTE * base)
{
	PARAM_MAP::const_iterator it = m_param_map->find(argu_name);
	IValue * pval = NULL;
	if ( it == m_param_map->end() )
	{
		if ( m_properties & PROP_MATCH_PARAM_NAME)	THROW_ERROR(ERR_PARAMETER, _T("unknown argument name &s."), argu_name)
		else	pval = static_cast<IValue*>(CTypedValue<CJCStringT>::Create(argu_val));
	}
	else
	{
		const CArguDescBase * argu_def = it->second;
		if (base && argu_def->m_offset > 0)	argu_def->SetValue(base, argu_val);
		else	pval = static_cast<IValue*>(CTypedValue<CJCStringT>::Create(argu_val));
	}

	if (pval)
	{
		m_remain.SetSubValue(argu_name.c_str(), pval);
		pval->Release();
	}
	return true;
}

