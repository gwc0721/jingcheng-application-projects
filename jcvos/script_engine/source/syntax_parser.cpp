#include "stdafx.h"

#include <boost/spirit/include/lex_lexertl.hpp>
#include "syntax_parser.h"
#include "atom_operates.h"
#include "expression_op.h"
#include "flow_ctrl_op.h"
#include "../include/parser.h"

// LOG 输出定义：
//  WARNING:	编译错误
//	NOTICE:		输出编译中间结果
//  TRACE:		调用堆栈

LOCAL_LOGGER_ENABLE(_T("script.parser"), LOGGER_LEVEL_WARNING);

#define LOG_COMPILE(...)		{	\
	TCHAR _str[256] = {0};	\
	_tcscat_s(_str, 255, _T("\t"));		\
	int ir = stdext::jc_sprintf(_str, 200, __VA_ARGS__);		\
	/*memcpy_s(_str + ir, 55*sizeof(TCHAR), m_first, 55*sizeof(TCHAR));*/	\
	/*_str[ir + 56] = 0;	*/	\
	LOG_NOTICE(_str);		\
}


#define ERROR_MSG_BUFFER	(512)


using namespace jcscript;

// 命令行的词法解析部分
namespace lex = boost::spirit::lex;

typedef lex::lexertl::token< TCHAR const *> token_type;
typedef lex::lexertl::lexer< token_type> lexer_type;

// 词法定义
template <typename Lexer>
class cmd_line_tokens : public lex::lexer<Lexer>
{
public:

