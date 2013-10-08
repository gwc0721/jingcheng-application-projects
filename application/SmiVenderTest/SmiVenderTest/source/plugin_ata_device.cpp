#include "stdafx.h"

#include "plugin_ata_device.h"
#include "plugin_manager.h"
#include "application.h"

LOCAL_LOGGER_ENABLE(_T("ata_pi"), LOGGER_LEVEL_ERROR);

using namespace jcparam;

#define __CLASS_NAME__	CPluginAtaDevice
BEGIN_COMMAND_DEFINATION
	//COMMAND_DEFINE(_T("setfeature"),SetFeature,		0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
	//	, _T("Set feature for device."),
	//	PARDEF(FN_LOGIC_ADD,		a, VT_UINT64,	_T("LBA address for write.") )
	//	PARDEF(FN_SECTORS,		n, VT_UINT,		_T("Sectors for write") )
	//	PARDEF(_T("block_size"),	b, VT_UINT,		_T("Block size (in sectors) for one write command.") )
	//	PARDEF(_T("align"),		l, VT_BOOL,		_T("Align to block size.") )
	//)
	COMMAND_DEFINE(_T("idtable"),	IdentifyDevice,		0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Identify device."),
		PARDEF(_T("raw"),		r, VT_BOOL,		_T("Return raw data.") )
	)

	COMMAND_DEFINE(_T("read"),	ReadSectors,		0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Read sectors."),
		PARDEF(FN_LOGIC_ADD,	a, VT_UINT64,	_T("LBA address to read.") )
		PARDEF(FN_SECTORS,		n, VT_UINT,		_T("Sectors to read") )
		PARDEF(_T("pio"),		p, VT_BOOL,		_T("read using PIO mode") )
		PARDEF(_T("ext"),		e, VT_BOOL,		_T("force LBA48 command") )		// to be implement
	)

	COMMAND_DEFINE(_T("write"),	WriteSectors,		0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Write sectors."),
		PARDEF(FN_LOGIC_ADD,	a, VT_UINT64,	_T("LBA address to write.") )
		PARDEF(FN_SECTORS,		n, VT_UINT,		_T("Sectors to write") )
		PARDEF(_T("flush"),		f, VT_BOOL,		_T("Flash cache after write") )
		PARDEF(_T("pio"),		p, VT_BOOL,		_T("read using PIO mode") )
		PARDEF(_T("ext"),		e, VT_BOOL,		_T("force LBA48 command") )		// to be implement
	)

	COMMAND_DEFINE(_T("cmd"),	AtaCommand, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Pass throw an ATA command to device."),
		PARDEF(_T("command"),	c, VT_UCHAR,	_T("Command regist.") )
		PARDEF(_T("feature"),	f, VT_UCHAR,	_T("Feature regist.") )
		PARDEF(FN_LOGIC_ADD,	a, VT_UINT64,	_T("Address in lba.") )
		PARDEF(_T("device"),	d, VT_UCHAR,	_T("Hi 4 bits of device regist.") )
		PARDEF(_T("count"),		n, VT_USHORT,	_T("Contents of sector count regist.") )
		PARDEF(_T("size"),		s, VT_USHORT,	_T("Data size in sector. Default = sector count.") )
		PARDEF(_T("write"),		w, VT_BOOL,		_T("Default read, set to write.") )
		PARDEF(_T("ext"),		x, VT_BOOL,		_T("Force using LBA48 command") )
		PARDEF(_T("dma"),		m, VT_BOOL,		_T("Use DMA mode.") )
	)

	COMMAND_DEFINE(_T("smart"),	Smart, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Get SMART attribute of device."), 
		PARDEF(_T("raw"),		r, VT_BOOL,	_T("Return raw data.") )
		PARDEF(_T("unknow"),	u, VT_BOOL,	_T("Show unknown attribute.") )
	)




END_COMMAND_DEFINATION
#undef __CLASS_NAME__

CPluginAtaDevice::CPluginAtaDevice(jcparam::IValue * param)
	: m_smi_dev(NULL)
{
	CSvtApplication * app = CSvtApplication::GetApp();
	app->GetDevice(m_smi_dev);

}


