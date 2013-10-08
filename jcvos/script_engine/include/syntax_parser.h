#pragma once

#include <vector>
#include <jcparam.h>

#include "iplugin.h"

namespace jcscript
{
	///////////////////////////////////////////////////////////////////////////////
	//--CExitException
	//  用于执行Exit并退出命令循环
	class CExitException
	{
	};	

	// 声明IAtomOperate的类(按字母顺序)	
	class CComboStatement;
	class CVariableOp;	

	class CSyntaxParser;
	class CFilterSt;

	// 词法分析
	enum TOKEN_ID
	{
		ID_WHITE_SPACE_ = 1,	// 空白
		ID_COLON,			// 冒号，模块名与命令的分界
		ID_DOT,
		ID_CURLY_BRACKET_1,		// 花括弧，跟在$后面用于表示执行字句
		ID_CURLY_BRACKET_2,
		ID_BRACKET_1,		// 括弧，跟在$后面表示创建变量
		ID_BRACKET_2,	
		ID_SQUARE_BRACKET_1,
		ID_SQUARE_BRACKET_2,

		// 关系运算符
		ID_REL_OP,			// 关系运算符
		ID_RELOP_LT,		// <
		ID_RELOP_GT,		// >
		ID_RELOP_LE,		// <=
		ID_RELOP_GE,		// >=
		ID_RELOP_EE,		// ==
		// 算术运算符
		ID_ATHOP_ADD_SUB,
		ID_ATHOP_MUL_DIV,
		// 逻辑运算符
		ID_BOOLOP_AND,
		ID_BOOLOP_OR,
		ID_BOOLOP_NOT,

		ID_COMMENT,			// 注释
		ID_CONNECT,			// 命令连接
		ID_VARIABLE,		// 变量
		ID_TABLE,			// @，用于执行表格中的每一项
		ID_PARAM_NAME,		// 参数名称
		ID_PARAM_TABLE,		// --@ 以表中的列名作为参数名称
		ID_EQUAL,			// 等号
		ID_QUOTATION1,		// 双引号
		ID_QUOTATION2,		// 单引号
		ID_STRING,			// 除去引号后的内容
		ID_ABBREV,			// 缩写参数
		ID_ABBREV_BOOL,		// 无变量的缩写参数
		ID_HEX,
		ID_WORD,
		ID_DECIMAL,
		ID_NUMERAL,
		ID_FILE_NAME,

		ID_VAR_RETURN,		// returned by OnVariable
		ID_EOF,
		ID_NEW_LINE,
		ID_SELECT,
		ID_ASSIGN,

		// 关键词
		ID_KEY_ELSE,
		ID_KEY_END,
		ID_KEY_EXIT,		
		ID_KEY_FILTER,		
		ID_KEY_HELP,
		ID_KEY_IF,
		ID_KEY_THEN,

		//编译控制
		ID_NEXT,
		ID_SYMBO_ERROR,
	};

	//-- token
	class CTokenProperty
	{
	public:
		TOKEN_ID	m_id;
		CJCStringT	m_str;
		INT64		m_val;
		CSyntaxParser	* m_parser;
	};

	class LSyntaxErrorHandler
	{
	public:
		virtual void OnError(JCSIZE line, JCSIZE column, LPCTSTR msg) = 0;
	};
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//-- CSyntaxParser
class jcscript::CSyntaxParser
{
public:
	CSyntaxParser(IPluginContainer * plugin_container, LSyntaxErrorHandler * err_handler);
	~CSyntaxParser(void);

public:
	static bool CreateVarOp(jcparam::IValue * val, IAtomOperate * &op);
	void SetVariableManager(IAtomOperate * val_op);
	void Parse(LPCTSTR &str, LPCTSTR last);
	void Source(FILE * file);
	bool GetError(void)	{return m_syntax_error;};

public:
	bool MatchAssign(CComboStatement * combo, IAtomOperate * & op);
	bool MatchBoolExpression(CComboStatement * combo, IAtomOperate * & op);
	//void MatchBE1(CComboStatement * combo, IAtomOperate * & op);

	bool MatchCmdSet(CComboStatement * combo, IFeature * proxy, int index);
	bool MatchComboSt(CComboStatement * & combo);
	void MatchConstant(IAtomOperate * & val);
	// 解析一个表达式，结果在op中返回。如果op立刻可以被求结果
	//（常数，常熟组成的表达式等）则返回true，通过调用op的Invloke
	// 成员函数得到计算结果。
	bool MatchExpression(CComboStatement * combo, IAtomOperate * & op);
	//bool MatchExp1();

	bool MatchFactor(CComboStatement * combo, IAtomOperate * & op);
	void MatchFeature(CComboStatement * combo, IPlugin * plugin, IFeature * & f);
	//bool MatchFileName(IAtomOperate * & op);
	void MatchFilterSt(CComboStatement * combo, CFilterSt * & ft);
	void MatchHelp(IAtomOperate * & op);
	void MatchParamSet(CComboStatement * comb, IFeature * proxy);
	bool MatchParamVal2(IAtomOperate * & op, bool & constant, bool & file);
	// 关系运算符
	bool MatchRelationExp(CComboStatement * comb, IAtomOperate * & op);
	bool MatchRelationFactor(CComboStatement * comb, IAtomOperate * & exp);


	void MatchS1(CComboStatement * combo);
	bool MatchScript(IAtomOperate * & program);
	//void OnSelect(CComboStatement * combo);
	bool MatchSingleSt(CComboStatement * combo, IAtomOperate * & proxy);
	// 处理@符号
	//	[IN] combo：当前Combo
	//	[OUT] op :由@后语句组成的新combo
	bool MatchTable(CComboStatement * combo, CComboStatement * & op);
	bool MatchTerm(CComboStatement * combo, IAtomOperate * & op);
	bool MatchVariable(IAtomOperate * & op);
	bool MatchV1(CVariableOp * parent_op, IAtomOperate * & op);
	void MatchV3(IAtomOperate * & op);

public:
	void NewLine(void);

protected:
	void NextToken(CTokenProperty & prop);
	void Token(CTokenProperty & prop);
	void ReadSource(void);
	bool Match(TOKEN_ID id, LPCTSTR err_msg);
	void PushOperate(IAtomOperate * op);
	bool MatchPV2PushParam(CComboStatement * combo, IFeature * func, 
		const CJCStringT & param_name);
	void SetBoolOption(IFeature * proxy, const CJCStringT & param_name);
	bool CheckAbbrev(IFeature * proxy, TCHAR abbr, CJCStringT & param_name);

	void OnError(LPCTSTR msg, ...);



protected:
	// 记录需要编译的字符串的头和尾
	LPCTSTR m_first, m_last;
	JCSIZE	m_line_num;		// 行计数
	LPCTSTR	m_line_begin;	// 行的开始位置

	CTokenProperty	m_lookahead;

	IPluginContainer	* m_plugin_container;

	int m_default_param_index;		// 用于无参数名的参数序号

	// 变量管理对象，目前仅支持全局变量管理。下一步可支持局部变量。
	// m_var_set不需要调用Invoke()，仅调用GetResult()就能够得到全局变量的根。
	IAtomOperate * m_var_set;

	// 源文件处理
	FILE * m_file;
	TCHAR * m_src_buf;		// 用于存储源文件
	JCSIZE	m_buf_remain;

	// 标志：直解析一行
	bool m_one_line;

	LSyntaxErrorHandler * m_error_handler;

	// 是否曾经发生过编译错误
	bool m_syntax_error;
};