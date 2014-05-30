#pragma once

#include <jcparam.h>
#include <FerriSimulator.h>
#include <script_engine.h>
//#include "feature_base.h"
#include "ata_trace.h"

class CPluginSimulator 
	: virtual public jcscript::IPlugin
	, public CPluginBase2<CPluginSimulator>
{
public:
	CPluginSimulator(void);
	virtual ~CPluginSimulator(void);

public:
	virtual bool Reset(void) {return true;} 
	//virtual LPCTSTR name() const {return _T("simulator");}

	// Features
public:
	class Inject;
	class RandomPatten;
	class Connect;
};

///////////////////////////////////////////////////////////////////////////////
// -- inject
class CPluginSimulator::Inject
	: public CFeatureBase<CPluginSimulator::Inject, CPluginSimulator>
	, public CJCInterfaceBase
{
public:
	typedef CFeatureBase<CPluginSimulator::Inject, CPluginSimulator>	_BASE;
public:
	Inject(void);
	virtual ~Inject(void);

public:
	virtual bool GetResult(jcparam::IValue * & val);
	virtual bool Invoke(void);

public:
	CJCStringT m_file_name;
	ISmiDevice * m_smi_dev;
	IStorageDevice * m_storage;

	BYTE *	m_data_buf;
	JCSIZE m_max_secs;

	CAtaTraceRow * m_trace;
};

class CPluginSimulator::RandomPatten
	: public virtual jcscript::IFeature
	//, virtual public jcscript::ILoopOperate
	//, public CLoopFeatureBase<CAtaTrace>
	, public CFeatureBase<CPluginSimulator::RandomPatten, CPluginSimulator>
	, public CJCInterfaceBase
{
public:
	typedef CFeatureBase<CPluginSimulator::RandomPatten, CPluginSimulator> _BASE;
public:
	RandomPatten(void);
	virtual ~RandomPatten(void);

public:
	virtual void GetProgress(JCSIZE &cur_prog, JCSIZE &total_prog) const;
	virtual void Init(void);
	virtual bool InvokeOnce(void);

public:
	FILESIZE	m_range;
	UINT		m_seed;
	JCSIZE		m_count, m_cur_count;
};

class CPluginSimulator::Connect
	: public CFeatureBase<CPluginSimulator::Connect, CPluginSimulator>
	, public CJCInterfaceBase
{
public:
	typedef CFeatureBase<CPluginSimulator::Connect, CPluginSimulator> _BASE;
public:
	Connect(void);
	virtual ~Connect(void);

// ILoopOperate
public:
	virtual bool GetResult(jcparam::IValue * & val);
	virtual bool Invoke(void);

public:
	CJCStringT m_type;
};