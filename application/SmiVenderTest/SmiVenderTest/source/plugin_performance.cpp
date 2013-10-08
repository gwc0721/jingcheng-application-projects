#include "stdafx.h"

#include "plugin_manager.h"
#include "plugin_performance.h"
#include <time.h>

LOCAL_LOGGER_ENABLE(_T("perf_pi"), LOGGER_LEVEL_ERROR);
#include "application.h"

LPCTSTR CPluginPerformance::PLUGIN_NAME = _T("perform");

#define __CLASS_NAME__	CPluginPerformance
BEGIN_COMMAND_DEFINATION

	COMMAND_DEFINE(_T("mklist"),	MakeTestList, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Make a LBA list for test."),
		PARDEF(_T("test"),		t, VT_USHORT,	_T("test count.") )
		PARDEF(_T("range"),		r, VT_FLOAT,	_T("test range in percent.") )

		//PARDEF(_T("test"),		t, VT_USHORT,	_T("test count.") )
	)

	COMMAND_DEFINE(_T("rw"),	RandomWrite, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Random write busy time test."),
		PARDEF(_T("test"),		t, VT_USHORT,	_T("test count.") )
		PARDEF(_T("range"),		r, VT_FLOAT,	_T("test range in percent.") )
		PARDEF(_T("filename"),	f, VT_FLOAT,	_T("file to save result.") )

//		PARDEF(_T("page"),		p, VT_USHORT,	_T("page size.") )
//		PARDEF(_T("test"),		t, VT_USHORT,	_T("test count.") )
	)


END_COMMAND_DEFINATION
#undef __CLASS_NAME__

CPluginPerformance::CPluginPerformance(void)
{
	CSvtApplication * app = CSvtApplication::GetApp();
	app->GetDevice(m_smi_dev);
	JCASSERT(m_smi_dev);
	m_smi_dev->GetCardInfo(m_card_info);

	// Calculate system frequency for performance test
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	m_freq = (float)freq.QuadPart;
}

CPluginPerformance::~CPluginPerformance(void)
{
	//ClearCID();
	if (m_smi_dev) m_smi_dev->Release();
}

bool CPluginPerformance::Regist(CPluginManager * manager)
{
	LOG_STACK_TRACE();
	//manager->RegistPlugin(
	//	PLUGIN_NAME, NULL, CPluginManager::CPluginInfo::PIP_SINGLETONE, 
	//	CPluginPerformance::Create);
	return true;
}

jcscript::IPlugin * CPluginPerformance::Create(jcparam::IValue * param)
{
	return static_cast<jcscript::IPlugin*>(new CPluginPerformance());
}

bool CPluginPerformance::MakeTestList(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	//LOG_STACK_TRACE();

	//stdext::auto_cif<IStorageDevice>	storage;
	//m_dev->QueryInterface(IF_NAME_STORAGE_DEVICE, storage);

	//CJCStringT type;	// pattern type
	//argu.GetCommand(0, type);

	//JCSIZE	test_count = 0;
	//argu.GetValT(_T("test"), test_count);

	//float range_percent = 0;
	//argu.GetValT(_T("range"), range_percent);
	//
	//FILESIZE max_lba = m_smi_dev->

	return false;
}

float CPluginPerformance::WriteTime(IStorageDevice * storage, FILESIZE start_lba, JCSIZE sectors, BYTE * buf)
{
	JCASSERT(storage);
	LOG_STACK_TRACE();
	LARGE_INTEGER t0, t1;	// Time from system performance
	QueryPerformanceCounter(&t0);

	storage->SectorWrite(buf, start_lba, sectors);

	QueryPerformanceCounter(&t1);
	float dt = (float)(t1.QuadPart - t0.QuadPart) / m_freq;
	return dt;
}

