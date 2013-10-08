// jcparam_test.cpp : 定义控制台应用程序的入口点。
//
#define LOGGER_LEVEL LOGGER_LEVEL_DEBUGINFO

#include "stdafx.h"
#include <vld.h>
//#include <boost/tokenizer.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <jclogger.h>
#include <jcparam.h>

LOCAL_LOGGER_ENABLE(_T("SmiVenderTest"), LOGGER_LEVEL_DEBUGINFO);

namespace lex = boost::spirit::lex;

enum LEXER_ID
{
	ID_WHITE_SPACE_ = 0,
	ID_WORD,
	ID_QUOTATION,
	ID_SEPERATOR,
	ID_COMMENT,
	ID_CONTROL,
};

typedef lex::lexertl::token< TCHAR const *> token_type;
typedef lex::lexertl::lexer< token_type> lexer_type;

template <typename Lexer>
class csv_tokens : public lex::lexer<Lexer>
{
public:
	csv_tokens()
	{
		this->self.add
			(_T("[ \t\n]+"),	ID_WHITE_SPACE_)
			(_T("\\#"),			ID_COMMENT)
			(_T(","),			ID_SEPERATOR)
			(_T("\\[[^\\]]+\\]"),		ID_CONTROL)
			(_T("\\\"[^\\\"]+\\\""),	ID_QUOTATION)
			(_T("[^, \t\n]+"),	ID_WORD)
			;
	}
};

class analyze_status
{
public:
	enum STATUS
	{
		NAME = 0,
		ABBREV = 1,
		TYPE = 2,
		DEFAULT = 3,
		DESCRIPTION = 4,
	};
public:
	analyze_status() 
		: m_ss(NAME), m_id(ID_WHITE_SPACE_), 
		m_abbrev(0), m_type(jcparam::VT_INT), m_desc(NULL)
	{};
public:
	int m_ss;
	LEXER_ID	m_id;
	CJCStringT m_name;
	TCHAR m_abbrev;
	jcparam::VALUE_TYPE	m_type;
	LPCTSTR m_desc;
};

class line_analyze
{
protected:
	analyze_status * m_status;

public:
	line_analyze(analyze_status * ss) : m_status(ss) {};
	line_analyze(const line_analyze & ss) : m_status(ss.m_status) {};

	template <typename TokenT> bool operator() (const TokenT & tok)
	{
		LEXER_ID id = (LEXER_ID)tok.id();
		if ((int)id == 0x00010000 || ID_WHITE_SPACE_ == id)	return true;		// Empty line?
		if (ID_SEPERATOR == id)
		{
			++m_status->m_ss;
			return true;
		}

		boost::iterator_range<LPCTSTR> range = tok.value();
		LPCTSTR ss = range.begin();
		JCSIZE ll = range.size();

		if (ID_QUOTATION == id)
		{
			if (ll <= 2) return true;
			++ss, ll -=2;
			id = ID_WORD;
		}

		switch (m_status->m_ss)
		{
		case analyze_status::NAME:
			m_status->m_name = CJCStringT(ss, 0, ll);
			if ( ID_CONTROL == id || ID_COMMENT == id )
			{
				m_status->m_id = id;
				return false;
			}
			else
			{
				m_status->m_id = ID_WORD;
				return true;
			}

		case analyze_status::ABBREV:
			m_status->m_abbrev = ss[0];
			return true;

		case analyze_status::TYPE: 		{
			CJCStringT stype(ss, 0, ll);
			m_status->m_type = jcparam::StringToType(stype.c_str());
			return true;
										}

		case analyze_status::DEFAULT:
			return true;

		case analyze_status::DESCRIPTION:
			m_status->m_desc = ss;
			return false;
		}

		return false;
	};

};

//int line_analyze::count = 0;

using namespace jcparam;

