#include "stdafx.h"

#include "plugin_test.h"

#include "plugin_manager.h"
#include <time.h>

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <limits>

#include <vector>

LOCAL_LOGGER_ENABLE(_T("test_pi"), LOGGER_LEVEL_ERROR);
#include "application.h"

using namespace jcparam;

#define __CLASS_NAME__	CPluginTest
BEGIN_COMMAND_DEFINATION
	COMMAND_DEFINE(_T("pattern"),	MakePattern, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Make a data pattern."), 
		PARDEF(FN_SECTORS,		n, VT_UINT,		_T("Data size in sector.") )
	)

	COMMAND_DEFINE(_T("randomptn"),	RandomPattern, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Make a random pattern."), 
		PARDEF(FN_SECTORS,		n, VT_UINT,		_T("Data size in sector.") )
		PARDEF(FN_LOGIC_ADD,	a, VT_UINT64,	_T("Address in LBA.") )
	)

	COMMAND_DEFINE(_T("verifyrand"),	VerifyRandomPattern, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Verify a random pattern.")
		//PARDEF(FN_LOGIC_ADD,	a, VT_UINT64,	_T("Address in LBA.") )
	)

	COMMAND_DEFINE(_T("verify"),	Verify, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Verify data."),
		PARDEF(_T("location"),		r, VT_BOOL,		_T("Report error location.") )

	)

	COMMAND_DEFINE(_T("find"),		FindData, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Find a specified data in default input data"), 
		PARDEF(FN_SECTORS,		n, VT_UINT,		_T("Data size in sector.") )
		PARDEF(_T("break"),		b, VT_BOOL,		_T("Break if found.") )
	)



END_COMMAND_DEFINATION
#undef __CLASS_NAME__

/*

const CCmdManager CPluginTest::m_cmd_man(CCmdManager::CRule<CPluginTest>()
	(_T("endurance"), &CPluginTest::Endurance, 0, _T("Up erase count for endurance test"), 0, jcparam::CCmdLineParser::CRule()
		(PNAME_BLOCK_SIZE,	_T('b'),	jcparam::VT_UINT,	_T("Block size for one write command in sectors.") )
		(_T("inteval"),		_T('i'),	jcparam::VT_UINT,	_T("Interval between erasing block and next block.") )
	)
	(_T("sw"), &CPluginTest::SequentialWrite, 0, _T("Perform a sequential write test."), 0, jcparam::CCmdLineParser::CRule()
		(PNAME_BLOCK_SIZE,	_T('b'),	jcparam::VT_UINT,	_T("Block size for one write command in sectors (default = page_size).") )
	)
	(_T("rw"), &CPluginTest::RandomWrite, 0, _T("Perform a random write test."), 0, jcparam::CCmdLineParser::CRule()
		(PNAME_BLOCK_SIZE,	_T('b'),	jcparam::VT_UINT,	_T("Block size for one write command in sectors.") )
		(PNAME_BLOCKS,		_T('n'),	jcparam::VT_UINT64,	_T("Blocks for test") )
		(PNAME_RANGE,		0,			jcparam::VT_UINT64, _T("Access range for random test") )
	)
	(_T("cleantime"), &CPluginTest::CleanTime, 0, _T("Measure clean time.")	)

	(_T("copycompare"), &CPluginTest::CopyCompare, 0, _T("Copy compare test for LBAs."), 0, jcparam::CCmdLineParser::CRule()
		(_T("address"),		_T('a'),	jcparam::VT_UINT64,	_T("LBA address for read write.") )
		(_T("sectors"),		_T('n'),	jcparam::VT_UINT,	_T("Sectors for test. Default is whole disk.") )
		(_T("blocksize"),	_T('b'),	jcparam::VT_UINT,	_T("Block size for test, default is page size."))
		//(PNAME_RANGE,		0,			jcparam::VT_UINT64, _T("Access range for random test") )
	)

	(_T("read"), &CPluginTest::ReadTest, 0, _T("Read Test."), 0, jcparam::CCmdLineParser::CRule()
		(_T("address"),		_T('a'),	jcparam::VT_UINT64,	_T("LBA address for read write.") )
		(_T("sectors"),		_T('n'),	jcparam::VT_UINT64,	_T("Sectors for test") )
		(_T("block_size"),	_T('b'),	jcparam::VT_UINT,	_T("Block size (in sectors) for one read command.") )
		//(PNAME_RANGE,		0,			jcparam::VT_UINT64, _T("Access range for random test") )
	)

	(_T("verify"), &CPluginTest::Verify, 0, _T("Verify data."), 0, jcparam::CCmdLineParser::CRule()
		(_T("address"),		_T('a'),	jcparam::VT_UINT,	_T("LBA address for read write.") )
		(_T("blocks"),		_T('n'),	jcparam::VT_UINT,	_T("Block numbers to verify, block size = reference data size.") )
		//(_T("sectors"),		_T('n'),	jcparam::VT_UINT,	_T("Pattern size in sectors") )
	)
	
	);
*/

CPluginTest::CPluginTest(jcparam::IValue * param)
{
	CSvtApplication * app = CSvtApplication::GetApp();
	app->GetDevice(m_smi_dev);
	JCASSERT(m_smi_dev);
	m_smi_dev->GetCardInfo(m_card_info);

	// Calculate system frequency for performance test
	//LARGE_INTEGER freq;
	//QueryPerformanceFrequency(&freq);
	//m_freq = (float)freq.QuadPart;
}

CPluginTest::~CPluginTest(void)
{
	if (m_smi_dev) m_smi_dev->Release();
}

jcscript::IPlugin * CPluginTest::Create(jcparam::IValue * param)
{
	return static_cast<jcscript::IPlugin*>(new CPluginTest(param));
}

bool CPluginTest::Regist(CPluginManager * manager)
{
	LOG_STACK_TRACE();
	//manager->RegistPlugin(PLUGIN_NAME, NULL, CPluginManager::CPluginInfo::PIP_SINGLETONE, CPluginTest::Create);
	return true;
}