CPluginAtaDevice::~CPluginAtaDevice(void)
{
	if (m_smi_dev) m_smi_dev->Release();
}


bool CPluginAtaDevice::Regist(CPluginManager * manager)
{
	LOG_STACK_TRACE();
	//manager->RegistPlugin(_T("ata"), NULL, CPluginManager::CPluginInfo::PIP_SINGLETONE, CPluginAtaDevice::Create);
	return true;
}

jcscript::IPlugin * CPluginAtaDevice::Create(jcparam::IValue * param)
{
	return static_cast<jcscript::IPlugin*>(new CPluginAtaDevice(param));
}


bool CPluginAtaDevice::IdentifyDevice(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(varout == NULL);
	JCASSERT(m_smi_dev);

	stdext::auto_cif<IAtaDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_ATA_DEVICE, storage);
	if (!storage.valid())	THROW_ERROR(ERR_UNSUPPORT, _T("Device is not an ATA device"));

	bool raw_data = argu.Exist(_T("raw"));

	stdext::auto_interface<CBinaryBuffer>	data;
	CBinaryBuffer::Create(SECTOR_SIZE, data);
	WORD * buf = (WORD*)(data->Lock());

	ATA_REGISTER reg;
	memset(&reg, 0, sizeof(ATA_REGISTER));
	reg.command = 0xEC;		// ATA IDENTIFY DEVICE

	storage->AtaCommand(reg, read, false, (BYTE*)(buf), 1);
	if (reg.status & 0x1)
		THROW_ERROR(ERR_DEVICE, _T("Failure on IDENTIFY DEVICE. Error = 0x%02X"), reg.error); 

	// Decode
	UINT w7w8 = MAKELONG(buf[8], buf[7]);
	UINT chs = MAKELONG(buf[57], buf[58]);
	UINT lba = MAKELONG(buf[60], buf[61]);
	UINT lba48_lo = MAKELONG(buf[100], buf[101]);
	UINT lba48_hi = MAKELONG(buf[102], buf[103]);
	UINT64 lba48 = stdext::MAKEUINT64(lba48_lo, lba48_hi);

	if (raw_data) 
	{
		data.detach(varout);
	}
	else
	{	// Decode
		stdext::auto_interface<CAttributeTable>		table;
		CAttributeTable::Create(10, table);

		static const int MAX_STR_BUF = 64;
		TCHAR str[MAX_STR_BUF];
		
		// Serial number
		WORD * str_str = buf + 10;
		memset(str, 0, MAX_STR_BUF * sizeof(TCHAR) );
		for (int ii = 0; ii<20; ++str_str)	str[ii++] = HIBYTE(*str_str), str[ii++] = LOBYTE(*str_str);
		table->push_back( CAttributeItem(10, _T("Serial number"), 0, str) );

		// f/w version
		str_str = buf + 23;
		memset(str, 0, MAX_STR_BUF * sizeof(TCHAR) );
		for (int ii = 0; ii<8; ++str_str) str[ii++] = HIBYTE(*str_str), str[ii++] = LOBYTE(*str_str);
		table->push_back( CAttributeItem(23, _T("F/W version"), 0, str) );

		// model name
		str_str = buf + 27;
		memset(str, 0, MAX_STR_BUF * sizeof(TCHAR) );
		for (int ii = 0; ii<40; ++str_str)	str[ii++] = HIBYTE(*str_str), str[ii++] = LOBYTE(*str_str);
		table->push_back( CAttributeItem(27, _T("Model no."), 0, str) );

		// capasity
		table->push_back( CAttributeItem(60, _T("Total LBA."), lba) );
		table->push_back( CAttributeItem(100, _T("Total LBA48."), lba48_lo) );


		table.detach(varout);
	}
	data->Unlock();

	return true;
}

bool CPluginAtaDevice::SetFeature(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	return false;
}

