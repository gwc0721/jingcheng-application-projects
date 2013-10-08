#include "stdafx.h"

#include "plugin_simulator.h"
#include "application.h"

LOCAL_LOGGER_ENABLE(_T("simulator_pi"), LOGGER_LEVEL_WARNING);

CJCStringT CPluginBase2<CPluginSimulator>::m_name(_T("simulator"));

FEATURE_LIST CPluginBase2<CPluginSimulator>::m_feature_list(
	FEATURE_LIST::RULE()
	(new CTypedFtInfo< CPluginSimulator::Inject::_BASE >( _T("cache mode simulator") ) ) 
	(new CTypedFtInfo< CPluginSimulator::RandomPatten::_BASE >( _T("make a random write patten") ) )
	(new CTypedFtInfo< CPluginSimulator::Connect::_BASE>(_T("connect to simulator device") ) )
);




CPluginSimulator::CPluginSimulator(void)
{
};

CPluginSimulator::~CPluginSimulator(void)
{
}

///////////////////////////////////////////////////////////////////////////////
// -- cache mode simulator
LPCTSTR CPluginSimulator::Inject::_BASE::m_feature_name = _T("inject");

CParamDefTab CPluginSimulator::Inject::_BASE::m_param_def_tab(
	CParamDefTab::RULE()
	(new CTypedParamDef<CJCStringT>(_T("#filename"), 0, offsetof(CPluginSimulator::Inject, m_file_name) ) )
	);

CPluginSimulator::Inject::Inject() 
	: m_smi_dev(NULL)
	, m_storage(NULL)
	, m_data_buf(NULL)
	, m_max_secs(0)
	, m_trace(NULL)
{
}

CPluginSimulator::Inject::~Inject(void)
{
	if (m_smi_dev) m_smi_dev->Release();
	if (m_storage) m_storage->Release();
	if (m_trace)	m_trace->Release();
	delete [] m_data_buf;
}

bool CPluginSimulator::Inject::GetResult(jcparam::IValue * & val)
{
	val = m_trace;
	if (val) val->AddRef();

	return true;
}

bool CPluginSimulator::Inject::Invoke(void)
{
	// Initialize
	if (!m_smi_dev)
	{
		CSvtApplication::GetApp()->GetDevice(m_smi_dev);
		stdext::auto_cif<IStorageDevice>	storage;
		m_smi_dev->QueryInterface(IF_NAME_STORAGE_DEVICE, storage);
		storage.detach(m_storage);
	}

	if (m_trace)	m_trace->Release(), m_trace = NULL;

	if (!m_src_op) return false;
	stdext::auto_interface<jcparam::IValue> val;
	m_src_op->GetResult(val);
	if ( !val ) return false;

	CAtaTrace * trace = val.d_cast<CAtaTrace *>();
	JCASSERT(trace);
	BYTE cmd = trace->m_cmd_code;
	JCSIZE lba = trace->m_lba;
	JCSIZE secs = trace->m_sectors;

	//BYTE cmd = 0;
	//JCSIZE lba = 0;
	//JCSIZE secs = 0;

	//GetSubVal(val, _T("cmd_code"), cmd);
	//GetSubVal(val, _T("lba"), lba);
	//GetSubVal(val, _T("sectors"), secs);

	if (secs > m_max_secs)
	{
		m_max_secs = secs;
		delete [] m_data_buf;
		m_data_buf = new BYTE[m_max_secs * SECTOR_SIZE];
	}
	
	m_trace = new CAtaTraceRow;
	m_trace->m_cmd_code = cmd;
	m_trace->m_lba = lba;
	m_trace->m_sectors = secs;
	
	if (cmd == 0xCA)
	{
		m_storage->SectorWrite(m_data_buf, lba, secs);
		m_trace->m_busy_time = m_storage->GetLastInvokeTime() / 1000.0;	// us -> ms
	}
	//val->Release();
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// -- make random patten
LPCTSTR CPluginSimulator::RandomPatten::_BASE::m_feature_name = _T("random");

CParamDefTab CPluginSimulator::RandomPatten::_BASE::m_param_def_tab(
	CParamDefTab::RULE()
	(new CTypedParamDef<FILESIZE>(_T("range"),	'r', offsetof(CPluginSimulator::RandomPatten, m_range), _T("") ) )
	(new CTypedParamDef<UINT>(_T("seed"),		's', offsetof(CPluginSimulator::RandomPatten, m_seed), _T("") ) )
	(new CTypedParamDef<JCSIZE>(_T("count"),	'n', offsetof(CPluginSimulator::RandomPatten, m_count), _T("") ) )
	);


CPluginSimulator::RandomPatten::RandomPatten()
	: m_seed(0)
	, m_count(100)
{
}

CPluginSimulator::RandomPatten::~RandomPatten(void)
{
}

void CPluginSimulator::RandomPatten::GetProgress(JCSIZE &cur_prog, JCSIZE &total_prog) const
{
}

void CPluginSimulator::RandomPatten::Init(void)
{
	srand(m_seed);
	m_cur_count = 0;
}

bool CPluginSimulator::RandomPatten::InvokeOnce(void)
{
	if (m_output) m_output->Release(), m_output = NULL;

	CAtaTraceRow * trace = new CAtaTraceRow;
	//memset( static_cast<CAtaTrace*>(trace), 0, sizeof(CAtaTrace) );

	trace->m_cmd_code = 0xCA;		// write dma
	trace->m_lba = ((UINT)(rand()) << 15) | (UINT)(rand());
	trace->m_lba %= m_range;
	trace->m_sectors = 1;

	m_output = static_cast<jcparam::IValue*>(trace);
	
	m_cur_count ++;
	return (m_cur_count <= m_count);
}

///////////////////////////////////////////////////////////////////////////////
// -- connect to simulation device
LPCTSTR CPluginSimulator::Connect::_BASE::m_feature_name = _T("connect");

CParamDefTab CPluginSimulator::Connect::_BASE::m_param_def_tab(
	CParamDefTab::RULE()
	(new CTypedParamDef<CJCStringT>(_T("type"), _T('t'), offsetof(CPluginSimulator::Connect, m_type) ) )
	);

CPluginSimulator::Connect::Connect() 
{
}

CPluginSimulator::Connect::~Connect(void)
{
}

bool CPluginSimulator::Connect::GetResult(jcparam::IValue * & val)
{
	return false;
}

bool CPluginSimulator::Connect::Invoke(void)
{
	stdext::auto_interface<CSimCacheMode> dev(new CSimCacheMode);
	CSvtApplication::GetApp()->SetDevice(_T("cache_mode"), dev);
	return true;
}