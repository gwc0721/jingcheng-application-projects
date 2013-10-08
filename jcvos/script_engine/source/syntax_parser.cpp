#include "stdafx.h"

#include <boost/spirit/include/lex_lexertl.hpp>
#include "../include/syntax_parser.h"
#include "atom_operates.h"
#include "expression_op.h"

// LOG 输出定义：
//  WARNING:	编译错误
//	NOTICE:		输出编译中间结果
//  TRACE:		调用堆栈

LOCAL_LOGGER_ENABLE(_T("Syntax_parser"), LOGGER_LEVEL_WARNING);

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
			(_T("\\|"),					ID_CONNECT)			// 命令连接
			(_T("\\$"),					ID_VARIABLE)		// 变量
			(_T("\\@"),					ID_TABLE)		// 变量
			(_T("\\="),					ID_EQUAL)			// 等号
			(_T("\\/[^\\/]+\\/"),				ID_FILE_NAME)		// 双引号
			(_T("\\\"[^\\\"]+\\\""),	ID_QUOTATION1)		// 双引号
			//(_T("\\'[^\\']+\\'"),		ID_QUOTATION2)		// 单引号
			(_T("\\-\\-[_a-zA-Z][_a-zA-Z0-9]*"),		ID_PARAM_NAME)		// 参数名称
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
			m_property->m_val = (UINT)stdext::str2hex(t_begin+2);
			m_property->m_id = ID_NUMERAL;
			break;

		case ID_WORD:
			m_property->m_str = CJCStringT(t_begin, 0, t_len);	
			break;

		case ID_ABBREV:
		case ID_ABBREV_BOOL:
			m_property->m_val = t_begin[1];
			break;

		case ID_PARAM_NAME:
			m_property->m_str = CJCStringT(t_begin+2, 0, t_len-2);		//	除去缩写前的--
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
/*
#define SYNTAX_ERROR(...)	{ \
		LPTSTR __temp_str = new TCHAR[512];		\
		stdext::jc_sprintf(__temp_str, 512, __VA_ARGS__);	\
		OnError(__temp_str);			\
		CSyntaxError err(__temp_str); \
		delete [] __temp_str;					\
        throw err; }
*/

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
}

void CSyntaxParser::Source(FILE * file)
{
	LOG_STACK_TRACE();
	JCASSERT(file);
	m_one_line = false;

	m_file = file;

	m_line_num = 1;
	m_line_begin = m_first;

	m_src_buf = new TCHAR[SRC_BUF_SIZE];
	m_first = m_src_buf;
	m_last = m_src_buf;
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

	NextToken(m_lookahead);
}

// for unicode file
//void CSyntaxParser::ReadSource(void)
//{
//	JCSIZE len = m_last - m_first;
//	if ( (len < SRC_BACK_SIZE) && m_file && (!feof(m_file)) )
//	{
//		// move tail to begin
//		memcpy_s(m_src_buf, (m_first - m_src_buf), m_first, len); 
//		m_first = m_src_buf; 
//		m_last = m_src_buf + len;
//		// read from file
//		JCSIZE read_size = fread(m_last, SRC_READ_SIZE, 1 m_file);
//		m_last += read_size;
//	}
//}

