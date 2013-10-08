#include "stdafx.h"
#include <stdext.h>

#include "plugin_manager.h"
#include "plugin_device.h"

LOCAL_LOGGER_ENABLE(_T("device_pi"), LOGGER_LEVEL_ERROR);
#include "application.h"

#define __CLASS_NAME__	CPluginDevice
BEGIN_COMMAND_DEFINATION
	COMMAND_DEFINE(_T("cardmode"),		ShowCardMode, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Show device card mode.")
	)

	COMMAND_DEFINE(_T("forcecm"),		ForceCardMode, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Force device card mode."),
		//PARDEF(_T("type"),		t, VT_STRING,	_T("set nand flash type.") )
		//PARDEF(_T("sectors"),	s, VT_USHORT,	_T("set f-chunk size.") )
		PARDEF(_T("chunk"),		k, VT_USHORT,	_T("set f-chunk size (sec).") )
		PARDEF(_T("page"),		p, VT_USHORT,	_T("set f-page size (sec).") )
		PARDEF(_T("block"),		b, VT_USHORT,	_T("set f-block size (sec)") )
		PARDEF(_T("interleave"),i, VT_USHORT,	_T("set interleave.") )
		PARDEF(_T("plane"),		l, VT_USHORT,	_T("set plane.") )
		PARDEF(_T("channel"),	c, VT_USHORT,	_T("set channel number.") )
	)

	COMMAND_DEFINE(_T("cleancache"),	CleanCacheBlock, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Clear cache block."),
		PARDEF(_T("clean"),		c, VT_BOOL,	_T("perform clean, default only show cache blocks.") )
		)

	COMMAND_DEFINE(_T("reset"),		CpuReset, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Reset CPU.")
		//, PARDEF(_T("clean"),		c, VT_BOOL,	_T("perform clean, default only show cache blocks.") )
		)

	COMMAND_DEFINE(_T("readflash"),	ReadFlash, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Read data in flash."),
		PARDEF(FN_BLOCK,	b, VT_USHORT,	_T("f-block ID.") )
		PARDEF(FN_PAGE,		p, VT_USHORT,	_T("f-page.") )
		PARDEF(FN_CHUNK,	k, VT_USHORT,	_T("f-chunk.") )
		PARDEF(_T("count"),	n, VT_UINT,		_T("length in sectors.") )
		PARDEF(_T("ecc"),	e, VT_BOOL,		_T("disable ecc engine.") )
		PARDEF(_T("scramb"),s, VT_UINT,		_T("disable scramble.") )
		//PARDEF(_T("mode"),	m, VT_UINT,		_T("mode.") )
		PARDEF(_T("all"),	h, VT_BOOL,		_T("do not ignore empty page") )
	)

	COMMAND_DEFINE(_T("sparedata"),	ReadSpareData, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Read spare data in flash."),
		PARDEF(FN_BLOCK,	b, VT_USHORT,	_T("f-block ID.") )
		PARDEF(FN_PAGE,		p, VT_USHORT,	_T("f-page.") )
		PARDEF(FN_CHUNK,	k, VT_USHORT,	_T("f-chunk.") )
		PARDEF(_T("count"),	n, VT_UINT,		_T("quantity of chunks .") )
		PARDEF(_T("all"),	h, VT_BOOL,		_T("do not ignore empty page") )
	)

	COMMAND_DEFINE(_T("writeflash"),	WriteFlash, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Write 1 page data to flash."),
		PARDEF(FN_BLOCK,		b, VT_USHORT,	_T("f-block ID.") )
		PARDEF(FN_PAGE,			p, VT_USHORT,	_T("f-page.") )
		PARDEF(_T("count"),		n, VT_USHORT,	_T("length in sectors.") )
	)

	COMMAND_DEFINE(_T("erase"),		EraseFlash, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Erase flash block."),
		PARDEF(FN_BLOCK,		b, VT_USHORT,	_T("f-block ID.") )
		PARDEF(_T("force"),		f, VT_BOOL,		_T("Erase block without confirm.") )
	)

	COMMAND_DEFINE(_T("readsec"),	ReadSectors, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Read sectors from device, using standard read command."), 
		PARDEF(FN_LOGIC_ADD,	a, VT_UINT64,	_T("LBA address for read.") )
		PARDEF(FN_SECTORS,		n, VT_UINT,		_T("Sectors for read") )
	)

	COMMAND_DEFINE(_T("writesec"),	WriteSectors, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Write sectors to device, using standard write command."), 
		PARDEF(FN_LOGIC_ADD,	a, VT_UINT64,	_T("LBA address for write. Default address from input data.") )
		PARDEF(FN_SECTORS,		n, VT_UINT,		_T("Sectors for read") )
		PARDEF(_T("flush"),		f, VT_BOOL,		_T("Flash cache after write") )		// to be implement
	)

	COMMAND_DEFINE(_T("readctrl"),		ReadControllerData, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		,	_T("Read data from controller, SRAM, FlashID, SFR, PAR."),
		PARDEF(_T("address"),		a, VT_USHORT,	_T("Start address.") )
		PARDEF(_T("length"),		l, VT_UINT,		_T("Length in byte.") )
		// categore set in param 0
		//PARDEF(_T("category"),	c, VT_STRING,	_T("Data category, SRAM, FID, SFR, PAR") )
	)

	COMMAND_DEFINE(_T("smarthist"),		SmartHistory, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Read All History of SMART.")
		, PARDEF(FN_BLOCK,		b, VT_USHORT,	_T("f-block ID.") )

	)

	COMMAND_DEFINE(_T("vendercmd"),		VenderCommand, 0, 0
		, _T("Execute a vendor command to read data."),
		PARDEF(_T("command"),	c, VT_USHORT,	_T("Command ID.") )
		PARDEF(FN_BLOCK,		b, VT_USHORT,	_T("F block ID.") )
		PARDEF(FN_PAGE,		p, VT_USHORT,	_T("physical page.") )
		PARDEF(FN_CHUNK,		k, VT_USHORT,	_T("chunk id.") )
		PARDEF(_T("count"),		n, VT_USHORT,	_T("length in sectors.") )
		PARDEF(_T("mode"),		m, VT_UCHAR,	_T("mode.") )
		PARDEF(_T("d[xyy]"),	0, VT_UCHAR,	_T("Set data directly, x:offset, yy:val (hex).") )
		PARDEF(_T("write"),		w, VT_UCHAR,	_T("Specify it is a write command, defalt is read.") )
	)

	COMMAND_DEFINE(_T("badblock"),		BadBlock, 0, 0
		, _T("Get bad block lists."),
		PARDEF(FN_BLOCK,	b,	VT_USHORT,	_T("F block ID.") )
		PARDEF(_T("dfailecc"),	d,	VT_BOOL,	_T("D Fail ECC.") )
	)

	COMMAND_DEFINE(_T("mpisp"),		DownloadMPISP, 0, 0
		, _T("Download MPISP.")
	)
	

