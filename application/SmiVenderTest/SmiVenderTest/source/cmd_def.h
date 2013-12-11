#pragma once

#include <map>
#include <jcparam.h>

#include <script_engine.h>

class CCmdDefBase;
///////////////////////////////////////////////////////////////////////////////
//-- CPorxyBase, template CProxy

class CProxyBase 
	: virtual public jcscript::IFeature
	, virtual public jcscript::IHelpMessage 
	, public CJCInterfaceBase
{
protected:
	CProxyBase(const CCmdDefBase * def);
	~CProxyBase(void);

// IHelpMessage
public:
	virtual void HelpMessage(FILE * output);

// IAtomOperate
public:
	virtual bool GetResult(jcparam::IValue * & val);
	virtual void SetSource(UINT src_id, jcscript::IAtomOperate * op);
#ifdef _DEBUG
public:
	virtual void DebugOutput(LPCTSTR indentation, FILE * outfile);
#endif

// IFeature
public:
	virtual bool PushParameter(const CJCStringT & var_name, jcparam::IValue * val);
	virtual bool CheckAbbrev(TCHAR param_name, CJCStringT & var_name) const;

protected:
	const CCmdDefBase * m_def;
	jcparam::CArguSet	m_argus;
	// 表格的循环处理
	jcparam::IValue *	m_inout;		// for Input and Output

	jcscript::IAtomOperate	* m_src_op;
};

template <class PLUGIN>
class CTypedProxy : public CProxyBase, public Factory2<const CCmdDefBase *, PLUGIN, CTypedProxy<PLUGIN> >
{
public:
	static void Create(const CCmdDefBase * def, PLUGIN * plugin, jcscript::IFeature * & proxy)
	{
		JCASSERT(NULL == proxy);
		proxy = static_cast<jcscript::IFeature*>( new CTypedProxy<PLUGIN>(def, plugin) );
	}

	virtual bool Invoke()
	{
		const CCmdDef<PLUGIN> * cmd_def = dynamic_cast<const CCmdDef<PLUGIN> * >(m_def);
		JCASSERT(cmd_def);
		CCmdDef<PLUGIN>::CMD_PROCESS proc = cmd_def->GetFunc();
		JCASSERT(proc);

		if (m_src_op)	m_src_op->GetResult(m_inout);
		jcparam::IValue * output = NULL;
		bool br = (m_plugin->*proc)(m_argus, m_inout, output);
		if (m_inout) m_inout->Release();
		m_inout = output;
		return br;
	}

protected:
	CTypedProxy(const CCmdDefBase * def, PLUGIN * plugin) : CProxyBase(def), m_plugin(plugin)
	{
		if (m_plugin) m_plugin->AddRef();
	}
	~CTypedProxy(void) { if (m_plugin) m_plugin->Release(); }

protected:
	PLUGIN * m_plugin;
};




///////////////////////////////////////////////////////////////////////////////
//-- 
#define BEGIN_COMMAND_DEFINATION	const CCmdManager __CLASS_NAME__::m_cmd_man(CCmdManager::RULE()

#define COMMAND_DEFINE(cmd, function, cmd_prop, param_prop, description, ...)		\
	(new CCmdDef<__CLASS_NAME__>(cmd,	&__CLASS_NAME__::function, cmd_prop, description, param_prop, \
	PARAM_RULE() __VA_ARGS__ ))

#define BEGIN_CMD_DEF(cmd, function, cmd_prop, param_prop, description)		\
	(new CCmdDef<__CLASS_NAME__>(cmd,	&__CLASS_NAME__::function, cmd_prop, description, param_prop, \
	PARAM_RULE()

#define END_CMD_DEF ))

#define PARDEF(name, abbrev, type, description)		\
	(name,	#@abbrev, jcparam::type, description)

#define END_COMMAND_DEFINATION );

#define PARAM_PROP(prop)	jcparam::CParameterDefinition::prop

typedef jcparam::CParameterDefinition::RULE	PARAM_RULE;

class CCmdDefBase
{
public:
	CCmdDefBase(LPCTSTR cmd, DWORD cmd_prop, LPCTSTR desc, DWORD parser_prop, const PARAM_RULE & parser_rule)
		: m_parser(parser_rule, parser_prop), m_cmd_name(cmd), m_prop(cmd_prop), m_description(desc)
	{
	}
	CCmdDefBase( const CCmdDefBase & def)
		: m_parser(def.m_parser), m_cmd_name(def.m_cmd_name), m_description(def.m_description), m_prop(def.m_prop)
	{
	}

public:
	//jcparam::CCmdLineParser * GetParser();
	const jcparam::CParameterDefinition & GetParamDefine(void) const {return m_parser;}

	const CJCStringT & GetCommandName() const { return m_cmd_name; }
	virtual bool CreateProxy(jcscript::IPlugin * plugin, jcscript::IFeature * & cmd_obj) const = 0;
	LPCTSTR Description() const {return m_description;};
	LPCTSTR name(void) const {return m_cmd_name.c_str(); }

protected:
	jcparam::CParameterDefinition		m_parser;
	CJCStringT					m_cmd_name;
	LPCTSTR						m_description;
	DWORD						m_prop;
};


template <class PLUGIN>
class CCmdDef : public CCmdDefBase
{
public:
	typedef bool (PLUGIN::*CMD_PROCESS)(jcparam::CArguSet &, jcparam::IValue *, jcparam::IValue * &);

public:
	CCmdDef(LPCTSTR cmd, CMD_PROCESS proc, DWORD cmd_prop, 
		LPCTSTR desc = NULL, 
		DWORD parser_prop = 0, 
		const PARAM_RULE & parser_rule = PARAM_RULE())
		: CCmdDefBase(cmd, cmd_prop, desc, parser_prop, parser_rule), m_cmd_proc(proc)
	{}

	CCmdDef(const CCmdDef & def)
		: CCmdDefBase(def), m_cmd_proc(def.m_cmd_proc)
	{}

	virtual bool CreateProxy(jcscript::IPlugin * plugin, jcscript::IFeature * & proxy) const
	{
		JCASSERT(NULL == proxy);
		PLUGIN *p = dynamic_cast<PLUGIN*>(plugin);
		JCASSERT(p);
		CTypedProxy<PLUGIN>::Create(static_cast<const CCmdDefBase*>(this), p, proxy);
		return true;
	}

	CMD_PROCESS GetFunc(void) const {return m_cmd_proc;};

protected:
	CMD_PROCESS		m_cmd_proc;
};

typedef jcparam::CStringTable<CCmdDefBase> CCmdManager;