bool CPluginTest::MakePattern(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(NULL == varout);

	JCSIZE secs = 1;
	argu.GetValT(FN_SECTORS, secs);


	JCSIZE data_len = secs * SECTOR_SIZE;

	stdext::auto_interface<CBinaryBuffer>	data;
	CBinaryBuffer::Create(data_len, data);


	// Make increasement pattern
	BYTE * buf = data->Lock();
	for (JCSIZE ii=0; ii<data_len; ++ii) buf[ii] = (BYTE)(ii & 0xFF);
	data->Unlock();

	varout = static_cast<jcparam::IValue*>(data);
	varout->AddRef();

	return true;
}

bool CPluginTest::RandomPattern(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == varout);

	JCSIZE secs = 1;
	argu.GetValT(FN_SECTORS, secs);

	JCSIZE data_len = secs * SECTOR_SIZE;
	stdext::auto_interface<CBinaryBuffer>	data;
	CBinaryBuffer::Create(data_len, data);

	FILESIZE	lba = 0;
	bool lba_set = argu.GetValT(FN_LOGIC_ADD, lba);
	// Make radom pattern
	if (lba_set)	
	{
		srand((UINT)lba);
		data->SetBlockAddress(lba);
	}
	else	srand((unsigned)time(NULL));

	BYTE * buf = data->Lock();
	for (JCSIZE ii=0; ii<data_len; ++ii) buf[ii] = (BYTE)(rand() & 0xFF);
	LOG_DEBUG(_T("lba=%08X, d[0]=%02X, d[1]=%02X, d[2]=%02X, d[3]=%02X"), (UINT)lba, buf[0], buf[1], buf[2], buf[3]);
	data->Unlock();

	varout = static_cast<jcparam::IValue*>(data);
	varout->AddRef();

	return true;
}

bool CPluginTest::VerifyRandomPattern(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();
	JCASSERT(NULL == varout);

	CBinaryBuffer * data = dynamic_cast<CBinaryBuffer*>(varin);
	if ( NULL == data) THROW_ERROR(ERR_PARAMETER, _T("Missing default input or wrong type."));

	JCSIZE len = data->GetSize();
	//stdext::auto_array<BYTE> buf0(len);

	BYTE * buf = data->Lock();

	// get block address
	stdext::auto_interface<IAddress>	add;
	data->GetAddress(add);
	if ( !add || (IAddress::ADD_BLOCK != add->GetType() ) )	
		THROW_ERROR(ERR_PARAMETER, _T("Missing address in data."));
	stdext::auto_cif<jcparam::CTypedValue<FILESIZE>, jcparam::IValue>	val;
	add->GetSubValue(FN_LOGIC_ADD, val);
	FILESIZE lba = (FILESIZE)(*val);

	// create random pattern
	srand((UINT)lba);
	for (JCSIZE ii=0; ii<len; ++ii)
	{
		BYTE ref = (BYTE)(rand() & 0xFF);
		if (ref != buf[ii]) 
		{
			UINT ll=(UINT)lba + (ii >> 9);		// byte to sector
			stdext::jc_printf(_T("Data dismatch: lba=0x%08X, offset=0x%03X, 0x%02X <> 0x%02X. \n")
				, ll, ii & 0x1FF, ref, buf[ii]);
		}
	}
	data->Unlock();

	return true;
}

bool CPluginTest::Verify(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	CBinaryBuffer *src[2] = {NULL};	// compare src[0] vs src[1]
	JCSIZE index = 0;
	if (varin)
	{
		src[index] = dynamic_cast<CBinaryBuffer*>(varin);
		if (NULL == src[index]) THROW_ERROR(ERR_PARAMETER, _T("Wrong data type on default input."));
		index++;
	}
	
	stdext::auto_cif<CBinaryBuffer, IValue>		arg0;
	stdext::auto_cif<CBinaryBuffer, IValue>		arg1;
	if ( argu.GetCommand(0, arg0) )
	{
		if ( !arg0.valid() ) THROW_ERROR(ERR_PARAMETER, _T("Wrong data type on argument 0."));
		src[index++] = arg0.d_cast<CBinaryBuffer*>();
		if (index < 2)
		{	// argument is not enough, try to retrieve argument 1
			if ( argu.GetCommand(1, arg1) )
			{
				if ( !arg1.valid() ) THROW_ERROR(ERR_PARAMETER, _T("Wrong data type on argument 1."));
				src[index++] = arg1.d_cast<CBinaryBuffer*>();
			}
		}
	}

	if (index < 2)	THROW_ERROR(ERR_PARAMETER, _T("Missing input data"));

	JCSIZE len0 = src[0]->GetSize();
	BYTE * data0 = src[0]->Lock();
	JCSIZE len1 = src[1]->GetSize();
	BYTE * data1 = src[1]->Lock();

	if (len0 != len1)
	{
		stdext::jc_printf(_T("Lengths are different, len0 = %d, len1 = %d. \n"), len0, len1);
		return true;
	}

	bool location = argu.Exist(_T("location"));
	if (location)
	{
		// get block address
		stdext::auto_interface<IAddress>	add;
		src[0]->GetAddress(add);
		if ( !add || (IAddress::ADD_BLOCK != add->GetType() ) )	
			THROW_ERROR(ERR_PARAMETER, _T("Missing address in data."));
		stdext::auto_cif<jcparam::CTypedValue<FILESIZE>, jcparam::IValue>	val;
		add->GetSubValue(FN_LOGIC_ADD, val);
		FILESIZE lba = (FILESIZE)(*val);

		for (JCSIZE ii = 0; ii < len0; ++ii)
		{
			if (data0[ii] != data1[ii]) 
			{
				stdext::jc_printf(_T("difference: at 0x%08X : 0x%06X, 0x%02X - 0x%02X \n"), 
					(UINT)(lba), ii, data0[ii], data1[ii]);
			}
		}
	}
	else
	{
		if (memcmp(data0, data1, len0) == 0)	stdext::jc_printf(_T("Binary same. \n"));
		else									stdext::jc_printf(_T("Different. \n"));
	}


	return true;
}