END_COMMAND_DEFINATION
#undef __CLASS_NAME__

/*
const CCmdManager CPluginDevice::m_cmd_man(CCmdManager::CRule<CPluginDevice>()
	(_T("dumpisp"),		&CPluginDevice::DumpISP,	0, _T("Dump ISP to variable.") )
	// 关于checkcid和setcid的参数
	// checkcid：	各种选项使用普通参数，即--<参数全名>或者-<参数缩写>
	//				对于需要显示的CID项目，用command表示，即不带有--或者-的参数。这样CID项目与选项不冲突
	//				以后考虑对CID项目名称使用通配符，这样可以省略all选项
	(_T("checkcid"),	&CPluginDevice::CheckCID,	0, _T("Dump CID table and show data."), 0, jcparam::CCmdLineParser::CRule()
		(_T("all"),		_T('a'), jcparam::VT_BOOL, _T("Show all items, ignore item options."))
		(_T("showraw"),	_T('s'), jcparam::VT_BOOL, _T("Show raw data."))			// <TODO>
		(_T("reload"),	_T('r'), jcparam::VT_BOOL, _T("Force reload CID table."))		
		(_T("list"),	_T('l'), jcparam::VT_BOOL, _T("List all CID names."))
		(_T("hex"),		_T('x'), jcparam::VT_BOOL, _T("Show items in hex value."))		// <TODO>
		(_T("decimal"),	_T('d'), jcparam::VT_BOOL, _T("Show items in decimal value."))	// <TODO>
	)
	// setcid：		各种选项仅使用缩写，即-<选项>。选项的名称前加#表示隐藏现象，help是也不显示
	//				对于设置的CID项目，使用--<CID项目>=<值>。这样避免冲突
	(_T("setcid"),		&CPluginDevice::ModifyCID,	0, _T("Change CID setting and download."), 0, jcparam::CCmdLineParser::CRule()
		(_T("#reload"),	_T('r'), jcparam::VT_BOOL, _T("Force reload CID table before modify."))
		(_T("#save"),	_T('s'), jcparam::VT_BOOL, _T("Save CID after modify."))
	)
	(_T("linkinfo"),	&CPluginDevice::LinkInfo,	0, _T("Get a link information from logic add to physical add."), 0, jcparam::CCmdLineParser::CRule()
		(_T("lba"),		_T('l'), jcparam::VT_UINT64, _T("Link info for specified LBA."))
		(_T("begin"),	_T('b'), jcparam::VT_UINT, _T("Start H block ID."))
		(_T("end"),		_T('e'), jcparam::VT_UINT, _T("End H block ID."))
	)
	(_T("layout"),		&CPluginDevice::Layout,	0,	_T("FBlock layout."), 0, jcparam::CCmdLineParser::CRule()
		//(_T("add"),		_T('a'), jcparam::VT_USHORT,	_T("Start address."))
		//(_T("length"),	_T('l'), jcparam::VT_UINT,		_T("Length in byte."))
	)

	(_T("setmaxlba"),		&CPluginDevice::SetMaxAddress, 0,	_T("Set max address."), 0, jcparam::CCmdLineParser::CRule()
		(_T("address"),		_T('a'), jcparam::VT_UINT64,	_T("Address in lba."))
		(_T("readnative"),	_T('r'), jcparam::VT_BOOL,	_T("Read native max address only. Do not set max address."))
		(_T("restore"),		_T('s'), jcparam::VT_BOOL,	_T("Restore max address to native."))
		(_T("volatile"),	_T('v'), jcparam::VT_BOOL,	_T("Set max value volatile (sec_cnt = 1)"))
		(_T("lba48"),		_T('4'), jcparam::VT_BOOL,	_T("Using 48-bit address feature set"))
	)

);
*/

CPluginDevice::CPluginDevice(void)
	: m_cid(NULL)
{
	CSvtApplication * app = CSvtApplication::GetApp();
	app->GetDevice(m_smi_dev);
	JCASSERT(m_smi_dev);
	m_smi_dev->GetCardInfo(m_card_info);
}

CPluginDevice::~CPluginDevice(void)
{
	ClearCID();
	if (m_smi_dev) m_smi_dev->Release();
}

bool CPluginDevice::Regist(CPluginManager * manager)
{
	//LOG_STACK_TRACE();
	//manager->RegistPlugin(
	//	PLUGIN_NAME, NULL, CPluginManager::CPluginInfo::PIP_SINGLETONE, 
	//	CPluginDevice::Create);
	return true;
}

jcscript::IPlugin * CPluginDevice::Create(jcparam::IValue * param)
{
	return static_cast<jcscript::IPlugin*>(new CPluginDevice());
}

bool CPluginDevice::DumpISP(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(varout == NULL);
	JCASSERT(m_smi_dev);

	// Download ISP
	_tprintf(_T("Reading ISP...\r\n"));

	IIspBuf * isp = NULL;
	m_smi_dev->ReadISP(isp);

	varout = static_cast<jcparam::IValue*>(isp);
	varout->AddRef();

	// Download ISP
	_tprintf(_T("Done.\r\n"));
	isp->Release();
	return true;
}

void CPluginDevice::ClearCID()
{
	if (m_cid)
	{
		m_cid->Release();
		m_cid = NULL;
	}
}

bool CPluginDevice::ModifyCID(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(m_smi_dev);

	bool is_reload=false;
	argu.GetValT(_T("#reload"), is_reload);
	bool is_save=false;
	argu.GetValT(_T("#save"), is_save);

	if ( is_reload ) ClearCID();

	if (!m_cid)
	{
		// Download ISP
		_tprintf(_T("Reading CID...\r\n"));
		m_smi_dev->ReadCID(m_cid);
	}
	JCASSERT(m_cid);

	// Modify CID
	jcparam::CParamSet::ITERATOR it, endit = argu.End();
	for (it = argu.Begin(); it != endit; ++ it)
	{
		const CJCStringT & str_name = it->first;
		if ( str_name[0] != _T('#') )
		{
			CJCStringT str_val;
			jcparam::IValueConvertor * conv = dynamic_cast<jcparam::IValueConvertor*>(it->second);
			if (conv) conv->GetValueText(str_val);
			DWORD val = _tstoi( str_val.c_str() );
			m_cid->SetValue(str_name, val);
		}
	}

	if ( is_save )
	{
		_tprintf(_T("Downloading CID...\r\n"));
		m_cid->Save();
		ClearCID();
		_tprintf(_T("Save CID OK.\r\n"));
	}

	if (m_cid)
	{
		varout = m_cid;
		varout->AddRef();
	}

	return true;
}