void OutputArgument(CCmdLineParser & cmd_line)
{
	stdext::auto_cif<CParamSet, IValue> argument;
	cmd_line.GetValI(NULL, argument);
	JCASSERT(argument.valid());

	// output command

	stdext::auto_cif<CValueArray, IValue> cmd_set;
	argument->GetSubValue(CArguSet::PNAME_CMD, cmd_set);
	LOG_DEBUG(_T("command list:")); 
	if ( cmd_set.valid() )
	{
		//JCASSERT(cmd_set.valid());
		size_t cmd_size = cmd_set->GetSize();
		for (size_t ii = 0; ii < cmd_size; ++ ii)
		{
			stdext::auto_cif<CTypedValue<CJCStringT>, IValue> cmd;
			cmd_set->GetValueAt(ii, cmd);
			JCASSERT(cmd.valid());
			CJCStringT str_cmd;
			cmd->GetValueText(str_cmd);
			LOG_DEBUG(_T("\t command %d: %s"), ii, str_cmd.c_str() );
		}
	}
	
	// output parameters
	LOG_DEBUG(_T("Parameter list:")); 

	CParamSet::ITERATOR it = argument->Begin();
	CParamSet::ITERATOR endit = argument->End();

	for ( ; it != endit; ++it)
	{
		if ( _tcscmp(it->first.c_str(), CArguSet::PNAME_CMD) ==0 ) continue;
		CJCStringT str_val;
		it->second->GetValueText(str_val);
		LOG_DEBUG(_T("\t parameter: %s = %s"), it->first.c_str(), str_val.c_str()); 
	}

}

int _tmain(int argc, _TCHAR* argv[])
{
	LOGGER_TO_FILE(O, _T("jcparam_test.log") );
	CJCLogger::Instance()->SetColumnSelect( CJCLogger::COL_COMPNENT_NAME | CJCLogger::COL_FUNCTION_NAME);


	// Load param config from file

	if (argc < 2)
	{
		LOG_ERROR(_T("Argument is too few") );
		return 1;
	}

	CCmdLineParser cmd_line;

	FILE * file = NULL;
	if (_tfopen_s(&file, argv[1], _T("r")) != 0)
	{
		LOG_ERROR(_T("Cannot open configure file %s"), argv[1] );
		return 1;
	}

	// 关于配置文件：
	//	没有任何符号开始的行为一组参数定义行。
	//	* 开始的行为测试行。测试行必须在参数定义之后。
	//	# 还是的行为注释行。

	static const int MAX_STR_BUF=1024;
	static const int MAX_ARGUMENT=256;

	TCHAR line_buf[MAX_STR_BUF];

	std::vector<TCHAR*>	configs;
	try
	{

		int kk = 0;
		while (1)
		{
			if ( _fgetts(line_buf, MAX_STR_BUF, file) == NULL) break;
			size_t len = _tcslen(line_buf);
			if ( len == 0 ) continue;
			TCHAR * str = new TCHAR[len+1];
			_tcscpy_s(str, len+1, line_buf);
			configs.push_back(str);

			csv_tokens< lexer_type > csv_token_functor;
			LPCTSTR first = str, str_end = str+len;

			analyze_status as;
			line_analyze   analyzer(&as);
			bool br = lex::tokenize(first, str_end, csv_token_functor, analyzer);

			if ( ID_COMMENT == as.m_id || ID_WHITE_SPACE_ == as.m_id) continue;
			if ( ID_CONTROL == as.m_id && 
				as.m_name == _T("[end]") ) break;

			try
			{
				cmd_line.AddParameterInfo(as.m_name.c_str(), as.m_abbrev, as.m_type, as.m_desc);
			}
			catch (stdext::CJCException & err)
			{
				LOG_DEBUG(_T("Wrong defination: %s, (err = %S) "), as.m_desc, err.what());
			}

		}

		// do test
		//stdext::auto_array<TCHAR*> test_argv(MAX_ARGUMENT);
		//test_argv[0] = _T("jcparam_test");
		while (1)
		{
			if ( _fgetts(line_buf, MAX_STR_BUF, file) == NULL) break;
//			if (!line_buf || line_buf[0] != _T('*')) continue;
			if (line_buf || line_buf[0] == _T('#')) continue;
			LOG_DEBUG(_T("Begin test for: %s"), line_buf);
			//int test_argc = 1;
			try
			{
				//cmd_line.ParseCommandLine(test_argc, test_argv);
				cmd_line.ParseCommandLine(line_buf);
				OutputArgument(cmd_line);
			}
			catch (stdext::CJCException & err)
			{
				LOG_DEBUG(_T("Test error because: %S"), err.what());
			}
		}

	}
	catch (stdext::CJCException & err)
	{
		printf("Error: %s\r\n", err.what());
	}

	std::vector<TCHAR*>::iterator it, endit = configs.end();
	for (it = configs.begin(); it != endit; ++it)
	{
		delete [] (*it);
	}

	fclose(file);

#ifdef _DEBUG
	printf("Press any key...");
	getchar();
#endif

	return 0;
}