bool CPluginTest::FindData(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();
	JCASSERT(m_smi_dev);

	// Check default input
	CBinaryBuffer * src = dynamic_cast<CBinaryBuffer*>(varin);
	if (NULL == src)	THROW_ERROR(ERR_PARAMETER, _T("Missing default input or wrong type."));

	static const int MAX_PATTERN = 10;
	BYTE * pattern[MAX_PATTERN] = {NULL};
	JCSIZE pattern_len[MAX_PATTERN] = {NULL};
	int patterns = 0;

	for ( ; patterns < MAX_PATTERN; ++ patterns)
	{
		// Check first input
		stdext::auto_cif<CBinaryBuffer, IValue>		ref;
		if ( !argu.GetCommand(patterns, ref) || !ref.valid() )		break;
		pattern_len[patterns] = ref->GetSize();
		pattern[patterns] = ref->Lock();
	}
	if (0 == patterns)	THROW_ERROR(ERR_PARAMETER, _T("no reference or wrong type"));

	JCSIZE len0 = src->GetSize();
	BYTE * data0 = src->Lock();

	bool is_break = argu.Exist(_T("break"));
	LOG_TRACE(_T("argument: break = %d"), is_break);

	// find
	stdext::auto_interface<jcparam::CDynaTab>	result;
	bool found = false;

	for (JCSIZE offset = 0; offset < len0; offset += SECTOR_SIZE)
	{
		JCSIZE remain = len0 - offset;
		for (int ii = 0; ii < patterns; ++ ii)
		{
			if ( remain < pattern_len[ii] ) continue;
			if (memcmp(data0 + offset, pattern[ii], pattern_len[ii]) == 0)
			{
				found = true;
				LOG_DEBUG(_T("found at 0x%X, sec 0x%X"), offset, offset / SECTOR_SIZE);
				stdext::auto_interface<IAddress>	add;
				src->GetAddress(add);
				if (add)
				{
					IAddress::ADDRESS_TYPE at = add->GetType();
					if (IAddress::ADD_FLASH == at)
					{
						if (!result)
						{
							jcparam::CDynaTab::Create(result);
							result->AddColumn(FN_BLOCK);
							result->AddColumn(FN_PAGE);
							result->AddColumn(FN_CHUNK);
							result->AddColumn(FN_SECTOR);
							result->AddColumn(_T("pattern"));
						}
						stdext::auto_interface<jcparam::CDynaRow> row;
						result->AddRow(row);

						JCSIZE off_secs = offset / SECTOR_SIZE;
						IAddress *new_add = NULL;
						add->Clone(new_add);
						new_add->Offset(m_smi_dev, off_secs);
						IValue * val = NULL;
						new_add->GetSubValue(FN_BLOCK, val);
						row->SetColumnVal(0, val);
						val->Release(); val = NULL;

						new_add->GetSubValue(FN_PAGE, val);
						row->SetColumnVal(1, val);
						val->Release(); val = NULL;

						new_add->GetSubValue(FN_CHUNK, val);
						row->SetColumnVal(2, val);
						val->Release(); val = NULL;

						val = CTypedValue<UINT>::Create(off_secs);
						row->SetColumnVal(3, val);
						val->Release(); val = NULL;
						new_add->Release();

						val = CTypedValue<int>::Create(ii);
						row->SetColumnVal(4, val);
						val->Release(); val = NULL;
					}
					else if (IAddress::ADD_BLOCK == at)
					{

					}
					else
					{

					}
				}
			}
		}
	}
	src->Unlock();

	if (result)		result.detach(varout);
	//		is_break	found	continue	return
	//		false		-		true		true
	//		-			false	true		true
	//		true		true	false		false
	return !(is_break && found);
}



bool CPluginTest::CopyCompare(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(m_smi_dev);
	stdext::auto_cif<IStorageDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_STORAGE_DEVICE, storage);
	JCASSERT(storage.valid());
	CSvtApplication * app = CSvtApplication::GetApp();
	JCASSERT(app);

	UINT64 add = 0;
	argu.GetValT(_T("address"), add);

	UINT64 secs = storage->GetCapacity();
	argu.GetValT(_T("sectors"), secs);

	UINT64 last_lba = add + secs;

	UINT block_size = PageSize();
	argu.GetValT(_T("block_size"), block_size);

	JCSIZE data_len = block_size * SECTOR_SIZE;
	stdext::auto_array<BYTE> buf0(data_len);
	stdext::auto_array<BYTE> buf1(data_len);

	memset(buf0, 0x5A, data_len);

	app->ReportProgress(NULL, -1);	// reset progress
	for (UINT64 ss = add; ss < last_lba; ss += block_size)
	{
		storage->SectorWrite(buf0, ss, block_size);

		memset(buf1, 0, data_len);
		storage->SectorRead(buf1, ss, block_size);

		if (memcmp(buf0, buf1, data_len) != 0)
		{
			stdext::jc_printf(_T("Error in LBA 0x%08X - 0x%08X\n"), (UINT)ss, (UINT)(ss + block_size -1) );
		}
		app->ReportProgress(_T("Writing LAB : "), (int)(ss * 100 / last_lba));
	}
	_tprintf(_T("copy compare test completed.\r\n"));
	return true;
}



bool CPluginTest::Performance(UINT resolution, IValue * var_in, IValue * & var_out)
{
	//JCASSERT(NULL == var_out);
	//CTimeLogArray * time_log= dynamic_cast<CTimeLogArray *>(var_in);
	//if (NULL == time_log)
	//{
	//	_tprintf(_T("Wrong type or no input value.\r\n"));
	//	return false;
	//}
	//stdext::auto_ptr<CTimeLogArray> perform(new CTimeLogArray);

	//JCSIZE size = (JCSIZE)time_log->size();
	//JCSIZE ii = 0;

	//while (ii < size)
	//{
	//	float tt = 0;
	//	TIME_LOG & tl = (*time_log)[ii];
	//	FILESIZE begin = tl.mBlock;

	//	UINT jj = 0;
	//	while ( jj < resolution )
	//	{
	//		TIME_LOG & tl = (*time_log)[ii];
	//		jj += tl.mBlockSize;
	//		tt += tl.mTime;
	//		++ii;
	//		if (ii >= size) break;
	//	}
	//	// 这里暂时利用TimeLog结构进行处理。由于在SaveToFile函数中，时间以ms单位处理（乘以1000），预先除1000。
	//	// 单位MB/s
	//	(*perform).push_back(TIME_LOG(begin, jj, (jj / tt) / 2048 / 1000) );
	//}
	//var_out = static_cast<IValue*>(perform.detatch() );
	return true;	
}