bool CPluginAtaDevice::ReadSectors(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(m_smi_dev);
	stdext::auto_cif<IStorageDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_STORAGE_DEVICE, storage);

	IAtaDevice * ata = storage.d_cast<IAtaDevice*>();
	if (NULL == ata) THROW_ERROR(ERR_UNSUPPORT, _T("The device doesn't support ata interface"));

	FILESIZE lba = 0;
	argu.GetValT(FN_LOGIC_ADD, lba);

	JCSIZE secs = 1;
	argu.GetValT(FN_SECTORS, secs);

	JCSIZE buf_len = secs * SECTOR_SIZE;
	stdext::auto_interface<CBinaryBuffer>	data;
	CBinaryBuffer::Create(buf_len, data);

	BYTE status=0, error=0;
	if (argu.Exist(_T("pio")) )	status = ata->ReadPIO(lba, secs, error, data->Lock()); // PIO mode
	else						status = ata->ReadDMA(lba, secs, error, data->Lock()); // DMA mode
	data->Unlock();

	if ( status & 0x01)
	{
		stdext::jc_printf(_T("read error: LBA=0x%08X, status=%02X, error=%02X\n"), (UINT)lba, status, error);
		return false;
	}

	varout = static_cast<jcparam::IValue*>(data);
	varout ->AddRef();
	return true;
}
	
bool CPluginAtaDevice::WriteSectors(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(m_smi_dev);
	stdext::auto_cif<IStorageDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_STORAGE_DEVICE, storage);

	IAtaDevice * ata = storage.d_cast<IAtaDevice*>();
	if (NULL == ata) THROW_ERROR(ERR_UNSUPPORT, _T("The device doesn't support ata interface"));

	CBinaryBuffer * data = dynamic_cast<CBinaryBuffer*>(varin);
	if (!data) THROW_ERROR(ERR_PARAMETER, _T("Missing input data or wrong"));

	FILESIZE max_lba = storage->GetCapacity();
	FILESIZE lba = 0;
	if ( !argu.GetValT(FN_LOGIC_ADD, lba) )
	{
		// address is not set, read address from input data
		stdext::auto_interface<IAddress>	add;
		data->GetAddress(add);
		if ( add && (IAddress::ADD_BLOCK == add->GetType() ) )
		{
			stdext::auto_cif<jcparam::CTypedValue<FILESIZE>, jcparam::IValue>	val;
			add->GetSubValue(FN_LOGIC_ADD, val);
			lba = (FILESIZE)(*val);
		}
	}
	if (lba >= max_lba)	THROW_ERROR(ERR_PARAMETER, _T("Address out of range. Max lba = 0x%08X"), max_lba);

	JCSIZE buf_len = data->GetSize();
	JCSIZE secs = buf_len / SECTOR_SIZE;
	argu.GetValT(FN_SECTORS, secs);
	if (secs * SECTOR_SIZE > buf_len)	THROW_ERROR(ERR_PARAMETER, _T("Sectors is larger than input data"));

	BYTE status=0, error=0;
	if (argu.Exist(_T("pio")) )	status = ata->WritePIO(lba, secs, error, data->Lock()); // PIO mode
	else						status = ata->WriteDMA(lba, secs, error, data->Lock()); // DMA mode
	data->Unlock();

	if ( status & 0x01)
	{
		stdext::jc_printf(_T("write error: LBA=0x%08X, status=%02X, error=%02X\n"), (UINT)lba, status, error);
		return false;
	}

	if (argu.Exist(_T("flush")) )
	{
		status = ata->FlashCache(error);
		stdext::jc_printf(_T("flush cache error: LBA=0x%08X, status=%02X, error=%02X\n"), (UINT)lba, status, error);
	}

	stdext::jc_printf(_T("done.\n"));
	return true;
}