	cmd_line_tokens()
	{
		this->self.add
			(_T("\\n"),					ID_NEW_LINE)
			(_T("\\s+?"),				ID_WHITE_SPACE_)	// 空白
			(_T("[\\:]"),				ID_COLON)			// 冒号，模块名与命令的分界
			(_T("[\\.]"),				ID_DOT)				// 变量及其成员的分割
			(_T("\\{"),					ID_CURLY_BRACKET_1)		// 花括弧，跟在$后面用于表示执行字句
			(_T("\\}"),					ID_CURLY_BRACKET_2)
			(_T("\\("),					ID_BRACKET_1)		// 括弧，跟在$后面表示创建变量
			(_T("\\)"),					ID_BRACKET_2)	
			(_T("\\["),					ID_SQUARE_BRACKET_1)
			(_T("\\]"),					ID_SQUARE_BRACKET_2)
			(_T("\\#[^\\n]*"),			ID_COMMENT)			// 注释
			(_T("\\|[\\n]?"),			ID_CONNECT)			// 命令连接
			(_T("\\$"),					ID_VARIABLE)		// 变量
			(_T("\\@"),					ID_TABLE)			// 变量
			(_T("\\="),					ID_EQUAL)			// 等号
			(_T("\\%[_a-zA-Z][_a-zA-Z0-9]*"),	ID_PREDEF_VAR)		// 预定义变量
			(_T("\\/[^\\/]+\\/"),		ID_FILE_NAME)		// 双引号
			(_T("\\\"[^\\\"]+\\\""),	ID_QUOTATION1)		// 双引号
			//(_T("\\'[^\\']+\\'"),		ID_QUOTATION2)		// 单引号
			(_T("\\-\\-[_a-zA-Z][_a-zA-Z0-9]*"),ID_PARAM_NAME)		// 参数名称
			(_T("\\-\\-\\@"),			ID_PARAM_TABLE)
			(_T("\\-[a-zA-Z]\\s+"),		ID_ABBREV_BOOL)		// 缩写参数
			(_T("\\-[a-zA-Z]"),			ID_ABBREV)			// 缩写参数
			(_T("=>"),					ID_ASSIGN)

			(_T("=="),					ID_RELOP_EE)
			(_T("<="),					ID_RELOP_LE)
			(_T(">="),					ID_RELOP_GE)
			(_T("<"),					ID_RELOP_LT)
			(_T(">"),					ID_RELOP_GT)

			(_T("[\\+\\-]"),			ID_ATHOP_ADD_SUB)
			(_T("[\\*\\/]"),			ID_ATHOP_MUL_DIV)
			(_T("[\\&\\|\\^]"),			ID_ATHOP_BITOP)

			(_T("0x[0-9a-fA-F]+"),		ID_HEX)

			(_T("\\-?[0-9]+"),			ID_DECIMAL)

			// 关键词
			(_T("and"),					ID_BOOLOP_AND)
			(_T("else"),				ID_KEY_ELSE)
			(_T("end"),					ID_KEY_END)
			(_T("exit"),				ID_KEY_EXIT)
			(_T("filter"),				ID_KEY_FILTER)
			(_T("help"),				ID_KEY_HELP)
			(_T("if"),					ID_KEY_IF)
			(_T("not"),					ID_BOOLOP_NOT)
			(_T("or"),					ID_BOOLOP_OR)
			(_T("then"),				ID_KEY_THEN)

			// 标识符
			(_T("[_a-zA-Z][_a-zA-Z0-9]*"),	ID_WORD)			
			;
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class lexer_parser
{
public:
	lexer_parser(CTokenProperty * prop)	: m_property(prop) {};
	template <typename TokenT> bool operator() (const TokenT & tok)
	{
		TOKEN_ID id = (TOKEN_ID)(tok.id());
		LOG_TRACE(_T("[TRACE] token_id=%d"), (int)(id) );

		m_property->m_id = id;
		boost::iterator_range<LPCTSTR> token_value = tok.value();

		LPCTSTR t_begin = token_value.begin();
		JCSIZE t_len = token_value.size();

		switch (id)
		{

		case ID_WHITE_SPACE_:
			return true;

		case ID_NEW_LINE:
			JCASSERT(m_property->m_parser);
			m_property->m_parser->NewLine();
			break;

		case ID_CONNECT:
			if (t_begin[1] == _T('\n'))	m_property->m_parser->NewLine();
			break;


		case ID_FILE_NAME:
			m_property->m_str = CJCStringT(t_begin + 1, 0, t_len -2 );
			break;

		case ID_QUOTATION1:
		case ID_QUOTATION2:
			m_property->m_str = CJCStringT(t_begin + 1, 0, t_len -2 );
			m_property->m_id = ID_STRING;
			break;

		case ID_DECIMAL: {
			LPTSTR end = NULL;
			m_property->m_val = stdext::jc_str2l(t_begin, &end);
			JCASSERT( (end - t_begin) >= (int)t_len);
			m_property->m_id = ID_NUMERAL;
			break;}

		case ID_HEX:
			m_property->m_val = stdext::str2hex(t_begin+2);
			m_property->m_id = ID_NUMERAL;
			break;

		case ID_WORD:
			m_property->m_str = CJCStringT(t_begin, 0, t_len);	
			break;

		case ID_ABBREV:
		case ID_ABBREV_BOOL:
			m_property->m_val = t_begin[1];
			break;

		case ID_PARAM_NAME:		//	除去参数名称前的--
			m_property->m_str = CJCStringT(t_begin+2, 0, t_len-2);		
			break;

		case ID_PREDEF_VAR:		//	除去预定义变两前的%
			m_property->m_str = CJCStringT(t_begin+1, 0, t_len-1);		
			break;

		case ID_RELOP_EE:
		case ID_RELOP_LE:
		case ID_RELOP_GE:
		case ID_RELOP_LT:
		case ID_RELOP_GT:
			m_property->m_id = ID_REL_OP;
			m_property->m_val = (UINT)(id);
			break;

		case ID_ATHOP_ADD_SUB:
		case ID_ATHOP_MUL_DIV:
		case ID_ATHOP_BITOP:
			m_property->m_val = (UINT)(t_begin[0]);
			break;

		default:
			break;
		}
		return false;
	}

	CTokenProperty * m_property;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//-- syntax error handling
#define SYNTAX_ERROR(...)	{ \
		LOG_DEBUG(_T("syntax error!"));	\
		OnError(__VA_ARGS__);			\
		}


class CSyntaxError : public stdext::CJCException
{
public:
	CSyntaxError(JCSIZE line, JCSIZE col, LPCTSTR msg)	
		: stdext::CJCException(msg, stdext::CJCException::ERR_PARAMETER)
		, m_line(line)
		, m_column(col)
	{
	}
	CSyntaxError(const CSyntaxError & err) : stdext::CJCException(err) {};

public:
	JCSIZE m_line;
	JCSIZE m_column;
};



// 缓存总长度
#define SRC_BUF_SIZE	(SRC_READ_SIZE + SRC_BACK_SIZE)


CSyntaxParser::CSyntaxParser(IPluginContainer * plugin_container, LSyntaxErrorHandler * err_handler)
	: m_file(NULL)
	, m_src_buf(NULL)
	, m_var_set(NULL)
	, m_one_line(false)
	, m_error_handler(err_handler)
	, m_syntax_error(false)
	, m_token_func(NULL)
	, m_first_it(NULL)
	, m_last_it(NULL)
	, m_first(NULL)
{
	LOG_STACK_TRACE();
	JCASSERT(plugin_container);
	m_plugin_container = plugin_container;
	m_plugin_container->AddRef();

	m_plugin_container->GetVarOp(m_var_set);

	m_lookahead.m_parser = this;
}

CSyntaxParser::~CSyntaxParser(void)
{
	LOG_STACK_TRACE();
	if (m_plugin_container) m_plugin_container->Release();
	delete [] m_src_buf;

	if (m_var_set) m_var_set->Release();

	if (m_first_it)		delete m_first_it;
	if (m_last_it)		delete m_last_it;
}

void CSyntaxParser::Source(FILE * file)
{
	LOG_STACK_TRACE();
	JCASSERT(file);

	m_one_line = false;
	m_file = file;

	m_src_buf = new TCHAR[SRC_BUF_SIZE];
	m_first = m_src_buf;
	m_last = m_src_buf;

	m_line_begin = m_first;
	m_line_num = 1;

	m_token_func = &CSyntaxParser::Token;

	NextToken(m_lookahead);
}


void CSyntaxParser::Parse(LPCTSTR &first, LPCTSTR last)
{
	LOG_STACK_TRACE();
	m_one_line = true;

	m_first = first;
	m_last = last;

	m_line_num = 1;
	m_line_begin = m_first;

	m_token_func = &CSyntaxParser::Token;
	NextToken(m_lookahead);
}

void CSyntaxParser::Parse(jcparam::IJCStream * stream)
{
	JCASSERT(stream);
	m_first_it = new CReadIterator(stream);
	m_last_it = new CReadIterator();
	m_line_num = 1;

	m_token_func = &CSyntaxParser::StreamToken;
	NextToken(m_lookahead);
}

void CSyntaxParser::StreamToken(CTokenProperty & prop)
{
	typedef lex::lexertl::token<CReadIterator> t_type;
	typedef lex::lexertl::lexer<t_type>	l_type;
	static const cmd_line_tokens< l_type > token_functor;

	LOG_STACK_TRACE();

	try
	{
		lexer_parser analyzer(&prop);
		CReadIterator & first = *m_first_it;
		lex::tokenize(first, *m_last_it, token_functor, analyzer);
		if (ID_SYMBO_ERROR == prop.m_id) 
		{
			prop.m_val = *(*m_first_it);
			++ (*m_first_it);
		}
	}
	catch (CEofException &)
	{
		prop.m_id = ID_EOF;
	}
}

// for ASCII file
void CSyntaxParser::ReadSource(void)
{
	#define READ_ASCII_SIZE		(SRC_READ_SIZE)

	JCSIZE len = m_last - m_first;
	TCHAR * last;

	if (m_file && !feof(m_file) )
	{
		// move tail to begin
		memcpy_s(m_src_buf, (m_first - m_src_buf) * sizeof(TCHAR), m_first, len * sizeof(TCHAR) ); 
		m_first = m_src_buf; 
		last = m_src_buf + len;

		// read from file
		char * file_buf = new char[READ_ASCII_SIZE];
		JCSIZE read_size = fread(file_buf, 1, READ_ASCII_SIZE, m_file);
		JCSIZE buf_remain = SRC_BUF_SIZE - len;
		JCSIZE conv_size = stdext::Utf8ToUnicode(last, buf_remain, file_buf, read_size);
		m_last = last + conv_size;
		last[conv_size] = 0;
		delete [] file_buf;
	}
}

void CSyntaxParser::NewLine(void)
{
	LOG_STACK_TRACE();
	++m_line_num;
	m_line_begin = m_first;
}

void CSyntaxParser::Token(CTokenProperty & prop)
{
	static const cmd_line_tokens< lexer_type> token_functor;

	LOG_STACK_TRACE();
	if ( (m_last - m_first) < SRC_BACK_SIZE)	ReadSource();
	if ( m_first >= m_last )	{prop.m_id = ID_EOF;	return;}
	lexer_parser analyzer(&prop);
	lex::tokenize(m_first, m_last, token_functor, analyzer);
	if (ID_SYMBO_ERROR == prop.m_id) 
	{
		prop.m_val = m_first[0];
		m_first ++;
	}
}

void CSyntaxParser::NextToken(CTokenProperty & prop)
{
	// for compile log
	if (m_first)
	{
		TCHAR strline[64] = {0};
		memcpy_s(strline, 63*sizeof(TCHAR), m_first, 63*sizeof(TCHAR));
		LOG_NOTICE(_T("[token] %s ..."), strline);
	}

	prop.m_id = ID_SYMBO_ERROR;
	JCASSERT(m_token_func);
	(this->*m_token_func)(prop);
	LOG_TRACE(_T("[TRACE] id=%d, val=%d, str=%s"), prop.m_id, (UINT)(prop.m_val), prop.m_str.c_str())

	if (ID_SYMBO_ERROR == prop.m_id)
		SYNTAX_ERROR(_T("Unknow symbo '%c'"), (TCHAR)(prop.m_val) );
}

class COpMakerBoolExp;
class COpMakerAddSub;

bool CSyntaxParser::Match(TOKEN_ID id, LPCTSTR err_msg)
{
	LOG_STACK_TRACE();

	if (m_lookahead.m_id == id)	
	{
		NextToken(m_lookahead);
		return true;
	}
	else	
	{
		SYNTAX_ERROR(err_msg);
		return false;
	}
}

bool CSyntaxParser::MatchScript(IAtomOperate * & program)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == program);
	CSequenceOp * seq = static_cast<CSequenceOp*>(new CScriptOp(NULL) );
	program = static_cast<IAtomOperate*>(seq);

