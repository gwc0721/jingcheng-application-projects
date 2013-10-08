#pragma once

#include <jcparam.h>

// 定义一个基本执行单位，包括从何处取参数1和2，执行什么过程(IProxy)，经结果保存在何处。
// 如果执行的过程为NULL，则直接将参数1保存到结果

namespace jcscript
{

	class IAtomOperate : virtual public IJCInterface
	{
	public:
		virtual bool GetResult(jcparam::IValue * & val) = 0;
		virtual bool Invoke(void) = 0;
		virtual void SetSource(UINT src_id, IAtomOperate * op) = 0;
#ifdef _DEBUG
	// 用于检查编译结果
	public:
		virtual void DebugOutput(LPCTSTR indentation, FILE * outfile) = 0;
#endif
	};

	class IHelpMessage : virtual public IJCInterface
	{
	public:
		virtual void HelpMessage(FILE * output) = 0;

	};

	class IFeature : virtual public IAtomOperate/*, virtual public IVariableSet*/
	{
	public:
		virtual bool PushParameter(const CJCStringT & var_name, jcparam::IValue * val) = 0;
		virtual bool CheckAbbrev(TCHAR param_name, CJCStringT & var_name) const = 0;
	};

	class ILoopOperate : virtual public IAtomOperate
	{
	public:
		virtual void GetProgress(JCSIZE &cur_prog, JCSIZE &total_prog) const = 0;
		virtual void Init(void) = 0;
		virtual bool InvokeOnce(void) = 0;
	};

	class IPlugin : public virtual IJCInterface
	{
	public:
		virtual bool Reset(void) = 0; 
		virtual void GetFeature(const CJCStringT & cmd_name, IFeature * & pr) = 0;
		virtual void ShowFunctionList(FILE * output) const = 0;

	public:
		virtual LPCTSTR name() const = 0;
	};

	class IPluginContainer : public virtual IJCInterface
	{
	public:
		virtual bool GetPlugin(const CJCStringT & name, IPlugin * & plugin) = 0;
		virtual void GetVarOp(IAtomOperate * & op) = 0;
		virtual bool ReadFileOp(LPCTSTR type, const CJCStringT & filename, IAtomOperate *& op) = 0;
	};
};