// for ASCII file
void CSyntaxParser::ReadSource(void)
{
#define READ_ASCII_SIZE		(SRC_READ_SIZE /2)

	JCSIZE len = m_last - m_first;
	TCHAR * last;


	if (m_file && !feof(m_file) )
	{
		// move tail to begin
		memcpy_s(m_src_buf, (m_first - m_src_buf), m_first, len); 
		m_first = m_src_buf; 
		last = m_src_buf + len;

		// read from file
		char * file_buf = new char[READ_ASCII_SIZE];
		JCSIZE read_size = fread(file_buf, 1, READ_ASCII_SIZE, m_file);
		JCSIZE buf_remain = SRC_BUF_SIZE - len;
		JCSIZE conv_size = stdext::Utf8ToUnicode(last, buf_remain, file_buf, read_size);
		m_last = last + conv_size;
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
	prop.m_id = ID_SYMBO_ERROR;
	Token(prop);
	LOG_TRACE(_T("[TRACE] id=%d, val=%d, str=%s"), prop.m_id, (UINT)(prop.m_val), prop.m_str.c_str())

	if (ID_SYMBO_ERROR == prop.m_id)
		SYNTAX_ERROR(_T("Unknow symbo '%c'"), (TCHAR)(prop.m_val) );
}

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

///////////////////////////////////////////////////////////////////////////////

bool CSyntaxParser::MatchScript(IAtomOperate * & op)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == op);

	stdext::auto_interface<CScriptOp>	prog;
	CScriptOp::Create(prog);

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
				stdext::auto_interface<IAtomOperate> op;
				Match(ID_KEY_HELP, _T("") ); MatchHelp(op);
				prog->PushBackAo(op);
				break;	}

			case ID_KEY_EXIT: {
				Match(ID_KEY_EXIT, _T("") );
				CExitOp * op = new CExitOp;
				prog->PushBackAo(static_cast<IAtomOperate*>(op) );
				op->Release();
				break;	}

			//case ID_KEY_IF: {
			//	MatchIfSt(prog);
			//				}

			default:	{
				stdext::auto_interface<CComboStatement>	combo;
				MatchComboSt(combo);
				prog->Merge(combo);
				break;	}
			}
		}
		catch (CSyntaxError & )
		{
			m_syntax_error = true;
			while (m_lookahead.m_id != ID_NEW_LINE && m_lookahead.m_id != ID_EOF) 
				Token(m_lookahead);
		}
	}
	prog.detach(op);
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


bool CSyntaxParser::MatchComboSt(CComboStatement * & combo)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == combo);
	CComboStatement::Create(combo);
#ifdef _DEBUG
	// 用于编译信息输出
	combo->m_line = m_line_num;
#endif

	stdext::auto_interface<IAtomOperate> op;
	bool constant = false, file=false;

	switch (m_lookahead.m_id)
	{
	case ID_KEY_FILTER:
	case ID_WORD:
	case ID_COLON:
		MatchSingleSt(combo, op);
		LOG_DEBUG(_T("matched single_st"));
		combo->AddLoopOp(op);
		op->SetSource(0, combo->LastChain( op ) );
		MatchS1(combo);
		break;

	case ID_TABLE: {
		stdext::auto_interface<CComboStatement> new_combo;
		MatchTable(combo, new_combo);
		// 一定是Combo的第一个语句，不需要设source.
		stdext::auto_interface<IAtomOperate> last;
		bool br = combo->Merge(new_combo, last);
		if (!br) SYNTAX_ERROR(_T("table variable has been already set") );
		combo->LastChain(last);
		MatchS1(combo);
		break;	   }

	default:
		if ( MatchParamVal2(op, constant, file) )
		{
			LOG_DEBUG(_T("matched variable"));
			combo->AddPreproOp(op);
			combo->LastChain(op);
			MatchS1(combo);
			return true;
		}
		else 
		{
			LOG_DEBUG(_T("do not match anything"));
			return false;
		}
	}
	return true;
}

void CSyntaxParser::MatchS1(CComboStatement * combo)
{
	LOG_STACK_TRACE();
	JCASSERT(combo);

	bool assign_set = false;
	while (1)
	{
		// 右递归简化为循环
		if (ID_CONNECT == m_lookahead.m_id)
		{
			stdext::auto_interface<IAtomOperate> proxy;
			NextToken(m_lookahead);		// Match(ID_CONNECT);
			if ( !MatchSingleSt(combo, proxy) )
				SYNTAX_ERROR(_T("Missing single statment") );
		}
		else if (ID_ASSIGN == m_lookahead.m_id)
		{
			stdext::auto_interface<IAtomOperate> op;
			MatchAssign(combo, op);
			assign_set = true;
			break;
		}
		else break;
	}

	if (!assign_set)
	{
		CAssignOp * op = new CAssignOp(m_var_set, _T("res"));
		combo->SetAssignOp(op);
		op->Release();
	}

	// 编译后处理
	combo->CompileClose();
}

