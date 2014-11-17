#include "stdafx.h"

#include "feature_2246_readcount.h"

#include "application.h"

LOCAL_LOGGER_ENABLE(_T("read_count"), LOGGER_LEVEL_WARNING);

///////////////////////////////////////////////////////////////////////////////
// block read count
#define __COL_CLASS_NAME CBlockReadCount
BEGIN_COLUMN_TABLE()
	COLINFO_DEC( JCSIZE, 	0,		m_id,			_T("block_id") )
	COLINFO_DEC( JCSIZE, 	1,		m_read_count_1,	_T("count_1") )
	COLINFO_DEC( JCSIZE, 	2,		m_read_count_2,	_T("count_2") )
	COLINFO_DEC( JCSIZE,	3,		m_total_read_count,		_T("count_all") )

END_COLUMN_TABLE()
#undef __COL_CLASS_NAME



///////////////////////////////////////////////////////////////////////////////
// read count

LPCTSTR CFeature2246ReadCount::_BASE::m_feature_name = _T("readcount");

CParamDefTab CFeature2246ReadCount::_BASE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<CJCStringT>(_T("#filename"), 0, offsetof(CFeature2246ReadCount, m_file_name) ) )
	);

const TCHAR CFeature2246ReadCount::m_desc[] = _T("retriev read count");

CFeature2246ReadCount::CFeature2246ReadCount(void)
	: m_init(false)
	, m_smi_dev(NULL)
	, m_read_count(NULL)
	, m_block_count(0)
{
}

CFeature2246ReadCount::~CFeature2246ReadCount(void)
{
	if (m_smi_dev)	m_smi_dev->Release();
	delete [] m_read_count;
}

bool CFeature2246ReadCount::Invoke(jcparam::IValue * , jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();
	if (!m_init)	Init();
	
	CBlockReadCountRow * row = new CBlockReadCountRow;
	JCSIZE index = m_block_count * 16;
	row->m_id = m_block_count;
	BYTE * read_cnt = m_read_count + index;
	row->m_read_count_1 = MAKELONG( MAKEWORD(read_cnt[7], read_cnt[6]), MAKEWORD(read_cnt[5], read_cnt[4]) );
	read_cnt += 8;
	row->m_read_count_2 = MAKELONG( MAKEWORD(read_cnt[7], read_cnt[6]), MAKEWORD(read_cnt[5], read_cnt[4]) );
	row->m_total_read_count = row->m_read_count_1 + row->m_read_count_2;
	outport->PushResult(row);
	row->Release();

	m_block_count ++;

	return (m_block_count < BLOCK_COUNT);
}

bool CFeature2246ReadCount::Init(void)
{
	CSvtApplication::GetApp()->GetDevice(m_smi_dev);
	JCASSERT(m_smi_dev);

	{
		// Retrieve read count

		CSmiCommand	cmd;
		cmd.id(0xF0DC);
		cmd.size() = READ_COUNT_LENGTH;
		delete [] m_read_count;
		m_read_count = new BYTE[READ_COUNT_LENGTH * SECTOR_SIZE];
		m_smi_dev->VendorCommand(cmd, read, m_read_count, READ_COUNT_LENGTH);
		m_block_count = 0;

		CJCStringT fn = m_file_name + _T("_1.bin");
		FILE * file = NULL;
		stdext::jc_fopen(&file, fn.c_str(), _T("w+"));
		if (!file)	THROW_ERROR(ERR_USER, _T("failure on creating file %s"), fn.c_str());
		fwrite(m_read_count, SECTOR_SIZE, READ_COUNT_LENGTH, file);
		fclose(file);
	}

	{
		// Retrieve idle time
		CSmiCommand	cmd;
		cmd.id(0xF0DD);
		cmd.size() = 1;

		stdext::auto_array<BYTE> idle_buf(SECTOR_SIZE);

		m_smi_dev->VendorCommand(cmd, read, idle_buf, 1);

		CJCStringT fn = m_file_name + _T("_2.bin");
		FILE * file = NULL;
		stdext::jc_fopen(&file, fn.c_str(), _T("w+"));
		if (!file)	THROW_ERROR(ERR_USER, _T("failure on creating file %s"), fn.c_str() );
		fwrite(idle_buf, SECTOR_SIZE, 1, file);
		fclose(file);
	}

	m_init = true;
	return true;
}