bool CPluginTest::VerifyData(ISmiDevice * smi_dev/*, IValue * & val*/)
{
	JCASSERT(smi_dev);
	//JCASSERT(time_log_arr);

	stdext::auto_cif<IStorageDevice>	storage;
	smi_dev->QueryInterface(IF_NAME_STORAGE_DEVICE, storage);
	JCASSERT(storage.valid());

	FILE * file = NULL;
	fopen_s(&file, "verify.csv", "w+");

	stdext::auto_array<BYTE> buf0(SECTOR_SIZE);
	stdext::auto_array<BYTE> buf1(SECTOR_SIZE);

	FILESIZE max_lba = storage->GetCapacity();

	// Do copy compare for random patten
	srand((unsigned)time(NULL));
	for (int ii = 0; ii < SECTOR_SIZE; ++ ii)  buf0[ii] = (BYTE)(rand() & 0xFF);

	for (FILESIZE ii = 1000; ii < 1500; ii ++)
	{
		// write
		storage->SectorWrite(buf0, ii, 1);
		// read lba
		memset(buf1, 0, SECTOR_SIZE);
		storage->SectorRead(buf1, ii, 1);

		// compare
		if (memcmp(buf1, buf0, SECTOR_SIZE) != 0 )
		{
			printf("Diff at lba, 0x%08x, offset.\r\n", ii);
			LOG_DEBUG(_T("Diff at lba, 0x%08x, offset"), ii);
		}
		
		//for (int jj = 0; jj < SECTOR_SIZE; ++jj)
		//{
		//	if (buf0[jj] != buf1[jj])
			//{
			//	fprintf(file, "Diff at lba, 0x%08x, offset, 0x%03x\n", ii, jj);
			//	fflush(file);
			//	LOG_DEBUG(_T("Diff at lba, 0x%08x, offset, 0x%03x"), ii, jj);
			//	break;
			//}
		//}
		if ( (ii & 0xFFF) == 0 ) printf("HBlock 0x%08x \r\n", ii);
	}
	fclose(file);
	printf("Done\r\n");
	return true;
}


bool CPluginTest::SequentialWrite(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	/*
	JCASSERT(m_smi_dev);
	CSvtApplication * app = CSvtApplication::GetApp();
	JCASSERT(app);

	CCardInfo card_info;
	m_smi_dev->GetCardInfo(card_info);
	UINT block_size = (UINT)(card_info.m_f_ckpp * card_info.m_f_spck);	// default 128K, 256 sectors
	argu.GetValT(PNAME_BLOCK_SIZE, block_size);
	
	stdext::auto_cif<IStorageDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_STORAGE_DEVICE, storage);
	JCASSERT(storage.valid());

	ULONGLONG max_lba = storage->GetCapacity();

	// prepqre a write buffer and fill with random value
	JCSIZE buf_size = block_size * SECTOR_SIZE;
	stdext::auto_array<BYTE> buf(buf_size);
	srand((unsigned)time(NULL));
	for (JCSIZE ii = 0; ii < buf_size; ++ ii)  buf[ii] = (BYTE)ii;

	_tprintf(_T("Sequential write test, density = %dM, block size = %.1fK\r\n"), 
		(UINT)(max_lba/2048), block_size / 2.0F);

	stdext::auto_interface<CTimeLogArray> time_log_arr(new CTimeLogArray((JCSIZE)(max_lba / block_size)));
	varout = static_cast<IValue*>(time_log_arr);
	varout->AddRef();
	
	app->ReportProgress(NULL, -1);	// reset progress
	for (ULONGLONG ss = 0; ss < max_lba; ss+=block_size)
	{
		for (JCSIZE ii = 0; ii < block_size; ++ii)
		{
			BYTE * b = buf + (ii*SECTOR_SIZE);
			UINT * d = (UINT*)b;
			*d = (UINT)(ss+ii);
		}

		float dt = WriteTime(storage, ss, block_size, buf);
		time_log_arr->push_back(TIME_LOG(ss, block_size, dt));
		app->ReportProgress(_T("Writing LAB : "), (int)(ss * 100 / max_lba));
	}
	_tprintf(_T("Sequential write test completed.\r\n"));
	*/
	return true;
}