bool CPluginDevice::CheckCID(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	// Download ISP
	JCASSERT(m_smi_dev);

	bool list = false;
	argu.GetValT(_T("list"), list);
	if ( list )
	{
		const CCidMap * defs = m_smi_dev->GetCidDefination();
		CCidMap::CONST_ITERATOR it = defs->Begin();
		CCidMap::CONST_ITERATOR endit = defs->End();
		for ( ; it != endit; ++it)
		{
			LPCTSTR desc = it->second->m_desc;
			_tprintf(_T("  %s : %s\r\n"), it->first.c_str(), desc?(desc):_T(""));
		}
		return true;
	}

	bool is_reload=false;
	argu.GetValT(_T("reload"), is_reload);
	if ( is_reload ) ClearCID();

	if ( !m_cid)
	{
		_tprintf(_T("Reading CID...\r\n"));
		m_smi_dev->ReadCID(m_cid);
	}
	JCASSERT(m_cid);

	varout = static_cast<jcparam::IValue *>(m_cid);
	varout->AddRef();

	//JCSIZE cmds = argu.GetCommandSize();

	JCSIZE ii = 0;
	CJCStringT key;
	while ( argu.GetCommand(ii++, key) )
	{
		DWORD val = m_cid->GetValue(key);
		_tprintf(_T("%s = 0x%X (%d)\r\n"), key.c_str(), val, val);
	}

	//for (JCSIZE ii=0; ii<cmds; ++ii)
	//{
	//	argu.GetCommand(ii, key);
	//	DWORD val = m_cid->GetValue(key);
	//	_tprintf(_T("%s = 0x%X (%d)\r\n"), key.c_str(), val, val);
	//}
	return true;
}

bool CPluginDevice::LinkInfo(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	/*
	JCASSERT(NULL == varout);
	JCASSERT(m_smi_dev);

	CCardInfo card_info;
	m_smi_dev->GetCardInfo(card_info);
	WORD max_f_block = card_info.m_f_block_num;

	stdext::auto_cif<IStorageDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_STORAGE_DEVICE, storage);
	JCASSERT(storage.valid());
	ULONGLONG max_lba = storage->GetCapacity();

	JCSIZE page_size = card_info.m_f_ckpp * card_info.m_f_spck;
	ULONGLONG max_h_block = (max_lba - 1) / (page_size * card_info.m_f_ppb) + 1;

	stdext::auto_interface<CLinkTable> link;
	CLinkTable::Create((JCSIZE)max_h_block, link);

	int old_percent = 0;
	_tprintf(_T("Reading Data ..."));
	CSpareData spare;
	for ( WORD bb = 0; bb < max_f_block; ++bb)	
	{
		// Update status
		int cur_percent = bb * 100 / max_f_block;
		if (cur_percent > old_percent)
		{
			_tprintf(_T("\rReading Data : %d%% ..."), cur_percent);
			old_percent = cur_percent;
		}
		if ( (spare.m_id < 0xE) && (spare.m_hblock < max_h_block) )
		{
		}
	}
	_tprintf(_T("\r\nDone.\r\n"));

	varout = static_cast<jcparam::IValue*>(link);
	varout->AddRef();
	*/
	return true;
}

bool CPluginDevice::BadBlock(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(NULL == varout);
	JCASSERT(m_smi_dev);

	ISmiDevice::BAD_BLOCK_LIST bad_block_list;
	JCSIZE new_bad_num = m_smi_dev->GetNewBadBlocks(bad_block_list);

	typedef jcparam::CTypedTable<CBadBlockInfo>	CBadBlockTable;
	stdext::auto_interface<CBadBlockTable> table;
	CBadBlockTable::Create(new_bad_num, table);

	ISmiDevice::BAD_BLOCK_LIST::iterator it;
	ISmiDevice::BAD_BLOCK_LIST::iterator endit = bad_block_list.end();

	for ( it = bad_block_list.begin(); it != endit; ++ it)
	{
		table->push_back(*it);
	}

	table.detach(varout);
	return true;
}

bool CPluginDevice::VenderCommand(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == varout);
	JCASSERT(m_smi_dev);
	
	bool is_write = false;
	argu.GetValT(_T("write"), is_write);

	WORD cmd_id = 0, block = 0;
	argu.GetValT(_T("command"), cmd_id);
	argu.GetValT(FN_BLOCK, block);

	BYTE page = 0, chunk = 0, mode = 0;
	argu.GetValT(FN_PAGE, page);
	argu.GetValT(FN_CHUNK, chunk);
	argu.GetValT(_T("mode"), mode);

	JCSIZE count = 0;

	CSmiBlockCmd cmd;
	cmd.id(cmd_id);
	cmd.block(block);
	cmd.page() = page;
	cmd.sector() = chunk;
	cmd.mode() = mode;

	// check --d parameters
	jcparam::CParamSet::ITERATOR it, endit = argu.End();
	for (it = argu.Begin(); it != endit; ++ it)
	{
		const CJCStringT & str_name = it->first;
		if ( _T('d') == str_name[0] )
		{
			if (str_name.size() != 4) 
				THROW_ERROR(ERR_PARAMETER, _T("Parameter %s has a wrong length."), str_name.c_str());
			JCSIZE offset = stdext::char2hex(str_name[1]);
			BYTE val = (BYTE)((stdext::char2hex(str_name[2]) << 4) | stdext::char2hex(str_name[3]));
			cmd.raw_byte(offset) = val;
		}
	}

	//READWRITE rw;
	JCSIZE data_size = 0;

	if (is_write)
	{
		stdext::auto_interface<CBinaryBuffer> buf;
		RetrieveBuffer(argu, varin, buf);
		//buf = dynamic_cast<CBinaryBuffer *>(varin);
		//if (!buf) THROW_ERROR(ERR_PARAMETER, _T("Missing input data"));
		//buf->AddRef();
		data_size = buf->GetSize();
		JCSIZE _secs = data_size / SECTOR_SIZE;
		JCSIZE _input_sec = _secs;
		argu.GetValT(_T("count"), _input_sec);
		count = min(_input_sec, _secs);
		//rw = write;
		cmd.size() = count;
		m_smi_dev->VendorCommand(cmd, write, buf->Lock(), count);
		buf->Unlock();
	}
	else
	{
		stdext::auto_interface<CBinaryBuffer> buf;
		count = 0;
		argu.GetValT(_T("count"), count);
		data_size = count * SECTOR_SIZE;
		CBinaryBuffer::Create(data_size, buf);
		//rw = read;
		cmd.size() = count;
		m_smi_dev->VendorCommand(cmd, read, buf->Lock(), count);
		buf->Unlock();
		buf.detach(varout);
	}
	//if ( !is_write && count > 0)	
	return true;
}