	bool br = MatchScript1(seq);
	return br;
}

///////////////////////////////////////////////////////////////////////////////

bool CSyntaxParser::MatchScript1(CSequenceOp * prog)
{
	LOG_STACK_TRACE();
	JCASSERT(prog);

	LOG_COMPILE(_T("- script  ") );

	bool loop = true;
	while (loop)
	{
		try
		{
			switch (m_lookahead.m_id)
			{
			case ID_EOF:	
				loop = false;
				break;

			case ID_NEW_LINE:	
				NextToken(m_lookahead);
				continue;
				break;

			case ID_COMMENT:	
				LOG_DEBUG(_T("comment line"));
				NextToken(m_lookahead);
				continue;
				break;

			case ID_KEY_HELP:	{
				//stdext::auto_interface<IAtomOperate> op;
				//Match(ID_KEY_HELP, _T("") ); MatchHelp(op);
				//prog->PushBackAo(op);
				break;	}

			case ID_KEY_EXIT: {
				Match(ID_KEY_EXIT, _T("") );
				CExitOp * op = new CExitOp;
				prog->AddOp( static_cast<IAtomOperate *>(op) );
				op->Release();
				break;	}

			//case ID_KEY_IF: {
			//	MatchIfSt(prog);
			//				}

			default:	{
				LOG_COMPILE(_T("- combo  "));
				MatchComboSt( prog );
				break;	}
			}
		}
		catch (CSyntaxError & )
		{
			m_syntax_error = true;
			// skip to line end;
			while (1)
			{
				if ( (m_last - m_first) < SRC_BACK_SIZE)	ReadSource();
				if ( m_first >= m_last )	{m_lookahead.m_id = ID_EOF;	break;}
				if ( *m_first == _T('\n') )	{m_lookahead.m_id = ID_NEW_LINE; break;}
				m_first ++;
			}
		}
	}
	return true;
}

// 对Table的处理：
//	Table总共只有五种情况：
//		(1) 在行的开头：@{语句1} | 函数2...		
//			“语句1”的第一个函数作为循环变量，其余连接在“函数2”之前
//
//		(2) 在行的开头：@变量1 | 函数...		
//			用CLoopVarOp作为循环变量，“变量1”作为CLoopVarOp的参数
//
//		(3) 作为函数的参数：函数1 | 函数2 -b@{语句1}...	
//			“语句1”的第一个函数作为循环变量，“语句1”的最后输出作为“函数2”的参数
//
//		(4) 作为函数的参数：函数1 | 函数2 -b@变量1...		
//			用CLoopVarOp作为循环变量，
//
//		(5) @单独出现：函数1 -b@变量1 | 函数2 -a@ |...
//	因此编译方法如下
//	1，将@后的内容编译成一个新的Combo
//	2, 合并新的Combo到现有Combo，
//		2.1 新Combo的pre-pro序列添加到现有Combo的pre-pro序列，
//		2.2 新Combo的loop序列添加到现有combo的loop序列
//		2.3 新Combo的第一个loop序列作为现有combo的loop value
//	3，如果case (3)(4)，添加push op
//	4, case (1)(2) 现有Combo的LastChain指向新Combo的最后一个


bool CSyntaxParser::MatchComboSt(CSequenceOp * prog)
{
	LOG_STACK_TRACE();
	JCASSERT(prog);

	stdext::auto_interface<CComboSt>	combo(new CComboSt(prog) );

	// 用于编译信息输出
	combo->m_line = m_line_num;
	stdext::auto_interface<IChainOperate> chain;
	switch (m_lookahead.m_id)
	{
	case ID_BRACKET_1:
	case ID_KEY_FILTER:
	case ID_WORD:
	case ID_COLON:
		LOG_COMPILE(_T("- single_st"));
		MatchSingleSt(combo, chain);
		MatchS1(combo, chain);
		break;

	case ID_TABLE: {
		LOG_COMPILE(_T("- table  "));
		bool br = MatchTableSt(combo, chain);
		MatchS1(combo, chain);
		break;	   }
	
	default:	{
		bool match = MatchFactor(combo, chain);
		if (!match) SYNTAX_ERROR(_T("unknow start of line") );
		MatchS1(combo, chain);
		break;
				}
	}

	if ( !combo->IsEmpty() )
	{
		// insert a collector to combo
		LOG_COMPILE(_T("- collector"));

		stdext::auto_interface<IAtomOperate> op;
		op = static_cast<IAtomOperate*>(new CCollectOp);
		op->SetSource(0, chain);
		combo->AddOp(op);
		chain.release();	
		op.detach<IAtomOperate>(chain);

		prog->AddOp( static_cast<IAtomOperate *>(combo) );
	}

	// process assign if there is
	if (ID_ASSIGN == m_lookahead.m_id)
	{
		MatchAssign(prog, chain);
	}
	return true;
}