bool CPluginPerformance::RandomWrite(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();

	// 考虑到长时间测试导致的结果过于庞大，采用单一函数直接写结果的文件的方法。
	stdext::auto_cif<IStorageDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_STORAGE_DEVICE, storage);
	FILESIZE total_lba = storage->GetCapacity();

	CJCStringT type;	// pattern type
	argu.GetCommand(0, type);
	if (type.empty() ) THROW_ERROR(ERR_PARAMETER, _T("Missing test type."));

	CJCStringT file_name(_T("busytime.csv"));
	argu.GetValT(_T("filename"), file_name);

	JCSIZE	test_count = 0;
	argu.GetValT(_T("test"), test_count);

	float range_percent = 100;
	argu.GetValT(_T("range"), range_percent);
	FILESIZE test_max_lba = (FILESIZE)(total_lba * (range_percent / 100));
	if (test_max_lba > total_lba) test_max_lba = total_lba;

	JCSIZE max_block = BlockNum();
	JCSIZE page_num = PageNum();
	JCSIZE page_size = PageSize();
	JCSIZE block_size = page_num * page_size;

	JCSIZE block_range = (JCSIZE)((test_max_lba -1) / block_size ) +1;		// max block id
	JCSIZE page_range = (JCSIZE)( (test_max_lba -1) / page_size ) +1;		// max page id
	LOG_NOTICE(_T("type=%s"), type.c_str());
	LOG_NOTICE(_T("total_block=%d, range_block=%d"), max_block, block_range);
	LOG_NOTICE(_T("total_page=%d, page_range=%d"), max_block * page_num, page_range);
	LOG_NOTICE(_T("page_num=%d, page_size=%d"), page_num, page_size);

	// prepare test data
	JCSIZE buf_size = page_size * SECTOR_SIZE;
	stdext::auto_array<BYTE> buf(buf_size);
	for (JCSIZE ii = 0; ii < buf_size; ++ii)	buf[ii] = (BYTE)(ii & 0xFF);

	// open file
	FILE * file = NULL;
	stdext::jc_fopen(&file, file_name.c_str(), _T("w+"));
	if (NULL == file)	THROW_ERROR(ERR_PARAMETER, _T("cannot open file %s"), file_name.c_str());

	// output buffer
	typedef char OUT_CHAR;
	static const JCSIZE out_len = 8*1024;
	static const JCSIZE out_margin = 64;
	stdext::auto_array<OUT_CHAR> out(out_len + out_margin);
	JCSIZE out_remain = out_len + out_margin;
	OUT_CHAR * out_ptr = out;

	// output header
	JCSIZE ir = sprintf_s(out_ptr, out_remain, ("address, time\n"));		// to ms
	out_ptr += ir;
	out_remain -= ir;

	stdext::jc_printf(_T("testing..."));
	JCSIZE total_write_count = test_count * block_range;
	JCSIZE written_count = 0;
	srand((UINT) time(NULL));
	JCSIZE pre_progress = 0;
	JCSIZE pp = page_num - 1;		// page count for ORDER test
	FILESIZE max_start_lba = total_lba - page_size;
	for (JCSIZE tt = 0; tt < test_count; ++tt, --pp)
	{
		//if (pp >= page_num) pp = 0;
		LOG_NOTICE(_T("cycle = %d / %d"), tt, test_count);
		for (JCSIZE jj = 0; jj < block_range; ++jj)
		{
			FILESIZE lba = 0;

			if ( type == _T("RAND_BK") )
			{
				JCSIZE block = rand() * block_range / (RAND_MAX+1);
				JCSIZE page = rand() * (page_num-1) / (RAND_MAX+1);
				lba = block * block_size + page * page_size;
				if (lba >= total_lba) LOG_ERROR(_T("LBA overflow: block = %d, page = %d, lba=%d"), block, page, lba);
			}
			else if (type == _T("RAND_PG"))
			{
				FILESIZE rnd = ((UINT)(rand()) << 15) | (UINT)(rand());
				JCSIZE page = (JCSIZE) (rnd * (page_range) / 0x40000000);
				lba = page * page_size;
				if (lba >= total_lba) LOG_ERROR(_T("LBA overflow: page = %d, lba=%d"), page, lba);
			}
			else if (type == _T("ORDER") )
			{
				lba = jj * block_size + pp * page_size;
				if (0 == jj) LOG_NOTICE(_T("page = %d"), pp);
				if (lba >= total_lba) LOG_ERROR(_T("block = %d, page = %d, lba=%d"), jj, pp, lba);
			}

			if ( lba > max_start_lba ) continue;
			float write_time = WriteTime(storage.d_cast<IStorageDevice*>(), lba, page_size, buf);

//				JCSIZE ir = stdext::jc_sprintf(out_ptr, out_remain, _T("0x%08X, %.2f\n"), (UINT)(lba & 0xFFFFFFFF), write_time * 1000);		// to ms
			ir = sprintf_s(out_ptr, out_remain, ("0x%08X, %.2f\n"), (UINT)(lba & 0xFFFFFFFF), write_time * 1000);		// to ms
			out_ptr += ir;
			out_remain -= ir;
			if (out_remain < out_margin)
			{	
				LOG_TRACE(_T("Save buffer to file"));
				// save to file
				fwrite((OUT_CHAR*)(out), out_len * sizeof(OUT_CHAR), 1, file);
				// copy margin to start
				memcpy_s((OUT_CHAR*)(out), out_len * sizeof(OUT_CHAR), (OUT_CHAR*)(out + out_len), out_margin * sizeof(OUT_CHAR) );
				// re-calulate point
				out_ptr -= out_len;
				out_remain += out_len;
			}
			written_count += 100;
			JCSIZE progress = written_count / total_write_count;
			LOG_DEBUG(_T("cycle = %d / %d, range = %d / %d, percent = %d"), tt, test_count, 
				jj, block_range, progress);

			if (progress > pre_progress)
			{
				stdext::jc_printf(_T("\rtesting... %d%%"), progress);
				pre_progress = progress;
			}

			if (pp == 0) pp = page_num;
		}
	}

	// save remain to file
#ifdef _DEBUG
	ir = sprintf_s(out_ptr, out_remain, ("finished\n"));		// to ms
	out_ptr += ir;
	out_remain -= ir;
#endif
	fwrite((OUT_CHAR*)(out), (out_ptr - out) * sizeof(OUT_CHAR), 1, file);
	
	stdext::jc_printf(_T("\rtest completed.\n"));
	fclose(file);
	return true;
}