bool CPluginTest::RandomWrite(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	/*
	JCASSERT(m_smi_dev);
	CSvtApplication * app = CSvtApplication::GetApp();
	JCASSERT(app);

	UINT block_size = 128 * 2;	// default 128K, 256 sectors
	argu.GetValT(PNAME_BLOCK_SIZE, block_size);
	FILESIZE blocks = 1024;
	argu.GetValT(PNAME_BLOCKS, blocks);
	FILESIZE range = 0;
	argu.GetValT(PNAME_RANGE, range);

	stdext::auto_cif<IStorageDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_STORAGE_DEVICE, storage);
	JCASSERT(storage.valid());

	srand((unsigned)time(NULL));

	ULONGLONG max_lba = storage->GetCapacity();

	_tprintf(_T("Random write test\r\n"));
	_tprintf(_T("Block size = %dK, Range = %dM\r\n"), block_size /2, (UINT)(blocks /2048));
	// prepqre an increasement patter
	int buf_size = block_size * SECTOR_SIZE;
	stdext::auto_array<BYTE> buf(buf_size);
	for (int ii = 0; ii < buf_size; ++ ii)  ((BYTE *)buf)[ii] = 0x55;

	_tprintf(_T("Preparing test list...\r\n"));
	// create a test sector list
	// 产生随即测试序列算法：（1）生成从0到range范围的lba地址序列；（2）序列的前部为测试序列，后部为待选序列；
	//		（3）开始时，测试序列长度为0，每次循环，测试序列长度加1；
	//		（4）从待选序列中选取一个lba地址与待选序列的第一个(ii)交换；（5）将待选序列的第一个放入测试序列的最后一个
	if ( 0 == range || range > max_lba ) range = max_lba; 
	FILESIZE total_blocks = range / block_size;
	stdext::auto_array<UINT> test_list((JCSIZE)total_blocks);
	for (UINT ii = 0; ii < total_blocks; ++ii) test_list[ii] = ii * block_size;
	if (blocks > total_blocks) blocks = total_blocks;
	for (UINT ii = 0; ii < blocks; ++ii)
	{
		UINT select = (UINT)((float)rand() * (blocks - ii) / RAND_MAX) + ii, jj;
		jj = test_list[ii], test_list[ii] = test_list[select], test_list[select] = jj;
		LOG_DEBUG(_T("Add lba 0x%08x (%d\t) to test list"), test_list[ii], test_list[ii]);
	}

	stdext::auto_interface<CTimeLogArray> time_log_arr(new CTimeLogArray(JCSIZE(blocks)));
	varout = static_cast<IValue*>(time_log_arr);
	varout->AddRef();

	app->ReportProgress(NULL, -1);	// reset progress
	for (UINT ii = 0; ii < blocks; ++ii)
	{
		float dt = WriteTime(storage, test_list[ii], block_size, buf);
		time_log_arr->push_back(TIME_LOG(test_list[ii], block_size, dt));
		app->ReportProgress(_T("Testing : "), (int)(ii * 100 / blocks));
	}
	printf("Random write test completed.\r\n");
	*/
	return true;
}

bool CPluginTest::RandomCopyCompareTest(
		ISmiDevice * smi_dev, UINT block_size, FILESIZE blocks, CJCStringT & pattern, UINT loops)
{
	JCASSERT(smi_dev);

	stdext::auto_cif<IStorageDevice> storage;
	bool br = smi_dev->QueryInterface(IF_NAME_STORAGE_DEVICE, storage);
	if (!br || !storage.valid() ) return false;

	srand((unsigned)time(NULL));

	int buf_size = block_size * SECTOR_SIZE;
	stdext::auto_array<BYTE> buf0(buf_size);		// for referenace and write
	stdext::auto_array<BYTE> buf1(buf_size);		// for read

	for (int ii = 0; ii < buf_size; ++ ii)  buf0[ii] = (BYTE)(rand() & 0xFF);


	ULONGLONG max_lba = storage->GetCapacity();

	_tprintf(_T("Random write test\r\n"));
	_tprintf(_T("Block size = %dK, Range = %dM\r\n"), block_size /2, (UINT)(blocks /2048));

	// create a test sector list
	
	int prepercent = 0;
	stdext::auto_array<FILESIZE> test_list(blocks);


	for (UINT ll = 0; ll < loops; ++ ll)
	{
		_tprintf(_T("Testing loop %d\r\n"), ll);
		_tprintf(_T("Preparing test list ...\r\n"));
		GenerateRandomList(blocks, block_size, test_list);
		// Write data
		_tprintf(_T("Writing data ...\r\n"));
		LOG_RELEASE(_T("Start test loop %d."), ll);
		for (UINT ii = 0; ii < blocks; ++ii)
		{
			storage->SectorWrite(buf0, test_list[ii], block_size);
		}
		_tprintf(_T("Reading and comparing ...\r\n"));
		LOG_RELEASE(_T("Written %d LBAs."), (UINT)(blocks * block_size) );
		for (UINT ii = 0; ii < blocks; ++ii)
		{
			storage->SectorRead(buf1, test_list[ii], block_size);
			if (memcmp(buf0, buf1, buf_size) != 0)
			{
				UINT lba = (UINT)(test_list[ii]);
				LOG_RELEASE(_T("Compare error at LBA 0x%08X - 0x%08X"),  lba, lba + block_size);
			}
		}
		LOG_RELEASE(_T("Read and compare %d LBAs."), (UINT)(blocks * block_size) );

		// SMART
/*	BOOL br = FALSE; 

	// 取得选择的当前目录
	CJCStringT cur_path;
	CMainFrame * frame = dynamic_cast<CMainFrame*>(AfxGetMainWnd());		ASSERT(frame);
	CCurrentPathBar * path_bar = frame->GetCurrentPathBar();		ASSERT(path_bar);
	path_bar->GetCurrentPath(cur_path);
	if (cur_path.IsEmpty() || (cur_path == _T("...")))
	{
		// 取得进程目录
		LPTSTR _buf = cur_path.GetBufferSetLength(_MAX_PATH);
		DWORD path_len = GetCurrentDirectory(_MAX_PATH, _buf);
		cur_path.ReleaseBufferSetLength(path_len);
	}

	ConsoleProcess * sub_proc = CreateStdPipe(m_process_codepage);
 
	// 设置PROCESS_INFORMATION结构
	PROCESS_INFORMATION piProcInfo; 
	ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

	//if (m_process_codepage == CP_UNICODE)
	//{
		// 设置STARTUPINFO结构
		STARTUPINFO siStartInfo;
		ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
		siStartInfo.cb = sizeof(STARTUPINFO); 
		siStartInfo.hStdError = sub_proc->m_child_stdout_wr;
		siStartInfo.hStdOutput = sub_proc->m_child_stdout_wr;
		siStartInfo.hStdInput = sub_proc->m_child_stdin_rd;
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
		// 创建进程
		br = CreateProcess(
			NULL, cmd_line,     // 命令行 
			NULL, NULL,         // 进程和主线程的安全属性
			TRUE,				// 继承句柄 
			//CREATE_NO_WINDOW,   // 标志:创建没有窗口的CUI 
			CREATE_SUSPENDED | CREATE_NO_WINDOW,   // 标志:创建没有窗口的CUI 
			NULL, cur_path,         // 环境变量和当前目录 
			&siStartInfo,		// STARTUPINFO pointer 
			&piProcInfo);		// receives PROCESS_INFORMATION 


	if ( !br)
	{
		DWORD errid = GetLastError();
		delete sub_proc;
		SYSTEM_ERROR1(errid, _T("create process"));
	}
	//CloseHandle(piProcInfo.hThread);
	sub_proc->m_sub_process = piProcInfo.hProcess;
*/

	}
	printf("Random copy compare completed.\r\n");
	return true;
}