bool CPluginDevice::ReadFlash(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();

	JCASSERT(NULL == varout);
	JCASSERT(m_smi_dev);

	// 如果指定长度：所有地址的缺省值为0，读取指定长度。
	// 如果没有指定长度：
	//		如果没有指定 chunk, 读取整个page
	//		如果没有指定 page，读取整个block

	WORD block = 0xFFFF, page = 0xFFFF, chunk = 0xFFFF;
	argu.GetValT(FN_BLOCK, block);
	argu.GetValT(FN_PAGE, page);
	argu.GetValT(FN_CHUNK, chunk);

	JCSIZE secs = 0xFFFFFFFF;
	argu.GetValT(_T("count"), secs);

	bool ignore = !argu.Exist(_T("all"));

	if ( secs != 0xFFFFFFFF )
	{	// sectors exist
		if (0xFFFF == chunk)	chunk = 0;
		if (0xFFFF == page)		page = 0;
		if (0xFFFF == block)	block = 0;
	}
	else
	{
		secs = ChunkSize();
		if (0xFFFF == chunk)
		{
			secs = PageSize();
			chunk = 0;
			if (0xFFFF == page)
			{
				secs = BlockSize();
				page = 0;
				if (0xFFFF == block) THROW_ERROR(ERR_PARAMETER, _T("Error. Missing parameter"));
			}
		}
	}

	UINT option = 0;
	//bool ecc = true;		// default enable ecc engine
	if (argu.Exist(_T("ecc"))) option |= ISmiDevice::RFO_DISABLE_ECC;		// disable ecc engine
	if (argu.Exist(_T("scramb"))) option |= ISmiDevice::RFO_DISABLE_SCRAMB;		// disable ecc engine

	//BYTE mode = 0;
	//argu.GetValT(_T("mode"), mode);

	JCSIZE chunk_size = ChunkSize();
	JCSIZE chunk_len = SECTOR_SIZE * chunk_size;

	LOG_TRACE(_T("Start reading flash ..."));
	stdext::auto_array<BYTE>	_buf( SECTOR_SIZE * (secs + chunk_size) );
	BYTE * buf = (BYTE*)(_buf);

	CFlashAddress add(block, page, chunk);

	JCSIZE read_secs = 0;

	while (read_secs < secs )
	{
		CSpareData spare;
		m_smi_dev->ReadFlashChunk(add, spare, buf, chunk_size + 1, option);
		if (0xFF == spare.m_id && 0xFFFF == spare.m_hblock && ignore)	break;
		read_secs += chunk_size;
		// increase address
		IncreaseFlashAddress(add);
		buf += chunk_len;
	}

	JCSIZE len = SECTOR_SIZE * min(read_secs, secs);

	stdext::auto_interface<CBinaryBuffer>	data;
	CBinaryBuffer::Create(len, data);
	memcpy_s(data->Lock(), len, _buf, len);
	data->Unlock();
	data->SetFlashAddress(add);
	varout = static_cast<jcparam::IValue*>(data);
	varout ->AddRef();
	return true;
}

bool CPluginDevice::ReadSpareData(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == varout);
	JCASSERT(m_smi_dev);

	// 如果指定长度：所有地址的缺省值为0，读取指定长度。
	// 如果没有指定长度：
	//		如果没有指定 chunk, 读取整个page
	//		如果没有指定 page，读取整个block
	bool all = argu.Exist(_T("all"));

	WORD block = 0xFFFF, page = 0xFFFF, chunk = 0xFFFF;
	argu.GetValT(FN_BLOCK, block);
	argu.GetValT(FN_PAGE, page);
	argu.GetValT(FN_CHUNK, chunk);

	JCSIZE chunks = 0xFFFFFFFF;
	argu.GetValT(_T("count"), chunks);

	if ( chunks != 0xFFFFFFFF )
	{	// sectors exist
		if (0xFFFF == chunk)	chunk = 0;
		if (0xFFFF == page)		page = 0;
		if (0xFFFF == block)	block = 0;
	}
	else
	{
		chunks = 1;
		if (0xFFFF == chunk)
		{
			// read all chunks
			chunks = m_card_info.m_f_ckpp;
			chunk = 0;
			if (0xFFFF == page)
			{
				// read all pages
				chunks = m_card_info.m_f_ckpp * m_card_info.m_f_ppb;
				page = 0;
				if (0xFFFF == block) THROW_ERROR(ERR_PARAMETER, _T("Error. Missing parameter"));
			}
		}
	}	


	JCSIZE chunk_size = m_card_info.m_f_spck + 1;
	JCSIZE buf_size = chunk_size * SECTOR_SIZE;
	stdext::auto_array<BYTE> buf(buf_size);

	CFlashAddress add(block, page, chunk);

	stdext::auto_interface<CBlockTable>	block_table;
	CBlockTable::Create(chunks, block_table);

	for (JCSIZE kk = 0; kk < chunks; ++kk)
	{
		CSpareData spare;
		m_smi_dev->ReadFlashChunk(add, spare, buf, chunk_size);
		if ( 0xFF != spare.m_id || all )	block_table->push_back(CFBlockInfo(add, spare) );
		IncreaseFlashAddress(add);
	}

	CFBlockInfo::m_channel_num = m_card_info.m_channel_num;

	block_table.detach(varout);
	return true;
}