bool CSyntaxParser::MatchAssign(CComboStatement * combo, IAtomOperate * & op)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == op);
	JCASSERT(combo);

	Match(ID_ASSIGN, _T("Missing =>") );
	switch (m_lookahead.m_id)
	{
	case ID_WORD:	{
		op = static_cast<IAtomOperate*>(new CAssignOp(m_var_set, m_lookahead.m_str));
		combo->SetAssignOp(op);
		op->Release();
		op = NULL;
		NextToken(m_lookahead);
		break;		}

	case ID_FILE_NAME:	{		// 文件名作为作值(assignee)，将结果保存到文件
		op = static_cast<IAtomOperate*>(new CSaveToFileOp(m_lookahead.m_str));
		combo->AddLoopOp(op);
		op->SetSource(0, combo->LastChain( op ) );
		NextToken(m_lookahead);
		break;			}

	default:
		SYNTAX_ERROR(_T("missing variable name."));
		return false;
	}
	return true;
}

bool CSyntaxParser::MatchSingleSt(CComboStatement * combo, IAtomOperate * & op)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == op);

	stdext::auto_interface<IPlugin>	plugin;
	stdext::auto_interface<IFeature> feature;

	m_default_param_index = 0;

	bool match = false;
	switch (m_lookahead.m_id)
	{
	case ID_FILE_NAME:	{		// 文件名作为作值(assignee)，将结果保存到文件
		op = static_cast<IAtomOperate*>(new CSaveToFileOp(m_lookahead.m_str));
		combo->AddLoopOp(op);
		op->SetSource(0, combo->LastChain( op ) );
		NextToken(m_lookahead);
		break;			}	

	case ID_KEY_FILTER:	{
		stdext::auto_interface<CFilterSt>	ft;
		MatchFilterSt(combo, ft);
		ft.detach<IAtomOperate>(op);
		match = true;
		break;		}

	case ID_WORD:				{
		// WORD : WORD
		stdext::auto_interface<IPlugin>	plugin;
		stdext::auto_interface<IFeature> feature;
		m_plugin_container->GetPlugin(m_lookahead.m_str, plugin);
		if ( ! plugin ) SYNTAX_ERROR(_T("Unknow plugin %s "), m_lookahead.m_str.c_str() );
		NextToken(m_lookahead);
		Match(ID_COLON, _T("expected :") );
		MatchFeature(combo, plugin, feature);
		feature.detach<IAtomOperate>(op);
		match = true;
		break;					}
	
	case ID_COLON:				{
		stdext::auto_interface<IPlugin>	plugin;
		stdext::auto_interface<IFeature> feature;
		m_plugin_container->GetPlugin(_T(""), plugin);
		JCASSERT( plugin.valid() );
		NextToken(m_lookahead);
		MatchFeature(combo, plugin, feature);
		feature.detach<IAtomOperate>(op);
		match = true;
		break;					}

	default:
		match = false;
		SYNTAX_ERROR(_T("Missing module name or :"));
	}
	return match;
}

void CSyntaxParser::MatchFeature(CComboStatement * combo, IPlugin * plugin, IFeature * &feature)
{
	LOG_STACK_TRACE();
	JCASSERT(combo);
	JCASSERT(plugin);
	JCASSERT(NULL == feature);

	if (ID_WORD != m_lookahead.m_id) SYNTAX_ERROR(_T("Missing feature name") );
	plugin->GetFeature(m_lookahead.m_str, feature);
	if ( !feature )	SYNTAX_ERROR(_T("%s is not a feature of plugin %s"), 
		m_lookahead.m_str.c_str(), plugin->name() ); 
	
	NextToken(m_lookahead);
	MatchCmdSet(combo, feature, 0);	
	MatchParamSet(combo, feature);
	
	combo->AddLoopOp(static_cast<IAtomOperate*>(feature) );
	feature->SetSource(0, combo->LastChain( feature ) );
}