bool CPluginTest::FileRepair(ISmiDevice * smi_dev)
{
	JCASSERT(smi_dev);

	stdext::auto_cif<IStorageDevice> storage;
	bool br = smi_dev->QueryInterface(IF_NAME_STORAGE_DEVICE, storage);
	if (! storage.valid()) return false;

	ULONGLONG max_lba = storage->GetCapacity();

	// buffer size = 4K, 8 sectors
	static const JCSIZE CLUSTER_SIZE = 8;
	static const JCSIZE CLUSTER_LEN = CLUSTER_SIZE * SECTOR_SIZE;

	typedef std::pair<CJCStringA, BYTE * >	Cluster;

	BYTE * buf = NULL;

	//static const boost::regex CRule("SMI[^\\s]*\\s*(\\d{4}/\\d{2}/\\d{2} \\d{2}:\\d{2}:\\d+)");
	static const boost::regex rule("(\\d{4}/\\d{2}/\\d{2} \\d{2}:\\d{2}:\\d+)");

/*
	buf = new BYTE[1024 * 1024];

	FILE * testfile = NULL;
	fopen_s(&testfile, "smartlog.txt", "r");
	JCSIZE size = fread(buf, 1, 1024*1024, testfile);

	fclose(testfile);

	boost::cmatch match;
	if (boost::regex_search((char*)buf, match, rule) )
	{
		LOG_DEBUG(_T("matched %d"), match.size());
		LOG_DEBUG(_T("$1 = %S"), match[1].str().c_str());
	}

	delete [] buf;

	return true;
*/

	std::vector<Cluster>	cluster_list;
	//FILE * file_log = NULL;
	//fopen_s(&file_log, "dump_smart.log", "w+");

	int old_percent = 0;
	int	old_block_found = 0, cur_block_found = 0;

	_tprintf(_T("Reading data..."));
	for (ULONGLONG ss = 0; ss < max_lba; ss += 8)
	{
		if (NULL == buf)
		{
			 buf = new BYTE[CLUSTER_LEN + 32];
			 memset(buf, 0, CLUSTER_LEN + 32);
		}

		storage->SectorRead(buf, ss, CLUSTER_SIZE);
		
		// recognize
		boost::cmatch	match;
		if ( boost::regex_search((const char*)buf, (const char *)buf + CLUSTER_LEN, match, rule) 
			&& (match.size() > 0) )
		{
			// if recoginzed, insert to list by order
			cluster_list.push_back(Cluster(match[1].str(), buf) );
			LOG_DEBUG(_T("Data found in LBA 0x%08X, time stamp %S"), (DWORD)(ss), match[1].str().c_str());
			buf = NULL;
			cur_block_found ++;
			_tprintf(_T("\rReading data %d%%, block found %d..."), old_percent, cur_block_found);
		}
		int cur_percent = (int)(ss * 100 / max_lba);
		if ( cur_percent > old_percent )
		{
			old_percent = cur_percent;
			_tprintf(_T("\rReading data %d%%, block found %d..."), old_percent, cur_block_found);
		}
	}
	_tprintf(_T("\r\nSorting...\r\n"));
	//fclose(file_log);

	std::sort(cluster_list.begin(), cluster_list.end());


	_tprintf(_T("Data saveing...\r\n"));

	FILE * file = NULL;
	fopen_s(&file, "smart.txt", "w+b");
	
	std::vector<Cluster>::iterator	it;
	std::vector<Cluster>::iterator	endit = cluster_list.end();
	for (it = cluster_list.begin(); it != endit; ++ it)
	{
		fwrite(it->second, CLUSTER_LEN, 1, file);
		delete [] it->second;
	}
	fclose(file);

	_tprintf(_T("Done.\r\n"));

	return true;
}


void CPluginTest::GenerateRandomList(FILESIZE blocks, FILESIZE alignment, FILESIZE* list)
{
	JCASSERT(list);
	// 产生随即测试序列算法：（1）生成从0到range范围的lba地址序列；（2）序列的前部为测试序列，后部为待选序列；
	//		（3）开始时，测试序列长度为0，每次循环，测试序列长度加1；
	//		（4）从待选序列中选取一个lba地址与待选序列的第一个(ii)交换；（5）将待选序列的第一个放入测试序列的最后一个
	for (UINT ii = 0; ii < blocks; ++ii) list[ii] = ii * alignment;
	for (UINT ii = 0; ii < blocks; ++ii)
	{
		UINT select = (UINT)((float)rand() * (blocks - ii) / RAND_MAX) + ii;
		FILESIZE jj;
		jj = list[ii], list[ii] = list[select], list[select] = jj;
		LOG_DEBUG(_T("Add lba 0x%08x (%d\t) to test list"), list[ii], list[ii]);
	}
}