bool CPluginDevice::WriteFlash(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();
	JCASSERT(m_smi_dev);

	//stdext::auto_cif<CBinaryBuffer, jcparam::IValue>		arg0;
	//CBinaryBuffer * buf = dynamic_cast<CBinaryBuffer*>(varin);

	//if (!buf) 
	//{
	//	if ( argu.GetCommand(0, arg0) )
	//	{
	//		if ( !arg0.valid() ) THROW_ERROR(ERR_PARAMETER, _T("Missing input data or wrong"));
	//		buf = arg0.d_cast<CBinaryBuffer*>();
	//	}
	//}
	//if (!buf) THROW_ERROR(ERR_PARAMETER, _T("Missing input data or wrong"));

	stdext::auto_interface<CBinaryBuffer> buf;
	RetrieveBuffer(argu, varin, buf);


	JCSIZE buf_len = buf->GetSize();
	JCSIZE secs = buf_len / SECTOR_SIZE;

	WORD block = 0xFFFF, page = 0xFFFF;
	argu.GetValT(FN_BLOCK, block);
	if ( 0xFFFF == block )	THROW_ERROR(ERR_PARAMETER, _T("Missing f-block id"));
	argu.GetValT(FN_PAGE, page);
	if ( 0xFFFF == page )	THROW_ERROR(ERR_PARAMETER, _T("Missing f-page id"));
	JCSIZE count = 0xFFFFFFFF;
	argu.GetValT(_T("count"), count);
	if ( secs > count) secs = count;

	JCSIZE remain_secs = secs;

	CFlashAddress add(block, page);
	BYTE * _buf = buf->Lock();

	while (remain_secs > 0)
	{
		JCSIZE written = m_smi_dev->WriteFlash(add, _buf, remain_secs);
		
		remain_secs -= written;
		_buf += written * SECTOR_SIZE;
		add.m_page ++;
		if (add.m_page >= m_card_info.m_f_ppb)	add.m_block ++, add.m_page = 0;
	}
	buf->Unlock();
	return true;
}

bool CPluginDevice::EraseFlash(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(NULL == varout);
	JCASSERT(m_smi_dev);

	WORD block = 0xFFFF;
	argu.GetValT(FN_BLOCK, block);

	bool force = argu.Exist(_T("force"));
	if (!force)
	{
		stdext::jc_printf(_T("Are you sure to erase block 0x%04X?\n"), block);
		if ( _T('y') != getchar() ) return false;
	}

	CFlashAddress add(block);
	m_smi_dev->EraseFlash(add);

	return true;
}

bool CPluginDevice::ReadControllerData(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(NULL == varout);
	JCASSERT(m_smi_dev);

	CJCStringT category;
	if ( !argu.GetCommand(0, category) ) THROW_ERROR(ERR_USER, _T("Missing category"));
	
	//argu.GetValT(_T("category"), category);

	WORD address = 0;
	argu.GetValT(_T("address"), address);

	JCSIZE length = SECTOR_SIZE;
	argu.GetValT(_T("length"), length); 
	
	stdext::auto_interface<CBinaryBuffer>	data;
	CBinaryBuffer::Create(length, data);

	BYTE * buf = data->Lock();

	if ( category == _T("SRAM") )
	{
		m_smi_dev->ReadSRAM(address, length, buf );
		//data->SetOffset(address);
		data->SetMemoryAddress(address);
	}
	else if ( category == _T("SFR") )
	{
		stdext::auto_array<BYTE> _buf(SECTOR_SIZE);
		m_smi_dev->ReadSFR(_buf, 1);
		if ( (length + address) > SECTOR_SIZE) length = SECTOR_SIZE - address;
		stdext::jc_memcpy(buf, length, _buf + address, length);
	}
	else if ( category == _T("FID") )
	{
		stdext::auto_array<BYTE> _buf(SECTOR_SIZE);
		m_smi_dev->ReadFlashID(_buf, 1);
		if ( (length + address) > SECTOR_SIZE) length = SECTOR_SIZE - address;
		stdext::jc_memcpy(buf, length, _buf + address, length);
	}
	else
	{
		THROW_ERROR(ERR_USER, _T("unknow category %s"), category.c_str());	
	}

	data->Unlock();

	varout = static_cast<jcparam::IValue*>(data);
	varout ->AddRef();

	return true;
}

