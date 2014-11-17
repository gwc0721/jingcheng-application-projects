#include "stdafx.h"

#include "ata_trace.h"
LOCAL_LOGGER_ENABLE(_T("parser_vendor_cmd"), LOGGER_LEVEL_WARNING);

///////////////////////////////////////////////////////////////////////////////
// vender command
CVenderCmd::CVenderCmd(void)
: m_data(NULL), m_cmd_data(NULL)
{
	memset(this, 0, sizeof(CVenderCmd));
}

CVenderCmd::~CVenderCmd(void)
{
	if (m_data) m_data->Release();
	if (m_cmd_data) m_cmd_data->Release();
}

CVenderCmd::CVenderCmd(const CVenderCmd & cmd)
: m_data(NULL), m_cmd_data(NULL)
{
	m_id = cmd.m_id;
	m_sectors = cmd.m_sectors;
	m_read_write = cmd.m_read_write;
	m_cmd_data = cmd.m_cmd_data;
	if (m_cmd_data)
	{
		m_cmd_data->AddRef();
		SetCmdData(m_cmd_data->Lock(), CMD_BUF_LENGTH);
		m_cmd_data->Unlock();
	}
}

void CVenderCmd::SetCmdData(BYTE * data, JCSIZE length)
{
	BYTE _buf[CMD_BUF_LENGTH];
	memcpy_s(_buf, CMD_BUF_LENGTH, data, length);
	m_cmd_id = MAKEWORD(_buf[1], _buf[0]);
	m_block = MAKEWORD(_buf[3], _buf[2]);
	m_page = MAKEWORD(_buf[5], _buf[4]);
	m_chunk = _buf[6];
}

#define __COL_CLASS_NAME CVenderCmd
BEGIN_COLUMN_TABLE()
	COLINFO_DEC( JCSIZE, 	0,	m_id,		_T("store") )
	COLINFO_HEX( WORD, 	1,		m_cmd_id,	_T("cmd_id") )
	COLINFO_HEX( WORD, 	2,		m_block,	_T("f_block") )
	COLINFO_HEX( WORD, 	3,		m_page,		_T("f_page") )
	COLINFO_HEX( BYTE,	4,		m_chunk,	_T("f_chunk") )
	COLINFO_DEC( int,	5,		m_read_write,	_T("r_w") )
	COLINFO_HEX( UINT,	6,		m_sectors,	_T("sectors") )
	( new CTracePayload(7, offsetof(CVenderCmd, m_data), _T("data") ) )
	( new CTracePayload(8, offsetof(CVenderCmd, m_cmd_data), _T("cmd") ) )

END_COLUMN_TABLE()
#undef __COL_CLASS_NAME

///////////////////////////////////////////////////////////////////////////////
// vender command
LPCTSTR CFeatureBase<CPluginTrace::VenderCmd, CPluginTrace>::m_feature_name = _T("vendercmd");

CParamDefTab CFeatureBase<CPluginTrace::VenderCmd, CPluginTrace>::m_param_def_tab(
	CParamDefTab::RULE()
	(new CTypedParamDef<CJCStringT>(_T("#filename"), 0, offsetof(CPluginTrace::VenderCmd, m_file_name) ) )
	);

//const UINT CPluginTrace::VenderCmd::_BASE::m_property = 0;

CPluginTrace::VenderCmd::VenderCmd(void)
	: m_cmd_status(0)
	, m_init(false)
	, m_cmd(NULL)
	, m_store(0)
{
}

CPluginTrace::VenderCmd::~VenderCmd(void)
{
	if (m_cmd) m_cmd->Release();
}

bool CPluginTrace::VenderCmd::Invoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	CAtaTraceRow * trace = dynamic_cast<CAtaTraceRow*>(row);
	if (!trace)	THROW_ERROR(ERR_USER, _T("expected CAtaTraceRow input for vender command"));

	bool is_read_cmd = (trace->m_cmd_code == CMD_READ_DMA) || (trace->m_cmd_code == CMD_READ_SECTOR);
	bool is_write_cmd = (trace->m_cmd_code == CMD_WRITE_DMA) || (trace->m_cmd_code == CMD_WRITE_SECTOR);


	switch (m_cmd_status)
	{
	case 0:	// waiting 0x00AA
		if ( is_read_cmd && (trace->m_sectors == 1) && (trace->m_lba == 0x00AA) ) 
		{
			m_cmd_status ++;
			m_store = trace->m_id;
		}
		else	m_cmd_status = 0;
		break;
	case 1: // waiting 0xAA00
		if ( is_read_cmd && (trace->m_sectors == 1) && (trace->m_lba == 0xAA00) ) m_cmd_status ++;
		else	m_cmd_status = 0;
		break;
	case 2: // waiting 0x0055
		if ( is_read_cmd && (trace->m_sectors == 1) && (trace->m_lba == 0x0055) ) m_cmd_status ++;
		else	m_cmd_status = 0;
		break;
	case 3: // waiting 0x5500
		if ( is_read_cmd && (trace->m_sectors == 1) && (trace->m_lba == 0x5500) ) m_cmd_status ++;
		else	m_cmd_status = 0;
		break;
	case 4: // waiting 0x55AA
		if ( is_read_cmd && (trace->m_sectors == 1) && (trace->m_lba == 0x55AA) ) m_cmd_status ++;
		else	m_cmd_status = 0;
		break;
	case 5:		{// send vender command
		if ( (!is_read_cmd) && (trace->m_sectors == 1) && (trace->m_lba == 0x55AA) )
		{
			// create vender command
			JCASSERT(NULL == m_cmd);
			m_cmd = new CVenderCmdRow;
			m_cmd->m_id = m_store;
			// fill data
			CBinaryBuffer * data = trace->m_data;
			if (data)	
			{
				m_cmd->m_cmd_data = data;
				m_cmd->m_cmd_data->AddRef();
				m_cmd->SetCmdData(data->Lock(), data->GetSize() );
			}
			m_cmd_status ++;
		}
		else m_cmd_status = 0;
		break;	}

	case 6: // read/write data
		// copy data
		if ( (trace->m_lba == 0x55AA) )
		{
			JCASSERT(m_cmd);
			m_cmd->m_data = trace->m_data;
			if (m_cmd->m_data)	m_cmd->m_data->AddRef();
			m_cmd->m_sectors = trace->m_sectors;
			m_cmd->m_read_write = is_read_cmd ? read : write;
		}
		m_cmd_status = 0;

		outport->PushResult(m_cmd);
		m_cmd->Release(), m_cmd = NULL;
		break;

	default:
		JCASSERT(0);
		break;

	}
	return false;
}

//bool CPluginTrace::VenderCmd::IsRunning(void)
//{
//	return true;
//}

bool CPluginTrace::VenderCmd::Init(void)
{
	m_cmd_status = 0;
	m_init = true;
	return true;
}