bool CSyntaxParser::MatchCmdSet(CComboStatement * combo, IFeature * func, int index)
{
	LOG_STACK_TRACE();

	// 右递归简化为循环
	while (1)
	{
		static const JCSIZE DEFAULT_PARAM_NAME_LEN = 4;
		TCHAR param_name[DEFAULT_PARAM_NAME_LEN];
		stdext::jc_sprintf(param_name, _T("#%02X"), index);

		if ( !MatchPV2PushParam(combo, func, param_name) ) return true;
		++ index;
	}
	return true;
}

void CSyntaxParser::MatchParamSet(CComboStatement * combo, IFeature * proxy)
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
				if ( ! MatchPV2PushParam(combo, proxy, param_name) )	
				{
					SYNTAX_ERROR( _T("Missing parameter value") );
				}
			}
			else	SetBoolOption(proxy, param_name);
			break; }

		case ID_ABBREV_BOOL:	{
			LOG_DEBUG(_T("matched abbrev bool %c"), (TCHAR)m_lookahead.m_val );
			CJCStringT param_name;
			CheckAbbrev(proxy, (TCHAR)m_lookahead.m_val, param_name);
			SetBoolOption(proxy, param_name);
			NextToken(m_lookahead);
			continue;				}

		case ID_ABBREV:	{
			LOG_DEBUG(_T("matched abbrev %c"), (TCHAR)m_lookahead.m_val );
			CJCStringT param_name;
			CheckAbbrev(proxy, (TCHAR)m_lookahead.m_val, param_name);
			NextToken(m_lookahead);
			if ( ! MatchPV2PushParam(combo, proxy, param_name) )	
			{
				SYNTAX_ERROR(_T("Missint parameter value") );
			}
			break;					}

		default:
			return;
		}
	}
}

bool CSyntaxParser::MatchParamVal2(IAtomOperate * & op, bool & constant, bool & file)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == op);
	constant = false;
	file = false;

	bool match = true;

	switch (m_lookahead.m_id)
	{
	// constant
	case ID_STRING:
	case ID_NUMERAL:		{
		MatchConstant(op);
		constant = true;
		break; }

	case ID_FILE_NAME: {	// 文件名作为参数。设置文件名为#filename参数
		op = static_cast<IAtomOperate*>( new CConstantOp<CJCStringT>(m_lookahead.m_str) );
		constant = true;
		file = true;
		NextToken(m_lookahead);
		break; }				

	case ID_VARIABLE:
		NextToken(m_lookahead);	//Match(ID_VARIABLE);
		MatchV3(op);
		break;

	default:
		// error
		match = false;
		break;
	}
	return match;
}

void CSyntaxParser::MatchConstant(IAtomOperate * & val)
{
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

void CSyntaxParser::MatchV3(IAtomOperate * & op)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == op);
	bool constant = false;
	switch (m_lookahead.m_id)
	{
	case ID_WORD:
		MatchVariable(op);
		break;

	//case ID_CURLY_BRACKET_1:
	//case ID_BRACKET_1:
	//	MatchFactor(op, constant);
	//	JCASSERT(constant == false);
	//	break;

	//case ID_FILE_NAME: {	//文件作为缺省输入
	//	MatchFileName(op);
	//	break;			}

	default:
		return;
	}
}