void CSyntaxParser::MatchS1(CSequenceOp * combo, IChainOperate * &chain)
{
	LOG_STACK_TRACE();
	JCASSERT(combo);

	bool assign_set = false;

	while (1)
	{
		// 右递归简化为循环
		if (ID_CONNECT == m_lookahead.m_id)
		{
			LOG_COMPILE(_T("- connection"));
			NextToken(m_lookahead);		// Match(ID_CONNECT);
			if ( !MatchSingleSt(combo, chain) ) 		SYNTAX_ERROR(_T("Missing single statment") );
		}
		else break;
	}
}

bool CSyntaxParser::MatchAssign(CSequenceOp * combo, IChainOperate * chain)
{
	LOG_STACK_TRACE();
	JCASSERT(combo);

	if (NULL == chain) SYNTAX_ERROR(_T("missing source for assignment"));

	Match(ID_ASSIGN, _T("Missing =>") );

	stdext::auto_interface<IAtomOperate> op;

	switch (m_lookahead.m_id)
	{
	case ID_WORD:	{
		LOG_COMPILE(_T("- assign to var %s"), m_lookahead.m_str.c_str());
		op = static_cast<IAtomOperate*>(new CAssignOp(m_var_set, m_lookahead.m_str));
		NextToken(m_lookahead);
		op->SetSource(0, chain);
		combo->AddOp(op);
		break;		}

	default:
		SYNTAX_ERROR(_T("missing variable name."));
		return false;
	}
	return true;
}

bool CSyntaxParser::MatchSingleSt(CSequenceOp * combo, IChainOperate * & chain)
{
	LOG_STACK_TRACE();
	JCASSERT(combo);

	m_default_param_index = 0;

	stdext::auto_interface<CSingleSt> single_st(new CSingleSt(combo) );
	if (chain) single_st->SetSource(0, chain);

	bool match = true;
	switch (m_lookahead.m_id)
	{
	case ID_BRACKET_1: {
		NextToken(m_lookahead);
		stdext::auto_interface<IAtomOperate> op;
		MatchExpression(static_cast<CSequenceOp*>(single_st), op);
		Match(ID_BRACKET_2, _T("expected ')' "));
		break;		   }

	case ID_FILE_NAME:	{		// 文件名作为作值(assignee)，将结果保存到文件
		LOG_COMPILE(_T("- file  "));
		stdext::auto_interface<IAtomOperate> op;
		op = static_cast<IAtomOperate*>(new CSaveToFileOp(m_lookahead.m_str));
		op->SetSource(0, single_st->GetInPort());
		single_st->AddOp(op);
		NextToken(m_lookahead);
		break;			}	

	case ID_KEY_FILTER:	{
		LOG_COMPILE(_T("- filter  "));
		MatchFilterSt(single_st);
		break;		}

	case ID_WORD:	{		// WORD : WORD
		LOG_COMPILE(_T("- plugin: %s  "), m_lookahead.m_str.c_str() );
		stdext::auto_interface<IPlugin>	plugin;
		m_plugin_container->GetPlugin(m_lookahead.m_str, plugin);
		if ( ! plugin ) SYNTAX_ERROR(_T("Unknow plugin %s "), m_lookahead.m_str.c_str() );
		NextToken(m_lookahead);
		Match(ID_COLON, _T("expected :") );
		MatchFeature(single_st, plugin);
		break;					}
	
	case ID_COLON:				{
		stdext::auto_interface<IPlugin>	plugin;
		m_plugin_container->GetPlugin(_T(""), plugin);
		JCASSERT( plugin.valid() );
		NextToken(m_lookahead);
		MatchFeature(single_st, plugin);
		break;					}

	default:
		match = false;
		SYNTAX_ERROR(_T("Missing module name or :"));
		return false;
	}
	
	combo->AddOp(single_st);
	if (chain) chain->Release(), chain = NULL;
	single_st.detach(chain);
	return match;
}

void CSyntaxParser::MatchFeature(CSingleSt * single_st, IPlugin * plugin)
{
	LOG_STACK_TRACE();
	JCASSERT(single_st);
	JCASSERT(plugin);

	LOG_COMPILE(_T("- feature: %s  "), m_lookahead.m_str.c_str() );

	// compile feature name
	stdext::auto_interface<IFeature>	feature;
	if (ID_WORD != m_lookahead.m_id) SYNTAX_ERROR(_T("Missing feature name") );
	plugin->GetFeature(m_lookahead.m_str, feature);
	if ( !feature )	SYNTAX_ERROR(_T("%s is not a feature of plugin %s"), 
		m_lookahead.m_str.c_str(), plugin->name() ); 
	NextToken(m_lookahead);

	stdext::auto_interface<CFeatureWrapper>	wrapper( new CFeatureWrapper(feature) );
	
	wrapper->SetOutPort( static_cast<IOutPort*>(single_st) );

	MatchCmdSet( single_st, feature, 0);	
	MatchParamSet( single_st, feature );

	CInPort * inport = single_st->GetInPort();
	if (inport)		wrapper->SetSource(0, inport);
	single_st->AddOp( static_cast<IAtomOperate*>(wrapper) );
}

void CSyntaxParser::MatchFilterSt(CSingleSt * single_st)
{
	LOG_STACK_TRACE();
	JCASSERT(single_st);

	NextToken(m_lookahead);
	stdext::auto_interface<CFilterSt> ft(new CFilterSt(dynamic_cast<IOutPort*>(single_st) ) );
	// 表格source在添加到loop中时，自动设置
	IAtomOperate * chain = static_cast<IAtomOperate*>( single_st->GetInPort() );
	if (NULL == chain)	SYNTAX_ERROR(_T("missing source for filter"));

	ft->SetSource(CFilterSt::SRC_TAB,  chain);
	stdext::auto_interface<IAtomOperate> op;
	MatchBinaryOperate<COpMakerBoolExp>(static_cast<CSequenceOp*>(single_st), op);

	ft->SetSource(CFilterSt::SRC_EXP, op);
	single_st->AddOp(static_cast<IAtomOperate*>(ft) );
}