//bool CPluginDevice::Layout(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
//{
//	typedef jcparam::CTypedTable<CFBlockInfo>		CFBlockTable;
//	CFBlockTable * table = NULL;
//	CFBlockTable::Create(10, table);
//	table->push_back( CFBlockInfo(0, CFBlockInfo::FBT_MOTHER, 100, 0) );
//	table->push_back( CFBlockInfo(1, CFBlockInfo::FBT_MOTHER, 200, 5) );
//	table->push_back( CFBlockInfo(2, CFBlockInfo::FBT_MOTHER, 300, 10) );
//
//	varout = static_cast<jcparam::IValue*>(table);
//
//	return true;
//}

	
bool CPluginDevice::SetMaxAddress(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(varout == NULL);
	JCASSERT(m_smi_dev);

	stdext::auto_cif<IAtaDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_ATA_DEVICE, storage);
	if (!storage.valid())	THROW_ERROR(ERR_UNSUPPORT, _T("Device is not an ATA device"));

	bool read_only = false;
	argu.GetValT(_T("readnative"), read_only);

	bool lba48_feature = false;
	argu.GetValT(_T("lba48"), lba48_feature);

	bool restore = false;
	argu.GetValT(_T("restore"), restore);

	UINT64 add = 0;
	argu.GetValT(FN_LOGIC_ADD, add);

	bool val_vol = false;
	argu.GetValT(_T("volatile"), val_vol);

	// Read Native
	ATA_REGISTER reg_cur, reg_pre;
	memset(&reg_cur, 0, sizeof(ATA_REGISTER) );
	memset(&reg_pre, 0, sizeof(ATA_REGISTER) );
	UINT cur_add_lo = 0, cur_add_hi = 0;

	if (lba48_feature)
	{
		reg_pre.command = 0x27;
		reg_cur.command = 0x27;
		reg_pre.device = 0xE0;
		reg_cur.device = 0xE0;
		storage->AtaCommand48(reg_pre, reg_cur, read, NULL, 0);
		if (reg_cur.status & 0x1)
			THROW_ERROR(ERR_DEVICE, _T("Failure on reading native max address. Error = 0x%02X"), reg_cur.error); 
		cur_add_lo = MAKELONG(MAKEWORD(reg_cur.lba_low, reg_cur.lba_mid), MAKEWORD(reg_cur.lba_hi, reg_pre.lba_low) );
		cur_add_hi = MAKELONG(MAKEWORD(reg_pre.lba_mid, reg_pre.lba_hi), 0 );
		_tprintf(_T("Native max address = 0x%08X-%08X\n"), cur_add_hi, cur_add_lo);
	}	
	else
	{
		reg_cur.command = 0xF8;
		reg_cur.device = 0xE0;
		storage->AtaCommand(reg_cur, read, false, NULL, 0);
		if (reg_cur.status & 0x1)
			THROW_ERROR(ERR_DEVICE, _T("Failure on reading native max address. Error = 0x%02X"), reg_cur.error); 
		cur_add_lo = MAKELONG(MAKEWORD(reg_cur.lba_low, reg_cur.lba_mid), MAKEWORD(reg_cur.lba_hi, reg_cur.device & 0x0F) );
		_tprintf(_T("Native max address = 0x%08X\n"), (UINT)(cur_add_lo));
	}

	if (restore)	add = stdext::MAKEUINT64(cur_add_lo, cur_add_hi);

	// Set Max Address
	if ( !read_only)
	{
		if (lba48_feature)
		{
			memset(&reg_cur, 0, sizeof(ATA_REGISTER) );
			memset(&reg_pre, 0, sizeof(ATA_REGISTER) );
			reg_cur.command = 0x37;
			reg_pre.command = 0x37;

			reg_cur.lba_low = BYTE(add & 0xFF);
			reg_cur.lba_mid = BYTE( (add >> 8) & 0xFF);
			reg_cur.lba_hi = BYTE( (add >> 16) & 0xFF);
			reg_pre.lba_low = BYTE( (add >> 24) & 0xFF);
			reg_pre.lba_mid = BYTE( (add >> 32) & 0xFF);
			reg_pre.lba_hi = BYTE( (add >> 40) & 0xFF);

			if (!val_vol)	reg_cur.sec_count = 1;

			reg_cur.device = 0xE0;
			storage->AtaCommand48(reg_pre, reg_cur, read, NULL, 0);
			_tprintf(_T("Status = 0x%02X, Error = 0x%02X\n"), reg_cur.status, reg_cur.error);
			if (reg_cur.status & 0x1)
				THROW_ERROR(ERR_DEVICE, _T("Failure on setting max address. Error = 0x%02X"), reg_cur.error); 
		}
		else
		{
			memset(&reg_cur, 0, sizeof(ATA_REGISTER) );
			reg_cur.command = 0xF9;
			reg_cur.lba_low = BYTE(add & 0xFF);
			reg_cur.lba_mid = BYTE( (add >> 8) & 0xFF);
			reg_cur.lba_hi = BYTE( (add >> 16) & 0xFF);
			if (!val_vol)	reg_cur.sec_count = 1;
			reg_cur.device = 0xE0 | BYTE( (add >> 24) & 0x0F);
			storage->AtaCommand(reg_cur, read, false, NULL, 0);
			_tprintf(_T("Status = 0x%02X, Error = 0x%02X\n"), reg_cur.status, reg_cur.error);
			if (reg_cur.status & 0x1)
				THROW_ERROR(ERR_DEVICE, _T("Failure on setting max address. Error = 0x%02X"), reg_cur.error); 
		}
	}

	return true;
}

bool CPluginDevice::GetDeviceInfo(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(varout == NULL);
	JCASSERT(m_smi_dev);

	//CCardInfo card_info;
	//m_smi_dev->GetCardInfo(card_info);

	stdext::jc_printf(_T("Controller : %ls\n"), m_card_info.m_controller_name.c_str());
	stdext::jc_printf(_T("NAND Flash type: "));
	switch (m_card_info.m_type)
	{
	case CCardInfo::NT_SLC:	stdext::jc_printf(_T("SLC\n")); break;
	case CCardInfo::NT_SLC_MODE:	stdext::jc_printf(_T("SLC-M\n")); break;
	case CCardInfo::NT_MLC:	stdext::jc_printf(_T("MLC\n"));	break;
	}
	stdext::jc_printf(_T("Channel: %d, Interleave: %d, Plane: %d\n"), m_card_info.m_channel_num, m_card_info.m_interleave, m_card_info.m_plane);
	stdext::jc_printf(_T("Blocks : 0x%X, (%d)\n"), BlockNum(), BlockNum());
	stdext::jc_printf(_T("Page per block : 0x%X, (%d)\n"), m_card_info.m_f_ppb, m_card_info.m_f_ppb);
	JCSIZE page_size = PageSize();
	stdext::jc_printf(_T("Page size : %dKB, (%d secs)\n"), page_size / 2, page_size);
	JCSIZE block_size = BlockSize();
	stdext::jc_printf(_T("Block size : %dKB, (%d secs)\n"), block_size / 2, block_size);

	return true;
}


bool CPluginDevice::GetSpare(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(NULL == varout);
	JCASSERT(m_smi_dev);
	
	WORD block = 0;
	argu.GetValT(FN_BLOCK, block);

	WORD page = 0, sector = 0;
	argu.GetValT(FN_PAGE, page);
	argu.GetValT(_T("sector"), sector);

	CFlashAddress add(block, page, sector);

	CSpareData spare;
	//m_smi_dev->GetSpareData(add, spare);

	stdext::jc_printf(_T("id, h-block, h-page, ecc-code \n"));
	stdext::jc_printf(_T("%02X, %04X, %04X, %02X \n"), spare.m_id, spare.m_hblock, spare.m_hpage, spare.m_ecc_code);

	return true;
}