bool CSyntaxParser::MatchTable(CComboStatement * combo, CComboStatement * & op)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == op);

	Match(ID_TABLE, _T("") );

	switch (m_lookahead.m_id)
	{
	case ID_CURLY_BRACKET_1:	{
		Match(ID_CURLY_BRACKET_1, _T(""));
		//stdext::auto_interface<CComboStatement> new_combo;
		CComboStatement::Create(op);
		stdext::auto_interface<IAtomOperate> st;
		MatchSingleSt(op, st);
		// merge new_combo to combo
		Match(ID_CURLY_BRACKET_2, _T("missing }"));
		break; }

	default:		{
		CComboStatement::Create(op);
		stdext::auto_interface<IAtomOperate> var_op;
		MatchV3(var_op);

		op->AddPreproOp(var_op);
		stdext::auto_interface<IAtomOperate>	loop_var( static_cast<IAtomOperate*>(new CLoopVarOp ));
		loop_var->SetSource(0, var_op);
		op->AddLoopOp(loop_var);
		break;		}
	}
	return true;
}

//bool CSyntaxParser::MatchFileName(IAtomOperate * & op)
//{
//	LOG_STACK_TRACE();
//	JCASSERT(NULL == op);
//
//	// 获取文件扩展名作为类型
//	static const JCSIZE EXT_LENGTH = 15;
//	TCHAR ext[EXT_LENGTH+1];
//	memset(ext, 0, sizeof(TCHAR)*(EXT_LENGTH+1));
//	CJCStringT & fn = m_lookahead.m_str;
//	JCSIZE dot = fn.find_last_of(_T("."));
//	JCSIZE ext_size = fn.size() - dot -1;
//	fn.copy(ext, min(ext_size, EXT_LENGTH), dot+1);
//
//	bool br = m_plugin_container->ReadFileOp(ext, m_lookahead.m_str, op);
//	if ( ! br || ! op) SYNTAX_ERROR(_T("unknow file type %s"), ext);
//	NextToken(m_lookahead);
//
//	return true;
//}

bool CSyntaxParser::MatchVariable(IAtomOperate * & op)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == op);

	if (ID_WORD == m_lookahead.m_id)
	{
		stdext::auto_interface<IAtomOperate> sub_op;

		stdext::auto_interface<CVariableOp> vop(
			new CVariableOp(NULL, m_lookahead.m_str) );

		//!! implement m_var_set in tester
		vop->SetSource(0, m_var_set);

		NextToken(m_lookahead);
		MatchV1(vop, sub_op);
		sub_op.detach(op);
	}
	else	SYNTAX_ERROR(_T("expected a variable name."));
	return false;
}

bool CSyntaxParser::MatchV1(CVariableOp * parent_op, IAtomOperate * & sub_op)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == sub_op);

	switch (m_lookahead.m_id)
	{
	case ID_SQUARE_BRACKET_1:
		SYNTAX_ERROR(_T("unsupport"));
		break;

	case ID_DOT:	{
		Match(ID_DOT, _T("missing .") );	
		stdext::auto_interface<CVariableOp> vop (
			new CVariableOp(parent_op, m_lookahead.m_str) );
		MatchV1(vop, sub_op);
		break;	}

	default:		// \E
		sub_op = static_cast<IAtomOperate*>(parent_op);
		sub_op ->AddRef();
		return true;
	}
	return true;
}


void CSyntaxParser::MatchHelp(IAtomOperate * & op)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == op);
	
	//switch (m_lookahead.m_id)
	//{
	//case ID_COLON:

	//case ID_WORD:

	//}
}

void CSyntaxParser::MatchFilterSt(CComboStatement * combo, CFilterSt * & ft)
{
	LOG_STACK_TRACE();
	JCASSERT(combo);
	JCASSERT(NULL == ft);

	NextToken(m_lookahead);

	ft = new CFilterSt;

	// 表格source在添加到loop中时，自动设置
	ft->SetSource(CFilterSt::SRC_TAB, combo->LastChain( NULL ));
	stdext::auto_interface<IAtomOperate> op;
	MatchBoolExpression(combo, op);
	ft->SetSource(CFilterSt::SRC_EXP, op);
	combo->AddLoopOp(static_cast<IAtomOperate*>(ft) );
	combo->LastChain(static_cast<IAtomOperate*>(ft) );
}