bool CPluginAtaDevice::AtaCommand(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(varout == NULL);
	JCASSERT(m_smi_dev);

	stdext::auto_cif<IAtaDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_ATA_DEVICE, storage);
	if (!storage.valid())	THROW_ERROR(ERR_UNSUPPORT, _T("Device is not an ATA device"));

	JCSIZE count = 0;
	argu.GetValT(_T("count"), count);

	JCSIZE secs = count;
	argu.GetValT(_T("size"), secs);


	UINT64 add = 0;
	argu.GetValT(FN_LOGIC_ADD, add);

	bool lba48 = argu.Exist(_T("ext"));
	bool brw = argu.Exist(_T("write"));

	ATA_REGISTER reg;
	memset(&reg, 0, sizeof(ATA_REGISTER) );

	//BYTE device = 0;
	argu.GetValT(_T("command"), reg.command);
	argu.GetValT(_T("feature"), reg.feature);
	argu.GetValT(_T("device"), reg.device);

	bool dma = false;
	argu.GetValT(_T("dma"), dma);

	
	READWRITE rw;
	JCSIZE data_size = 0;
	stdext::auto_interface<CBinaryBuffer> bbuf;

	if (brw)
	{
		bbuf = dynamic_cast<CBinaryBuffer *>(varin);
		if (!bbuf) THROW_ERROR(ERR_PARAMETER, _T("Missing input data"));
		bbuf->AddRef();
		data_size = bbuf->GetSize();
		JCSIZE _secs = data_size / SECTOR_SIZE;
		secs = min(secs, _secs);
		rw = write;
	}
	else
	{
		data_size = secs * SECTOR_SIZE;
		CBinaryBuffer::Create(data_size, bbuf);
		rw = read;
	}

	if ( lba48 || (add >= 0x100000000) || (count > 256) )
	{
		// Invoke a lba 48 command
		THROW_ERROR(ERR_USER, _T("Does not support LBA48 command set"));
	}
	else
	{
		// Invoke normal command
		reg.sec_count = (BYTE)count;
		reg.lba_low = BYTE(add & 0xFF);
		reg.lba_mid = BYTE( (add >> 8) & 0xFF);
		reg.lba_hi = BYTE( (add >> 16) & 0xFF);
		reg.device &= 0xF0;
		reg.device |= BYTE( (add >> 24) & 0x0F);

		_tprintf(_T("Invoke %s ATA command 0x%02X, featue = 0x%02X\n"), (brw)?_T("writing"):_T("reading"), reg.command, reg.feature);

		storage->AtaCommand(reg, rw, dma, bbuf->Lock(), secs);
		bbuf->Unlock();
	}

	if (!brw && data_size > 0)	bbuf.detach(varout);	// read copy data to varout

	_tprintf(_T("Status = 0x%02X, Error = 0x%02X\n"), reg.status, reg.error);
	
	return ( (reg.status & 0x01) == 0);

}


bool CPluginAtaDevice::Smart(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(varout == NULL);
	JCASSERT(m_smi_dev);

	bool raw_data = false, show_unknow = false;
	argu.GetValT(_T("raw"), raw_data);
	argu.GetValT(_T("unknow"), show_unknow);

	stdext::auto_cif<IAtaDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_ATA_DEVICE, storage);
	if (!storage.valid())	THROW_ERROR(ERR_UNSUPPORT, _T("Device is not an ATA device"));

	stdext::auto_interface<CBinaryBuffer>	data;
	CBinaryBuffer::Create(SECTOR_SIZE, data);

	BYTE * _data = data->Lock();
	storage->ReadSmartAttribute(_data, SECTOR_SIZE);

	if (raw_data)
	{
		data->Unlock();
		varout = static_cast<jcparam::IValue*>(data);
		varout->AddRef();
	}
	else
	{
		// Get smart attribute define table
		const CSmartAttrDefTab * smart_attr_def_tab = m_smi_dev->GetSmartAttrDefTab();
		// analyze smart attribute and set to table
		stdext::auto_interface<CSmartAttrTab> tab;
		CSmartAttrTab::Create(10, tab);

		TCHAR str_id[3];
		for ( JCSIZE ii = 2; ii < 314; ii += 12)
		{
			BYTE attr_id = _data[ii];
			if ( 0 == attr_id ) continue;

			stdext::itohex(str_id, 2, attr_id);
			LPCTSTR attr_name = NULL;

			const CSmartAttributeDefine * def = smart_attr_def_tab->GetItem(str_id);
			if ( NULL == def)
			{
				if (show_unknow) attr_name = _T("Unknow");
				else continue;
			}
			else attr_name = def->m_name;
			//attr_name = _T("Unknow");

			CSmartAttribute attr(attr_name, _data + ii);
			tab->push_back(attr);
		}
		data->Unlock();
		varout = static_cast<jcparam::IValue*>(tab);
		varout->AddRef();
	}
	return true;
}