bool CPluginDevice::SmartHistory(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();
	JCASSERT(varout == NULL);
	JCASSERT(m_smi_dev);

	const CSmartAttrDefTab * smart_def_tab = m_smi_dev->GetSmartAttrDefTab();

	stdext::auto_interface<jcparam::CDynaTab> smart_table;
	jcparam::CDynaTab::Create(smart_table);

	// Add head according to device smart defination
	JCSIZE smart_def_size = smart_def_tab->GetSize();
	for (JCSIZE ii=0, jj=0; ii < smart_def_size; ++ii)
	{
		const CSmartAttributeDefine * def = smart_def_tab->GetItem(ii);
		JCASSERT(def);
		if (def->m_using)
		{
			TCHAR title[128];
			stdext::jc_sprintf(title, 128, _T("0x%s (%s)"), def->m_str_id, def->m_name);
			smart_table->AddColumn(title);
			jj++;
		}
	}

	UINT block = 0xFFFF;
	argu.GetValT(FN_BLOCK, block);
	if ( 0xFFFF == block )
	{
		//stdext::auto_interface<jcparam::IValue> val;
		//UINT _b;
		bool br = m_smi_dev->GetProperty(CSmiDeviceBase::PROP_WPRO, block);
		//if (br && val) jcparam::GetSimpleValue(val, block);
		//if (br) block = (WORD)_b;
	}


	JCSIZE chunk_size = m_card_info.m_f_spck + 1;
	JCSIZE buf_size = chunk_size * SECTOR_SIZE;
	JCSIZE page_num = PageNum();

	CFlashAddress add(block);
	stdext::auto_array<BYTE> smart_buf(buf_size);
	
	bool print_head = true;
	for ( ; add.m_page < page_num; add.m_page++, add.m_chunk = 0)
	//while (add.m_page < page_num)
	{
		CSpareData spare;
		m_smi_dev->ReadFlashChunk(add, spare, smart_buf, chunk_size);

		//m_smi_dev->GetSpareData(add, spare);
		if (0xFF == spare.m_id) continue;

		stdext::auto_interface<jcparam::CDynaRow> row;
		smart_table->AddRow(row);

		JCSIZE offset = 2;
		for (JCSIZE ii=0, jj=0; ii < smart_def_size; ++ii, offset += 12)
		{
			const CSmartAttributeDefine * def = smart_def_tab->GetItem(ii);
			JCASSERT(def);
			if (def->m_using)
			{
				BYTE attr_id = smart_buf[offset];
				DWORD lo = *(DWORD*) (smart_buf + offset + 5);
				stdext::auto_interface<jcparam::CTypedValue<UINT> > val(jcparam::CTypedValue<UINT>::Create(lo) );
				row->SetColumnVal(jj++, static_cast<jcparam::IValue*>(val) );		
			}
		}
		//add.m_page ++ ;
		//add.m_chunk = 0;
	}
	smart_table.detach(varout);
	return true;
}


bool CPluginDevice::WriteSectors(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();
	JCASSERT(m_smi_dev);
	stdext::auto_cif<IStorageDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_STORAGE_DEVICE, storage);
	JCASSERT(storage.valid());

	stdext::auto_interface<CBinaryBuffer> buf;
	RetrieveBuffer(argu, varin, buf);

	//stdext::auto_cif<CBinaryBuffer, jcparam::IValue>		arg0;
	//CBinaryBuffer * buf = dynamic_cast<CBinaryBuffer*>(varin);
	//if (!buf) 
	//{
	//	if ( argu.GetCommand(0, arg0) )
	//	{
	//		if ( !arg0.valid() ) THROW_ERROR(ERR_PARAMETER, _T("Missing input data or wrong"));
	//		buf = arg0.d_cast<CBinaryBuffer*>();
	//	}
	//}
	//if (!buf) THROW_ERROR(ERR_PARAMETER, _T("Missing input data or wrong"));

	FILESIZE max_lba = storage->GetCapacity();
	FILESIZE lba = 0;
	if ( !argu.GetValT(FN_LOGIC_ADD, lba) )
	{
		// address is not set, read address from input data
		stdext::auto_interface<IAddress>	add;
		buf->GetAddress(add);
		if ( add && (IAddress::ADD_BLOCK == add->GetType() ) )
		{
			stdext::auto_cif<jcparam::CTypedValue<FILESIZE>, jcparam::IValue>	val;
			add->GetSubValue(FN_LOGIC_ADD, val);
			lba = (FILESIZE)(*val);
		}
	}
	if (lba > max_lba)	THROW_ERROR(ERR_PARAMETER, _T("Address out of range. Max lba = 0x%08X"), max_lba);


	JCSIZE buf_len = buf->GetSize();

	JCSIZE secs = buf_len / SECTOR_SIZE;
	argu.GetValT(FN_SECTORS, secs);

	if (secs * SECTOR_SIZE > buf_len)	THROW_ERROR(ERR_PARAMETER, _T("Sectors is larger than input data"));

	storage->SectorWrite(buf->Lock(), lba, secs);
	buf->Unlock();

	return true;
}

bool CPluginDevice::ReadSectors(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == varout);
	JCASSERT(m_smi_dev);
	stdext::auto_cif<IStorageDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_STORAGE_DEVICE, storage);
	JCASSERT(storage.valid());

	FILESIZE max_lba = storage->GetCapacity();
	FILESIZE lba = 0;
	argu.GetValT(FN_LOGIC_ADD, lba);
	if (lba > max_lba)	THROW_ERROR(ERR_PARAMETER, _T("Address out of range. Max lba = 0x%08X"), max_lba);

	JCSIZE secs = 1;
	argu.GetValT(FN_SECTORS, secs);

	LOG_DEBUG(_T("lba=0x%08X, sec=0x%X"), (UINT)lba, secs);
	//JCSIZE buf_len = secs * SECTOR_SIZE;
	stdext::auto_interface<CBinaryBuffer>	data;
	CBinaryBuffer::Create(SECTOR_SIZE * secs, data);
	storage->SectorRead(data->Lock(), lba, secs);
	data->Unlock();
	data->SetBlockAddress(lba);

	data.detach(varout);
	//_tprintf(_T("Completed.\r\n"));
	return true;
}

bool CPluginDevice::CleanCacheBlock(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();
	JCASSERT(m_smi_dev);

	UINT cache_num = 0;

	//stdext::auto_interface<jcparam::IValue> val;
	m_smi_dev->GetProperty(CSmiDeviceBase::PROP_CACHE_NUM, cache_num);
	//if (br && val) jcparam::GetSimpleValue(val, cache_num);
	
	//m_smi_dev->GetProperty(_T("CACHE_BLOCK_NUM"));
	stdext::jc_printf(_T("cache block before clean = %d \n"), cache_num);

	stdext::auto_cif<IStorageDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_STORAGE_DEVICE, storage);
	JCASSERT(storage.valid());

	if ( argu.Exist(_T("clean")) )
	{
		// clean cache
		CSmiBlockCmd cmd;
		cmd.id(0xF0F4);
		stdext::auto_array<BYTE> buf(SECTOR_SIZE);

		stdext::jc_printf(_T("cleaning... \n"));
		m_smi_dev->VendorCommand(cmd, read, buf, 1);

		//stdext::auto_interface<jcparam::IValue> val;
		m_smi_dev->GetProperty(CSmiDeviceBase::PROP_CACHE_NUM, cache_num);
		//if (br && val) jcparam::GetSimpleValue(val, cache_num);

		//cache_num = m_smi_dev->GetProperty(_T("CACHE_BLOCK_NUM"));
		stdext::jc_printf(_T("cache block after clean = %d \n"), cache_num);
	}

	return true;
}