bool CSyntaxParser::MatchCmdSet(CSingleSt * single_st, IFeature * feature, int index)
{
	LOG_STACK_TRACE();

	// 右递归简化为循环
	while (1)
	{
		static const JCSIZE DEFAULT_PARAM_NAME_LEN = 4;
		TCHAR param_name[DEFAULT_PARAM_NAME_LEN];
		stdext::jc_sprintf(param_name, _T("#%02X"), index);

		LOG_COMPILE(_T("- cmd: %s  "), param_name );
		if ( !MatchPV2PushParam(single_st, feature, param_name) ) return true;
		++ index;
	}
	return true;
}

void CSyntaxParser::MatchParamSet(CSingleSt * single_st, IFeature * feature)
{
	LOG_STACK_TRACE();

	while (1)
	{
		switch (m_lookahead.m_id)
		{
		case ID_PARAM_TABLE:
			SYNTAX_ERROR(_T("do not suporot --@"));
			break;

		case ID_PARAM_NAME: {
			CJCStringT param_name = m_lookahead.m_str;
			LOG_DEBUG(_T("matched parameter name --%s"), param_name.c_str() );
			NextToken(m_lookahead);
			if (ID_EQUAL == m_lookahead.m_id)
			{
				LOG_DEBUG(_T("parsing parameter value") );
				NextToken(m_lookahead);
				if ( ! MatchPV2PushParam(single_st, feature, param_name) )	SYNTAX_ERROR( _T("Missing parameter value") );
			}
			else	SetBoolOption(feature, param_name);
			break; }

		case ID_ABBREV_BOOL:	{
			LOG_DEBUG(_T("matched abbrev bool %c"), (TCHAR)m_lookahead.m_val );
			CJCStringT param_name;
			CheckAbbrev(feature, (TCHAR)m_lookahead.m_val, param_name);
			SetBoolOption(feature, param_name);
			NextToken(m_lookahead);
			continue;				}

		case ID_ABBREV:	{
			LOG_DEBUG(_T("matched abbrev %c"), (TCHAR)m_lookahead.m_val );
			CJCStringT param_name;
			CheckAbbrev(feature, (TCHAR)m_lookahead.m_val, param_name);
			NextToken(m_lookahead);
			if ( ! MatchPV2PushParam(single_st, feature, param_name) )	SYNTAX_ERROR(_T("Missint parameter value") );
			break;					}

		default:
			return;
		}
	}
}

bool CSyntaxParser::MatchParamVal2(CSequenceOp * combo, IAtomOperate * & exp)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == exp);

	bool match = true;

	switch (m_lookahead.m_id)
	{
	// constant
	case ID_STRING:
	case ID_NUMERAL:		{
		LOG_COMPILE(_T("- constant ") );
		MatchConstant(exp);
		combo->AddOp(exp);
		break; }

	case ID_FILE_NAME: {	// 文件名作为参数。设置文件名为#filename参数
		LOG_COMPILE(_T("- file ") )
		exp = static_cast<IAtomOperate*>( new CConstantOp<CJCStringT>(m_lookahead.m_str) );
		combo->AddOp(exp);
		NextToken(m_lookahead);
		break; }				

	case ID_VARIABLE:
		LOG_COMPILE(_T("- variable ") )
		NextToken(m_lookahead);
		MatchV3(combo, exp);
		break;

	default:
		// error
		LOG_COMPILE(_T("- do not match p-val"));
		match = false;
		break;
	}
	return match;
}

void CSyntaxParser::MatchConstant(IAtomOperate * & val)
{	// 终结符
	LOG_STACK_TRACE();
	JCASSERT(NULL == val);

	switch (m_lookahead.m_id)
	{
	// constant
	case ID_STRING:
		val = static_cast<IAtomOperate*>(
			new CConstantOp<CJCStringT>(m_lookahead.m_str) );
		NextToken(m_lookahead);
		break;	
	
	case ID_NUMERAL:
		//TODO 优化：根据值的大小选择类型
		val = static_cast<IAtomOperate*>( new CConstantOp<INT64>(m_lookahead.m_val) );
		NextToken(m_lookahead);
		break;
	}
}

void CSyntaxParser::MatchV3(CSequenceOp * combo, IAtomOperate * & op)
{
	LOG_STACK_TRACE();
	JCASSERT(combo);
	JCASSERT(NULL == op);

	switch (m_lookahead.m_id)
	{
	case ID_WORD:
		MatchVariable(combo, op);
		break;
	default:
		return;
	}
}

bool CSyntaxParser::MatchTableParam(CSequenceOp * combo, IChainOperate * & chain)
{
	LOG_STACK_TRACE();

	Match(ID_TABLE, _T("") );

	switch (m_lookahead.m_id)
	{
	case ID_CURLY_BRACKET_1:	{
		Match(ID_CURLY_BRACKET_1, _T(""));
		MatchSingleSt(combo, chain);
		Match(ID_CURLY_BRACKET_2, _T("missing }"));
		break; }

	default:		{
		stdext::auto_interface<IAtomOperate> var_op;
		MatchV3(combo, var_op);
		combo->AddOp(var_op);

		stdext::auto_interface<IAtomOperate>	loop_var( static_cast<IAtomOperate*>(new CLoopVarOp ));
		loop_var->SetSource(0, var_op);
		combo->AddOp(loop_var);
		loop_var.detach(chain);
		break;		}
	}
	return true;
}

bool CSyntaxParser::MatchTableSt(CSequenceOp * combo, IChainOperate * & chain)
{
	LOG_STACK_TRACE();

	Match(ID_TABLE, _T("") );
	switch (m_lookahead.m_id)
	{
	case ID_CURLY_BRACKET_1:	{
		Match(ID_CURLY_BRACKET_1, _T(""));
		MatchSingleSt(combo, chain);
		Match(ID_CURLY_BRACKET_2, _T("missing }"));	
		break; }

	default:		{
		stdext::auto_interface<IAtomOperate> var_op;
		MatchV3(combo, var_op);
	
		stdext::auto_interface<CSingleSt> single_st(new CSingleSt(combo) );

		stdext::auto_interface<CLoopVarOp>	loop_var( new CLoopVarOp );
		loop_var->SetSource(0, var_op);
		loop_var->SetOutPort(single_st.d_cast<IOutPort*>());
		single_st->AddOp(loop_var);

		combo->AddOp(single_st);
		single_st.detach(chain);
		break;		}
	}
	return true;
}