bool CPluginTest::Endurance(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	CSvtApplication * app = CSvtApplication::GetApp();
	stdext::auto_interface<ISmiDevice> smi_dev;
	app->GetDevice(smi_dev);
	JCASSERT(smi_dev);

	stdext::auto_cif<IAtaDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_ATA_DEVICE, storage);
	if (!storage.valid())	THROW_ERROR(ERR_UNSUPPORT, _T("Device is not an ATA device"));

	FILESIZE max_lba = storage->GetCapacity();

	UINT block_size = 128 * 2;	// default 128K, 256 sectors
	argu.GetValT(PNAME_BLOCK_SIZE, block_size);

	UINT interval = 1;
	argu.GetValT(_T("inteval"), interval);

	UINT log = 100;				// interval of log
	argu.GetValT(_T("log"), log);

	// prepare buffer for write and read
	JCSIZE buf_size = block_size * SECTOR_SIZE;
	stdext::auto_array<BYTE> buf_w(buf_size);
	stdext::auto_array<BYTE> buf_r(buf_size);

	stdext::auto_array<BYTE> smart_buf(SECTOR_SIZE);

	stdext::auto_array<TCHAR> log_buf(1024);

	UINT verify_failed = 0;
	UINT64	total_erase_count = 0;

	// Init time value for interval
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	double dfreq = (double)freq.QuadPart;

	LARGE_INTEGER last_time_stamp;
	QueryPerformanceCounter(&last_time_stamp);


	srand((unsigned)time(NULL));
	while (1)
	{
		for (UINT jj=0; jj<log; ++jj)
		{
			// Generate test pattern
			for (int ii = 0; ii < buf_size; ++ ii)  buf_w[ii] = (BYTE)(rand() & 0xFF);

			// Random write
			DWORD ran32 = (rand() << 15) | rand();
			FILESIZE lba = (ran32 * max_lba) >> 30;
			storage->SectorWrite(buf_w, lba, block_size);

			// Read and verify
			memset((BYTE*)buf_r, buf_size, 0);
			storage->SectorRead(buf_r, lba, block_size);
			if ( memcmp(buf_r, buf_w, buf_size) != 0)
			{
				++ verify_failed;
				LOG_ERROR(_T("Verify failed at LBA 0x%08X, total failed %d"), (DWORD)lba, verify_failed);
			}
		}

		// Get SMART
		enum	SMART_ID
		{
			RUNTIME_BAD_BLOCK = 0x05,
			CRC_ERROR = 0xC7,
			MAX_ERASE_COUNT = 0xF1,
			TOTAL_ERASE_COUNT = 0xF8,
			MIN_ERASE_COUNT = 0xFC,
			AVG_ERASE_COUNT = 0xFD,
		};
		storage->ReadSmartAttribute(smart_buf, SECTOR_SIZE);
		JCSIZE ii = 2;

		WORD bad_block, crc_error;
		UINT max_erase_count, min_erase_count, avg_erase_count;
		UINT64 cur_total_erase;

		while ( ii < 362)
		{
			BYTE smart_id = smart_buf[ii];
			BYTE * raw = smart_buf + ii + 5;
			switch (smart_id)
			{
			case RUNTIME_BAD_BLOCK:
				bad_block = GetRawValue<WORD, 2>(raw);
				break;

			case CRC_ERROR:
				crc_error = GetRawValue<WORD, 2>(raw);
				break;

			case MAX_ERASE_COUNT:
				max_erase_count = GetRawValue<UINT, 4>(raw);
				break;

			case TOTAL_ERASE_COUNT:
				cur_total_erase = GetRawValue<UINT64, 6>(raw);
				break;

			case MIN_ERASE_COUNT:
				min_erase_count = GetRawValue<UINT, 4>(raw);
				break;

			case AVG_ERASE_COUNT:
				avg_erase_count = GetRawValue<UINT, 4>(raw);
				break;
			}
			ii += 12;
		}

		// LOG
		_stprintf_s(log_buf, 1024, _T("LOG %d blocks written, error %d blocks, total erase count %d, "));


		// Adjust time
		LARGE_INTEGER current_time_stamp;
		QueryPerformanceCounter(&current_time_stamp);



		total_erase_count = cur_total_erase;
	}
}

#ifdef max
#define MAX_PUSHED
#pragma push_macro("max")
#undef max
#endif

class CCleanTime
{
public:
	CCleanTime() 
		: m_hblock(0xFFFF), m_first_page(0), m_last_page(0), m_max(0)
		, m_min( std::numeric_limits<float>::max() ), m_total(0), m_pages(0) {};
	WORD	m_hblock;
	BYTE	m_pages;	// page number for total value
	float	m_first_page;
	float	m_last_page;
	float	m_max;
	float	m_min;
	float	m_total;
};

#ifdef MAX_PUSHED
#pragma pop_macro("max")
#undef MAX_PUSHED
#endif