bool CPluginDevice::CpuReset(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();
	JCASSERT(m_smi_dev);
	CSmiBlockCmd cmd;
	cmd.id(0xF02C);
	stdext::auto_array<BYTE> buf(SECTOR_SIZE);
	stdext::jc_printf(_T("CPU reseting... "));
	m_smi_dev->VendorCommand(cmd, read, buf, 1);
	stdext::jc_printf(_T("Done. \n"));
	return true;
}

bool CPluginDevice::ShowCardMode(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();
	JCASSERT(m_smi_dev);

	CCardInfo card_info;
	m_smi_dev->GetCardInfo(card_info);

	stdext::auto_interface<CAttributeTable>	card_mode_tab;
	CAttributeTable::Create(10, card_mode_tab);

	card_mode_tab->push_back(CAttributeItem(0, _T("controller"), 0, card_info.m_controller_name) );
	CJCStringT	nand_type;
	switch (card_info.m_type)
	{
	case CCardInfo::NT_SLC: nand_type = _T("SLC"); break;
	case CCardInfo::NT_MLC: nand_type = _T("MLC"); break;
	case CCardInfo::NT_SLC_MODE: nand_type = _T("SLC_MODE"); break;
	case CCardInfo::NT_TLC: nand_type = _T("TLC"); break;
	}
	card_mode_tab->push_back(CAttributeItem(1, _T("nand_type"), 0, nand_type) );

	static const JCSIZE STR_BUF_SIZE= 127;
	stdext::auto_array<TCHAR> str(STR_BUF_SIZE+1);

	stdext::jc_sprintf(str, STR_BUF_SIZE, _T("%dKB"), card_info.m_f_spck / 2);
	card_mode_tab->push_back(CAttributeItem(2, _T("f-chunk_size"), card_info.m_f_spck, (LPCTSTR)str) );

	JCSIZE page_size = card_info.m_f_spck * card_info.m_f_ckpp;
	stdext::jc_sprintf(str, STR_BUF_SIZE, _T("%dKB"), page_size / 2);
	card_mode_tab->push_back(CAttributeItem(3, _T("f-page_size"), page_size, (LPCTSTR)str) );

	JCSIZE block_size = page_size * card_info.m_f_ppb;
	if (block_size >= 2048)		stdext::jc_sprintf(str, STR_BUF_SIZE, _T("%dMB"), block_size / 2048);
	else						stdext::jc_sprintf(str, STR_BUF_SIZE, _T("%dKB"), block_size / 2);
	card_mode_tab->push_back(CAttributeItem(4, _T("f-block_size"), block_size, (LPCTSTR)str) );

	stdext::jc_sprintf(str, STR_BUF_SIZE, _T("%d"), card_info.m_f_block_num);
	card_mode_tab->push_back(CAttributeItem(5, _T("f-block_number"), card_info.m_f_block_num, (LPCTSTR)str) );

	card_mode_tab->push_back(CAttributeItem(6, _T("interleave"), card_info.m_interleave) );
	card_mode_tab->push_back(CAttributeItem(6, _T("plane"), card_info.m_plane) );

	card_mode_tab.detach(varout);
	return true;
}

bool CPluginDevice::ForceCardMode(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();
	JCASSERT(m_smi_dev);

	UINT mask = 0;
	if (argu.Exist(_T("chunk") ) )
	{
		argu.GetValT(_T("chunk"), m_card_info.m_f_spck);
		mask |= ISmiDevice::CIM_F_SPCK;
	}

	JCSIZE page = 0;
	argu.GetValT(_T("page"), page);
	if (page)
	{
		m_card_info.m_f_ckpp = page / m_card_info.m_f_spck;
		mask |= ISmiDevice::CIM_F_CKPP;
	}

	JCSIZE block = 0;
	argu.GetValT(_T("block"), block);

	if ( block )
	{
		m_card_info.m_f_ppb = block / ( m_card_info.m_f_ckpp * m_card_info.m_f_spck);
		mask |= ISmiDevice::CIM_F_PPB;

	}
	//	argu.GetValT(_T("block"), m_card_info.m_f_block_num);
	//	mask |= ISmiDevice::CIM_F_BLOCK_NUM;
	//}
		
	if (argu.Exist(_T("interleave") ) )
	{
		argu.GetValT(_T("interleave"), m_card_info.m_interleave);
		mask |= ISmiDevice::CIM_M_INTLV;
	}

	if (argu.Exist(_T("plane") ) )
	{
		argu.GetValT(_T("plane"), m_card_info.m_plane);
		mask |= ISmiDevice::CIM_M_PLANE;
	}
	if (argu.Exist(_T("channel") ) )
	{
		argu.GetValT(_T("channel"), m_card_info.m_channel_num);
		mask |= ISmiDevice::CIM_M_CHANNEL;
	}
	m_smi_dev->SetCardInfo(m_card_info, mask);
	return true;
}

bool CPluginDevice::DownloadMPISP(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();
	JCASSERT(m_smi_dev);

	stdext::auto_interface<CBinaryBuffer> buf;
	//CBinaryBuffer * buf = NULL;
	RetrieveBuffer(argu, varin, buf);

	JCSIZE buf_len = buf->GetSize();
	JCSIZE secs = 0;
	if (buf_len <= 16) THROW_ERROR(ERR_PARAMETER, _T("Data is too short."));
	secs = (buf_len - 16) / SECTOR_SIZE;

	BYTE* _buf = buf->Lock();
	if ( 0xF127 != MAKEWORD(_buf[1], _buf[0]) )	THROW_ERROR(ERR_PARAMETER, _T("Not MPISP data."));

	CSmiBlockCmd cmd;
	for (JCSIZE ii = 0; ii < 16; ++ii)	cmd.raw_byte(ii) = _buf[ii];

	m_smi_dev->VendorCommand(cmd, write, _buf + 16, secs);
	buf->Unlock();

	return true;
}

bool CPluginDevice::RetrieveBuffer(jcparam::CArguSet & argu, jcparam::IValue * varin, CBinaryBuffer * & buf)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == buf);

	stdext::auto_cif<CBinaryBuffer, jcparam::IValue>		arg0;
	buf = dynamic_cast<CBinaryBuffer*>(varin);

	if (!buf) 
	{
		if ( argu.GetCommand(0, arg0) )
		{
			if ( !arg0.valid() ) THROW_ERROR(ERR_PARAMETER, _T("Missing input data or wrong"));
			buf = arg0.d_cast<CBinaryBuffer*>();
		}
	}
	if (!buf) THROW_ERROR(ERR_PARAMETER, _T("Missing input data or wrong"));
	buf->AddRef();

	return true;
}