bool CSyntaxParser::MatchBoolExpression(CComboStatement * combo, IAtomOperate * & op)
{
	LOG_STACK_TRACE();

	bool match = MatchRelationFactor(combo, op);
	//if ( !match ) SYNTAX_ERROR(_T("expcted boolean expression")); 
	if (!match) return false;

	//// MatchBE1();
	while (1)
	{
		bool exit = false;
		stdext::auto_interface<IAtomOperate> this_op;
		switch (m_lookahead.m_id)
		{
		case ID_BOOLOP_AND:	this_op = static_cast<IAtomOperate*>(new CBoolOpAnd); break;
		case ID_BOOLOP_OR:	this_op = static_cast<IAtomOperate*>(new CBoolOpOr);	break;
		default:			exit = true; break;
		}
		if (exit) break;
		NextToken(m_lookahead);
		this_op->SetSource(0, op);

		stdext::auto_interface<IAtomOperate> exp_r;
		match = MatchRelationFactor(combo, exp_r);
		if ( !match ) SYNTAX_ERROR( _T("synatx error in right side of boolean exp") );
		this_op->SetSource(1, exp_r);
		combo->AddLoopOp(this_op);

		op->Release(), op = NULL;
		this_op.detach<IAtomOperate>(op);
	}
	return true;
}

bool CSyntaxParser::MatchRelationFactor(CComboStatement * combo, IAtomOperate * & op)
{
	LOG_STACK_TRACE();
	bool match = false;

	switch (m_lookahead.m_id)
	{
	case ID_BRACKET_1:
		NextToken(m_lookahead);
		match = MatchBoolExpression(combo, op);
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
		combo->AddLoopOp(this_op);
		op->Release(), op=NULL;
		this_op.detach<IAtomOperate>(op);
		break;	}

	default:
		match = MatchRelationExp(combo, op);
	}
	return match;

	//return MatchRelationExp(combo, exp);
}

bool CSyntaxParser::MatchRelationExp(CComboStatement * combo, IAtomOperate * & op)
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
	combo->AddLoopOp(op);
	return true;
}

// 解析一个表达式可能返回下列结果之一
// 常数：返回IValue
// 变量：返回变量所在的位置
// 计算过程：返回一个IAtomOperate

bool CSyntaxParser::MatchExpression(CComboStatement * combo, IAtomOperate * & op)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == op);

	// 左值
	bool match = MatchTerm(combo, op);
	if (!match) return false;

	while (1)
	{
		// Match('+' / '-')
		if (ID_ATHOP_ADD_SUB != m_lookahead.m_id) break;
		stdext::auto_interface<IAtomOperate>	this_op;

		switch (m_lookahead.m_val)
		{
		case _T('+'):	this_op = static_cast<IAtomOperate*>(new CAthOpAdd); break;
		case _T('-'):	this_op = static_cast<IAtomOperate*>(new CAthOpSub); break;
		default:		break;
		}
		NextToken(m_lookahead);
		this_op->SetSource(0, op);

		// 右值
		stdext::auto_interface<IAtomOperate> exp_r;
		match = MatchTerm(combo, exp_r);
		if ( !match )  SYNTAX_ERROR( _T("syntax error on right side of exp") );

		this_op->SetSource(1, exp_r);
		combo->AddLoopOp(this_op);

		op->Release(); op=NULL;
		this_op.detach<IAtomOperate>(op);
	}
	return true;
}

bool CSyntaxParser::MatchTerm(CComboStatement * combo, IAtomOperate * & op)
{
	return MatchFactor(combo, op);
}