bool CSyntaxParser::MatchVariable(CSequenceOp * combo, IAtomOperate * & op)
{
	LOG_STACK_TRACE();
	JCASSERT(combo);

	JCASSERT(NULL == op);

	stdext::auto_interface<CVariableOp> vop;
	
	if (ID_WORD == m_lookahead.m_id)
	{
		vop = new CVariableOp(m_lookahead.m_str);	
		//!! implement m_var_set in tester
		vop->SetSource(0, m_var_set);
		combo->AddOp(vop);

		NextToken(m_lookahead);
		vop.detach(op);
	}
	else	SYNTAX_ERROR(_T("expected a variable name."));

	// MatchV1
	// 循环代替右递归
	bool loop = true;
	while (loop)
	{
		switch (m_lookahead.m_id)
		{
		case ID_SQUARE_BRACKET_1:
			SYNTAX_ERROR(_T("unsupport"));
			break;

		case ID_DOT:	{
			NextToken(m_lookahead);		// Match(ID_DOT)
			if (ID_WORD != m_lookahead.m_id)	SYNTAX_ERROR(_T("missing variable name"));
			vop = new CVariableOp(m_lookahead.m_str);
			vop->SetSource(0, op);
			combo->AddOp(vop);
			op->Release(), op = NULL;
			vop.detach(op);
			NextToken(m_lookahead);
			break;	}

		default:
			loop = false;
			break;
		}
	}
	return true;
}

void CSyntaxParser::MatchHelp(IAtomOperate * & op)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == op);
}

bool CSyntaxParser::MatchRelationFactor(CSequenceOp * combo, IAtomOperate * & op)
{
	LOG_STACK_TRACE();
	bool match = false;

	switch (m_lookahead.m_id)
	{
	case ID_BRACKET_1:
		NextToken(m_lookahead);
		match = MatchBinaryOperate<COpMakerBoolExp>(combo, op);
		if (!match) SYNTAX_ERROR(_T("expected a bool expression"));
		Match(ID_BRACKET_2, _T("expect ')'"));
		break;

	case ID_BOOLOP_NOT:	{
		NextToken(m_lookahead);
		stdext::auto_interface<IAtomOperate> this_op;
		this_op = static_cast<IAtomOperate*>(new CBoolOpNot);
		match = MatchRelationFactor(combo, op);
		if (!match) SYNTAX_ERROR(_T("expected a bool expression"));
		this_op->SetSource(0, op);
		combo->AddOp(this_op);
		op->Release(), op=NULL;
		this_op.detach<IAtomOperate>(op);
		break;	}

	default:
		match = MatchRelationExp(combo, op);
	}
	return match;
}

bool CSyntaxParser::MatchRelationExp(CSequenceOp * combo, IAtomOperate * & op)
{
	LOG_STACK_TRACE();
	JCASSERT(combo);
	JCASSERT(NULL == op);

	// 左值
	stdext::auto_interface<IAtomOperate> exp_l;
	bool match = MatchExpression(combo, exp_l);
	if ( !match ) return false;

	// 符号
	if (ID_REL_OP != m_lookahead.m_id)	SYNTAX_ERROR(_T("missing relation op"));
	TOKEN_ID op_id = (TOKEN_ID)(m_lookahead.m_val);
	NextToken(m_lookahead);

	// 右值
	stdext::auto_interface<IAtomOperate> exp_r;
	match = MatchExpression(combo, exp_r);
	if ( !match ) SYNTAX_ERROR( _T("expected an expression") );

	switch ( op_id )
	{
	case ID_RELOP_LT:		// exp_l < exp_r
		op = static_cast<IAtomOperate*>(new CRelOpLT);
		op->SetSource(0, exp_l);
		op->SetSource(1, exp_r);
		break;

	case ID_RELOP_GT:		// exp_l > exp_r
		op = static_cast<IAtomOperate*>(new CRelOpLT);
		op->SetSource(0, exp_r);
		op->SetSource(1, exp_l);
		break;

	case ID_RELOP_LE:		// exp_l <= exp_r
		op = static_cast<IAtomOperate*>(new CRelOpLE);
		op->SetSource(0, exp_l);
		op->SetSource(1, exp_r);
		break;

	case ID_RELOP_GE:		// exp_l >= exp_r
		op = static_cast<IAtomOperate*>(new CRelOpLE);
		op->SetSource(0, exp_r);
		op->SetSource(1, exp_l);
		break;

	case ID_RELOP_EE:
		op = static_cast<IAtomOperate*>(new CRelOpEQ);
		op->SetSource(0, exp_l);
		op->SetSource(1, exp_r);
		break;

	default:
		SYNTAX_ERROR(_T("unknown relation operate"));
		break;
	}
	combo->AddOp(op);
	return true;
}

// 解析一个表达式可能返回下列结果之一
// 常数：返回IValue
// 变量：返回变量所在的位置
// 计算过程：返回一个IAtomOperate

bool CSyntaxParser::MatchExpression(CSequenceOp * combo, IAtomOperate * & op)
{
	// 解析 : 运算
	return MatchTermVec(combo, op);
}

