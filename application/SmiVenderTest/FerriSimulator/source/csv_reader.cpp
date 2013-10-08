#include "stdafx.h"

#include "csv_reader.h"
#include <boost/spirit/include/lex_lexertl.hpp>

LOCAL_LOGGER_ENABLE(_T("csv_reader"), LOGGER_LEVEL_NOTICE);

// 词法解析器
namespace lex = boost::spirit::lex;
typedef lex::lexertl::token< TCHAR const *> token_type;
typedef lex::lexertl::lexer< token_type> lexer_type;





template <typename Lexer>
class cmd_line_tokens : public lex::lexer<Lexer>
{
public:

	cmd_line_tokens()
	{
		this->self.add
			//(_T("\\n"),					ID_NEW_LINE)
			(_T("\\s+"),				ID_WHITE_SPACE_)	// 空白
			(_T("[\\,]"),				ID_COMMA)			// 逗号
			(_T("0x[0-9a-fA-F]+"),		ID_HEX)
			//(_T("[_a-zA-Z][_a-zA-Z0-9]*"),	ID_WORD)			// 标识符
			(_T("\\-?[0-9]+\\.[0-9]*"),	ID_FLOAT)
			(_T("\\-?[0-9]+"),			ID_DECIMAL)
			//(_T(".*"),					ID_OTHER)
			;
	}
};

class lexer_parser
{
public:
	lexer_parser(CTokenProperty * prop)	: m_property(prop) {};
	template <typename TokenT> bool operator() (const TokenT & tok)
	{
		LOG_STACK_TRACE();

		TOKEN_ID id = (TOKEN_ID)(tok.id());

		//m_property->m_id = id;
		boost::iterator_range<LPCTSTR> token_value = tok.value();

		LPCTSTR t_begin = token_value.begin();
		JCSIZE t_len = token_value.size();

		LOG_DEBUG(_T("token_id=%d"), (int)(id));

		switch (id)
		{
		case ID_WHITE_SPACE_:
			return true;

		//case ID_NEW_LINE:
		//	JCASSERT(m_property->m_parser);
		//	m_property->m_parser->NewLine();
		//	break;

		case ID_OTHER:
			m_property->m_id = ID_OTHER;
			return false;

		case ID_COMMA:
			//m_property->m_index ++;
			return false;

		case ID_DECIMAL: {
			LPTSTR end = NULL;
			m_property->m_int = stdext::jc_str2l(t_begin, &end);
			JCASSERT( (end - t_begin) >= (int)t_len);
			m_property->m_id = id;
			return true;}

		case ID_HEX:
			m_property->m_int = (UINT)stdext::str2hex(t_begin+2);
			m_property->m_id = id;
			return true;

		case ID_FLOAT:
			m_property->m_double = _tstof(t_begin);
			m_property->m_id = id;
			return true;

		default:
			break;
		}
		return false;
	}

	CTokenProperty * m_property;
};

void LineParse(LPCTSTR &first, LPCTSTR last, CTokenProperty * prop)
{
	LOG_STACK_TRACE();
	static const cmd_line_tokens< lexer_type> token_functor;
	lexer_parser analyzer(prop);
	lex::tokenize(first, last, token_functor, analyzer);
}