bool CPluginTest::CleanTime(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	/*
	JCASSERT(m_smi_dev);

	CTimeLogArray * time_log_arr = dynamic_cast<CTimeLogArray *>(varin);
	if (NULL == time_log_arr) THROW_ERROR(ERR_PARAMETER, _T("input variable type mismatched") );

	CCardInfo card_info;
	m_smi_dev->GetCardInfo(card_info);
	JCSIZE pages = card_info.m_f_ppb;
	JCSIZE page_size = card_info.m_f_ckpp * card_info.m_f_spck;
	JCSIZE lba_per_block = page_size * pages;

	stdext::auto_cif<IStorageDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_STORAGE_DEVICE, storage);
	ULONGLONG max_lba = storage->GetCapacity();
	JCSIZE max_h_block = (JCSIZE)( (max_lba - 1) / lba_per_block)+1;

	JCSIZE time_length = time_log_arr->size();

	_tprintf(_T("Clean time test, density = %dM, hblock = %d, page size = %.1fK, %d pages per block.\n"), 
		(UINT)(max_lba/2048), max_h_block, page_size / 2.0, pages);

	typedef std::vector<CCleanTime> CCleanTimeVector;
	CCleanTimeVector	clean_vector(max_h_block+1);

	for ( JCSIZE ii = 0; ii < time_length; ++ii)
	{
		TIME_LOG & time_log = (*time_log_arr)[ii];
		ULONGLONG lba = time_log.mBlock;

		WORD hblock = (WORD)(lba / lba_per_block);
		JCASSERT(hblock <= max_h_block);
		CCleanTime & clean_time = clean_vector[hblock];
		WORD page = (WORD)((lba % lba_per_block) / page_size);

		if (0 == page)					clean_time.m_first_page = time_log.mTime;
		else if ((pages-1) == page)		clean_time.m_last_page = time_log.mTime;
		else
		{
			clean_time.m_total += time_log.mTime;
			clean_time.m_pages ++;
			if (clean_time.m_max < time_log.mTime) clean_time.m_max = time_log.mTime;
			if (clean_time.m_min > time_log.mTime) clean_time.m_min = time_log.mTime;
		}
	}

	FILE * file = NULL;
	_tfopen_s(&file, _T("cleantime.csv"), _T("w+"));
	_ftprintf(file, _T("h_block, strat_lba, first_page, max, min, avg, last_page\n")); 
	for ( JCSIZE hh = 0; hh < max_h_block; ++hh)
	{
		CCleanTime & clean_time = clean_vector[hh];
		_ftprintf(file, _T("0x%04X, 0x%08X, %.1f, %.1f, %.1f, %.1f, %.1f\n"), 
			hh, (UINT)(hh*lba_per_block), clean_time.m_first_page * 1000, 
			clean_time.m_max * 1000, clean_time.m_min * 1000, 
			clean_time.m_total / (clean_time.m_pages) * 1000, clean_time.m_last_page * 1000);

		// Output progress
		//app->ReportProgress(_T("Writing LAB : "),  (int)(hh * 100 / max_h_block));
	}
	fclose(file);
*/
	return true;
}
/*
{
	JCASSERT(m_smi_dev);
	CSvtApplication * app = CSvtApplication::GetApp();
	JCASSERT(app);

	stdext::auto_cif<IStorageDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_STORAGE_DEVICE, storage);
	JCASSERT(storage.valid());

	CCardInfo card_info;
	m_smi_dev->GetCardInfo(card_info);

	WORD max_f_block = card_info.m_f_block_num;
	JCSIZE pages = card_info.m_f_page_num;
	JCSIZE page_size = card_info.m_f_page_size;

	ULONGLONG max_lba = storage->GetCapacity();
	JCSIZE lba_per_block = page_size * pages;
	JCSIZE max_h_block = (JCSIZE)(max_lba / lba_per_block);
	
	JCSIZE buf_size = page_size * SECTOR_SIZE;
	stdext::auto_array<BYTE> buf(buf_size);
	
	// create a pattern
	memset((BYTE*)buf, 0xaa, buf_size);

	_tprintf(_T("Clean time test, density = %dM, hblock = %d, page size = %.1fK, %d pages per block.\n"), 
		(UINT)(max_lba/2048), max_h_block, page_size / 2.0, pages);

	FILE * file = NULL;
	_tfopen_s(&file, _T("cleantime.csv"), _T("w+"));

	app->ReportProgress(NULL, -1);	// reset progress

	_ftprintf(file, _T("h_block, strat_lba, first_page, max, min, avg, last_page\n")); 
	for ( JCSIZE hh = 0; hh < max_h_block; ++hh)
	{
		FILESIZE lba = hh * lba_per_block;
		float t = 0, tmax = 0, tmin = 100, ttotal = 0;

		// first page
		float tfirst = WriteTime(storage, lba, page_size, buf) * 1000;
		//_ftprintf(file, _T("%f, "), t * 1000);		// in ms

		// mid page: max min avg
		for ( JCSIZE pp = 0; pp < (pages-1) ; ++pp, lba += page_size)
		{
			t = WriteTime(storage, lba, page_size, buf);
			ttotal += t;
			if (tmax < t) tmax = t;
			if (tmin > t) tmin = t;
		}

		// last page
		float tlast = WriteTime(storage, lba, page_size, buf) * 1000;

		_ftprintf(file, _T("0x%04X, 0x%08X, %.1f, %.1f, %.1f, %.1f, %.1f, 0x%08X\n"), hh, (UINT)(hh*lba_per_block), tfirst, 
			tmax * 1000, tmin* 1000, ttotal / (pages-2) * 1000, tlast, (UINT)(lba));

		// Output progress
		app->ReportProgress(_T("Writing LAB : "),  (int)(hh * 100 / max_h_block));
	}
	fclose(file);
	_tprintf(_T("\nDone.\n"));
	return true;
}
*/


//
//bool CPluginTest::ReadTest(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
//{
//	JCASSERT(m_smi_dev);
//	CSvtApplication * app = CSvtApplication::GetApp();
//	JCASSERT(app);
//
//	UINT block_size = PageSize();
//	argu.GetValT(PNAME_BLOCK_SIZE, block_size);
//	
//	stdext::auto_cif<IAtaDevice>	storage;
//	m_smi_dev->QueryInterface(IF_NAME_ATA_DEVICE, storage);
//	if (!storage.valid())	THROW_ERROR(ERR_UNSUPPORT, _T("Device is not an ATA device"));
//
//	UINT64 add = 0;
//	argu.GetValT(_T("address"), add);
//
//	ULONGLONG max_lba = storage->GetCapacity();
//	JCSIZE secs = (JCSIZE)(max_lba - add);
//	argu.GetValT(_T("sectors"), secs);
//
//	UINT64 last_lba = add + secs;
//	// prepqre a write buffer and fill with random value
//	JCSIZE buf_size = block_size * SECTOR_SIZE;
//	stdext::auto_array<BYTE> buf(buf_size);
//
//	_tprintf(_T("Sequential read test, density = %dM, block size = %.1fK\r\n"), 
//		(UINT)(max_lba/2048), block_size / 2.0F);
//	
//	app->ReportProgress(NULL, -1);	// reset progress
//	for (ULONGLONG ss = add; ss < last_lba; ss += block_size)
//	{
//		UINT err = storage->ReadDMA(ss, block_size, buf);
//		if (0 != err)  stdext::jc_printf(_T("Error on reading 0x%08X, err=0x%02X"), (UINT)ss, err);
//		app->ReportProgress(_T("Writing LAB : "), (int)(ss * 100 / max_lba));
//	}
//	_tprintf(_T("Sequential write test completed.\r\n"));
//	return true;
//}
//
//