bool CSyntaxParser::MatchTermVec(CSequenceOp * single_st, IAtomOperate * & op)
{	// 解析 : 运算
	JCASSERT(NULL == op);

	// 值1
	bool match = MatchBinaryOperate<COpMakerAddSub>(single_st, op);
	if (!match) return false;

	JCASSERT(op);
	if (ID_COLON == m_lookahead.m_id)
	{
		IOutPort * outport = dynamic_cast<IOutPort*>(single_st);
		if (NULL == outport) SYNTAX_ERROR( _T("vecotr maker can only used in single st.") );
		NextToken(m_lookahead);
		stdext::auto_interface<COpVectorMaker> vector_op(new COpVectorMaker);
		vector_op->SetOutPort(outport);

		vector_op->SetSource(0, op);
		op->Release();	op = NULL;

		// 值2
		stdext::auto_interface<IAtomOperate> exp_r;
		match = MatchBinaryOperate<COpMakerAddSub>(single_st, exp_r);
		if (!match) SYNTAX_ERROR( _T("syntax error on right side :") );
		vector_op->SetSource(1, exp_r);

		if (ID_COLON == m_lookahead.m_id)
		{
			NextToken(m_lookahead);
			// 值3
			stdext::auto_interface<IAtomOperate> exp_3;
			match = MatchBinaryOperate<COpMakerAddSub>(single_st, exp_3);
			if (!match) SYNTAX_ERROR( _T("syntax error on right side :") );
			vector_op->SetSource(2, exp_3);		
		}

		single_st->AddOp(vector_op);
		vector_op.detach(op);
	}
	return true;
}


class COpMakerBitOp
{
public:
	inline static CSyntaxParser::BINARY_OP NextTerm() {return &CSyntaxParser::MatchFactor;};
	inline static bool Make(TOKEN_ID id, INT64 val, IAtomOperate * & op)
	{
		if (ID_ATHOP_BITOP != id) return false;
		switch (val)
		{
		case _T('&'):	op = static_cast<IAtomOperate*>(new CAthOpBase<CAthAnd>); return true;
		case _T('|'):	op = static_cast<IAtomOperate*>(new CAthOpBase<CAthOr>); return true;
		case _T('^'):	op = static_cast<IAtomOperate*>(new CAthOpBase<CAthXor>); return true;
		default:		return false;
		}
	}
};

class COpMakerMulDiv
{
public:
	inline static CSyntaxParser::BINARY_OP NextTerm() {return &CSyntaxParser::MatchBinaryOperate<COpMakerBitOp>;};
	inline static bool Make(TOKEN_ID id, INT64 val, IAtomOperate * & op)
	{
		if (ID_ATHOP_MUL_DIV != id) return false;
		switch (val)
		{
		case _T('*'):	op = static_cast<IAtomOperate*>(new CAthOpBase<CAthMul>); return true;
		case _T('/'):	op = static_cast<IAtomOperate*>(new CAthOpBase<CAthDiv>); return true;
		default:		return false;
		}
	}
};

class COpMakerAddSub
{
public:
	inline static CSyntaxParser::BINARY_OP NextTerm() {return &CSyntaxParser::MatchBinaryOperate<COpMakerMulDiv>;};
	inline static bool Make(TOKEN_ID id, INT64 val, IAtomOperate * & op)
	{
		if (ID_ATHOP_ADD_SUB != id) return false;
		switch (val)
		{
		case _T('+'):	op = static_cast<IAtomOperate*>(new CAthOpBase<CAthAdd>); return true;
		case _T('-'):	op = static_cast<IAtomOperate*>(new CAthOpBase<CAthSub>); return true;
		default:		return false;
		}
	}
};

class COpMakerBoolExp
{
public:
	inline static CSyntaxParser::BINARY_OP NextTerm() {return &CSyntaxParser::MatchRelationFactor;};
	inline static bool Make(TOKEN_ID id, INT64 val, IAtomOperate * & op)
	{
		switch (id)
		{
		case ID_BOOLOP_AND:	op = static_cast<IAtomOperate*>(new CBoolOpAnd); return true;
		case ID_BOOLOP_OR:	op = static_cast<IAtomOperate*>(new CBoolOpOr);	return true;
		default:		return false;
		}
	}
};

bool CSyntaxParser::MatchFactor(CSequenceOp * combo, IAtomOperate * & exp)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == exp);
	JCASSERT(combo);

	CSingleSt * st = dynamic_cast<CSingleSt*>(combo);
	switch (m_lookahead.m_id)
	{
	case ID_STRING:
	case ID_NUMERAL:
		LOG_COMPILE(_T("- constant ") );
		MatchConstant(exp);
		combo->AddOp(exp);
		break;

	case ID_FILE_NAME: {	// 文件名作为参数。设置文件名为#filename参数
		LOG_COMPILE(_T("- file ") )
		exp = static_cast<IAtomOperate*>( new CConstantOp<CJCStringT>(m_lookahead.m_str) );
		combo->AddOp(exp);
		NextToken(m_lookahead);
		break; }	

	case ID_CURLY_BRACKET_1:	{
		Match(ID_CURLY_BRACKET_1, _T("Missing {") );
		if (ID_NUMERAL == m_lookahead.m_id)
		{
			SYNTAX_ERROR(_T("unsupport make vector"));
		}
		else
		{
			stdext::auto_interface<CComboSt>	new_combo;
			MatchComboSt(new_combo);
			new_combo.detach<IAtomOperate>(exp);
		}
		Match(ID_CURLY_BRACKET_2, _T("Missing }"));
		break;					}

	case ID_BRACKET_1:
		NextToken(m_lookahead);
		MatchExpression(combo, exp);
		Match(ID_BRACKET_2, _T("Missing )") );
		break;

	case ID_VARIABLE:
		LOG_COMPILE(_T("- variable ") )
		NextToken(m_lookahead);	//Match(ID_VARIABLE);
		MatchVariable(combo, exp);
		break;

	case ID_PREDEF_VAR:		{
		if (m_lookahead.m_str == _T("INDEX"))
		{
			if (!st) SYNTAX_ERROR(_T("Pre defined variable must in single st"));
			IAtomOperate * chain = static_cast<IAtomOperate*>(st->GetInPort());
			if (NULL == chain) SYNTAX_ERROR(_T("missing source for INDEX"));

			CPdvIndex * index_op = new CPdvIndex();
			exp = static_cast<IAtomOperate*>(index_op);
			exp->SetSource(0, chain);
			st->AddOp(exp);
		}
		else SYNTAX_ERROR(_T("Unknown pre defined variable %s"), m_lookahead.m_str.c_str() );
		NextToken(m_lookahead);
		break;					}

	case ID_DOT:	{
		NextToken(m_lookahead);			//Match(ID_DOT, _T("missing ."));
		// match column value
		if (!st) SYNTAX_ERROR(_T("\".\" operator must in single st"));
		IAtomOperate * chain = static_cast<IAtomOperate*>(st->GetInPort());
		if (NULL == chain) SYNTAX_ERROR(_T("missing source for column %s"), m_lookahead.m_str.c_str());

		if (ID_WORD == m_lookahead.m_id)
		{
			exp = static_cast<IAtomOperate*>( new CColumnVal(m_lookahead.m_str) );
			exp->SetSource(0, chain);
			st->AddOp(exp);
			NextToken(m_lookahead);
		}
		else
		{
			exp = chain;
			exp->AddRef();
		}
		break;	}

	default:
		LOG_COMPILE(_T("- do not match factor"));
		return false;
	}
	return true;
}

