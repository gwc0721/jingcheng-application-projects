#pragma once

#include <map>
#include <stdext.h>
#include <script_engine.h>

class jcscript::IPlugin;
class CDynamicModule;

typedef bool (*PLUGIN_CREATOR)(jcparam::IValue *, jcscript::IPlugin * &);

class CPluginDefault;

///////////////////////////////////////////////////////////////////////////////
//-- CVariableContainer
class CVariableContainer 
	: virtual public jcparam::IValue
	, public CJCInterfaceBase
{
public:
	CVariableContainer(void);
	virtual ~CVariableContainer(void);
	
public:
	virtual void GetValueText(CJCStringT & str) const {};
	virtual void SetValueText(LPCTSTR str)  {};
	virtual void GetSubValue(LPCTSTR name, IValue * & val);
	// 如果name不存在，则插入，否则修改name的值
	virtual void SetSubValue(LPCTSTR name, IValue * val);

protected:
	jcparam::CParamSet* 	m_global_var;

};


///////////////////////////////////////////////////////////////////////////////
//-- CPluginManager
class CPluginManager : virtual public jcscript::IPluginContainer,  public CJCInterfaceBase
{
protected:

public:
	class CPluginInfo
	{
	public:
		enum PROPERTY
		{
			PIP_SINGLETONE = 0x00000001,
			PIP_DYNAMIC =	 0x00000002,
		};

		CPluginInfo(const CJCStringT& name, CDynamicModule * module, DWORD _property, PLUGIN_CREATOR creator)
			: m_name(name), m_module(module), m_property(_property), m_creator(creator)
			, m_object(NULL)
		{};
		~CPluginInfo(void)
		{
			if (m_object) m_object->Release();
		}

		CJCStringT		m_name;
		CDynamicModule	*m_module;
		DWORD			m_property;
		PLUGIN_CREATOR	m_creator;
		jcscript::IPlugin	*		m_object;
	};

public:
	CPluginManager(void);
	virtual ~CPluginManager(void);

public:
	bool RegistPlugin(const CJCStringT & name, CDynamicModule * module, DWORD _property, PLUGIN_CREATOR creator);
	bool RegistPlugin(jcscript::IPlugin * plugin);

	bool ReleasePlugin(jcscript::IPlugin* plugin);
	bool RegistDefaultPlugin(jcscript::IPlugin * plugin);

	void GetVariable(jcparam::IValue * & vars)
	{
		JCASSERT(NULL == vars);
		vars = m_vars;
		if (vars) vars->AddRef();
	}

	virtual bool GetPlugin(const CJCStringT & name, jcscript::IPlugin * & plugin);
	virtual void GetVarOp(jcscript::IAtomOperate * & op);
	virtual bool ReadFileOp(LPCTSTR type, const CJCStringT & filename, jcscript::IAtomOperate *& op);

protected:

	typedef std::pair<CJCStringT, CPluginInfo *> PLUGIN_INFO_PAIR;
	typedef std::map<CJCStringT, CPluginInfo *> PLUGIN_MAP;
	typedef PLUGIN_MAP::iterator				PLUGIN_ITERATOR;

	PLUGIN_MAP	m_plugin_map;

	jcscript::IPlugin * m_default_plugin;
	CVariableContainer	* m_vars;

	//CPluginDefault	* m_default_plugin;
};