bool CSyntaxParser::MatchFactor(CComboStatement * combo, IAtomOperate * & op)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == op);
	JCASSERT(combo);

	switch (m_lookahead.m_id)
	{
	case ID_CURLY_BRACKET_1:	{
		Match(ID_CURLY_BRACKET_1, _T("Missing {") );
		if (ID_NUMERAL == m_lookahead.m_id)
		{
			SYNTAX_ERROR(_T("unsupport make vector"));
		}
		else
		{
			stdext::auto_interface<CComboStatement>	combo;
			MatchComboSt(combo);
			combo.detach<IAtomOperate>(op);
		}
		Match(ID_CURLY_BRACKET_2, _T("Missing }"));
		break;					}

	case ID_BRACKET_1:
		NextToken(m_lookahead);
		MatchExpression(combo, op);
		Match(ID_BRACKET_2, _T("Missing )") );
		break;

	case ID_VARIABLE:
		NextToken(m_lookahead);	//Match(ID_VARIABLE);
		MatchVariable(op);
		combo->AddPreproOp(op);
		break;

	case ID_DOT:	{
		NextToken(m_lookahead);			//Match(ID_DOT, _T("missing ."));
		// match column value
		if (ID_WORD != m_lookahead.m_id) 	SYNTAX_ERROR(_T("expected a column name"));
		op = static_cast<IAtomOperate*>( new CColumnVal(m_lookahead.m_str) );
		op->SetSource(0, combo->LastChain(NULL) );
		combo->AddLoopOp(op);
		NextToken(m_lookahead);
		break;	}

	case ID_STRING:
	case ID_NUMERAL:
		MatchConstant(op);
		combo->AddPreproOp(op);
		break;

	default:
		return false;
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


void CSyntaxParser::SetBoolOption(IFeature * proxy, const CJCStringT & param_name)
{
	JCASSERT(proxy);
	stdext::auto_interface<jcparam::IValue> val;
	val = static_cast<jcparam::IValue*>(jcparam::CTypedValue<bool>::Create(true));
	proxy->PushParameter(param_name, val);
}

bool CSyntaxParser::CheckAbbrev(IFeature * func, TCHAR abbr, CJCStringT & param_name)
{
	JCASSERT(func);
	bool br = func->CheckAbbrev(abbr, param_name);
	if (!br) SYNTAX_ERROR(_T("Abbrev %c is not defined."), abbr);
	return br;
}

bool CSyntaxParser::MatchPV2PushParam(CComboStatement * combo,
		IFeature * func, const CJCStringT & param_name)
{
	LOG_STACK_TRACE();
	JCASSERT(combo);
	JCASSERT(func);

	bool constant = false, file=false;
	stdext::auto_interface<IAtomOperate> expr_op;

	CPushParamOp * push_op = new CPushParamOp(func, param_name);

	if (ID_TABLE == m_lookahead.m_id)
	{
		LOG_DEBUG(_T("match table"));
		stdext::auto_interface<CComboStatement> new_combo;
		MatchTable(combo, new_combo);
		//stdext::auto_interface<IAtomOperate> last;
		bool br = combo->Merge(new_combo, expr_op);
		//bool br = combo->SetLoopVar(op);
		if (!br) SYNTAX_ERROR(_T("table variable has been already set") );

		//combo->GetLoopVar(expr_op);
		//JCASSERT(expr_op);

		combo->AddLoopOp(static_cast<IAtomOperate*>(push_op)/*, false*/);
	}
	else if ( MatchParamVal2(expr_op, constant, file) )
	{
		LOG_DEBUG(_T("match val2"));
		if (file && _T('#') == param_name.at(0) )
			push_op->SetParamName(_T("#filename"));

		combo->AddPreproOp(expr_op/*, false*/);
		combo->AddPreproOp(static_cast<IAtomOperate*>(push_op)/*, false*/);
	}
	else
	{
		push_op->Release();
		return false;
	}

	push_op->SetSource(0, expr_op);
	push_op->Release();
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

bool CSyntaxParser::CreateVarOp(jcparam::IValue * val, IAtomOperate * &op)
{
	JCASSERT(NULL == op);
	op = static_cast<IAtomOperate*>(new CVariableOp(val));
	return true;
}