template <class OP_MAKER>
bool CSyntaxParser::MatchBinaryOperate(CSequenceOp * combo, IAtomOperate * & op)
{
	// 解析加减法运算
	LOG_STACK_TRACE();
	JCASSERT(NULL == op);

	// 左值
	bool match = (this->*(OP_MAKER::NextTerm()))(combo, op);
	if (!match) return false;

	while (1)
	{
		// Match('+' / '-')
		stdext::auto_interface<IAtomOperate>	this_op;
		match = OP_MAKER::Make(m_lookahead.m_id, m_lookahead.m_val, this_op);
		if ( !match ) break;

		NextToken(m_lookahead);
		this_op->SetSource(0, op);

		// 右值
		stdext::auto_interface<IAtomOperate> exp_r;
		match = (this->*(OP_MAKER::NextTerm()))(combo, exp_r);
		if ( !match )  SYNTAX_ERROR( _T("syntax error on right side of exp") );

		this_op->SetSource(1, exp_r);
		combo->AddOp(this_op);

		op->Release(); op=NULL;
		this_op.detach<IAtomOperate>(op);
	}
	return true;
}

//void CSyntaxParser::MatchIfSt( CSequenceOp * prog)
//{
//	NextToken(m_lookahead);		// skip keyword if
//
//	stdext::auto_interface<CStIf> ifop = new CStIf;
//	stdext::auto_interface<IAtomOperate> boolexp;
//	MatchBoolExpression(prog, boolexp);
//	ifop->SetSource(0, boolexp);
//	Match(ID_KEY_THEN, _T("missing then"));
//	Match(ID_NEW_LINE, _T(""));
//
//	MatchScript( ifop->GetSubSequence(true) );
//
//	if (ID_KEY_ELSE == m_lookahead.m_id)
//	{
//		NextToken(m_lookahead);
//		Match(ID_NEW_LINE, _T(""));
//		MatchScript( ifop->GetSubSequence(false) );
//	}
//	Match(ID_KEY_END, _T(""));
//	Match(ID_NEW_LINE, _T(""));
//}


void CSyntaxParser::SetBoolOption(IFeature * feature, const CJCStringT & param_name)
{
	JCASSERT(feature);
	stdext::auto_interface<jcparam::IValue> val;
	val = static_cast<jcparam::IValue*>(jcparam::CTypedValue<bool>::Create(true));
	feature->PushParameter(param_name, val);
}

bool CSyntaxParser::CheckAbbrev(IFeature * func, TCHAR abbr, CJCStringT & param_name)
{
	JCASSERT(func);
	bool br = func->CheckAbbrev(abbr, param_name);
	if (!br) SYNTAX_ERROR(_T("Abbrev %c is not defined."), abbr);
	return br;
}

bool CSyntaxParser::MatchPV2PushParam(CSingleSt * single_st,
		IFeature * feature, const CJCStringT & param_name)
{
	// 该函数只在编译参数时被调用
	LOG_STACK_TRACE();
	JCASSERT(single_st);
	JCASSERT(feature);

	bool constant = false, is_file=false;

	stdext::auto_interface<CPushParamOp>	push_op( new CPushParamOp(feature, param_name) );

	if (ID_TABLE == m_lookahead.m_id)
	{
		CSequenceOp * combo = single_st->GetSuper();
		JCASSERT( dynamic_cast<CComboSt*>(combo) );

		stdext::auto_interface<IChainOperate>	chain;
		bool br = MatchTableParam(combo, chain);
		if (chain)	single_st->SetSource(0, chain);
		push_op->SetSource(0, single_st->GetInPort());
	}
	else
	{
		stdext::auto_interface<IAtomOperate> expr_op;
		is_file = (m_lookahead.m_id == ID_FILE_NAME);
		if ( MatchFactor(single_st, expr_op) )
		{
			if (is_file && _T('#') == param_name.at(0) ) push_op->SetParamName(_T("#filename"));
			push_op->SetSource(0, expr_op);
		}
		else	return false;
	}
	single_st->AddOp( static_cast<IAtomOperate*>(push_op) );
	return true;
}

void CSyntaxParser::OnError(LPCTSTR msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);

	LPTSTR err_msg = new TCHAR[ERROR_MSG_BUFFER];
	stdext::jc_vsprintf(err_msg, ERROR_MSG_BUFFER, msg, argptr);
	JCSIZE col = (UINT)(m_first - m_line_begin);

	LOG_ERROR(_T("syntax error! l:%d, c:%d, %s"), m_line_num, col, err_msg);	

	if (m_error_handler)	m_error_handler->OnError(m_line_num, col, err_msg);
	CSyntaxError err(m_line_num, col, err_msg);
	delete [] err_msg;
	throw err;
}

///////////////////////////////////////////////////////////////////////////////
// -- export functions
bool jcscript::CreateVarOp(jcparam::IValue * val, IAtomOperate * &op)
{
	JCASSERT(NULL == op);
	op = static_cast<IAtomOperate*>(new CVariableOp(val));
	return true;
}

bool jcscript::Parse(IPluginContainer * plugin_container, LSyntaxErrorHandler * err_handler, LPCTSTR &str, LPCTSTR last, IAtomOperate * & script)
{
	CSyntaxParser	syntax_parser(plugin_container, err_handler);
	syntax_parser.Parse(str, last);
	syntax_parser.MatchScript(script);
	return syntax_parser.GetError();
}

bool jcscript::Parse(IPluginContainer * plugin_container, LSyntaxErrorHandler * err_handler, FILE * file, IAtomOperate * & script)
{
	CSyntaxParser	syntax_parser(plugin_container, err_handler);
	syntax_parser.Source(file);
	syntax_parser.MatchScript(script);
	return syntax_parser.GetError();
}
	//void Parse(jcparam::IJCStream * stream);
