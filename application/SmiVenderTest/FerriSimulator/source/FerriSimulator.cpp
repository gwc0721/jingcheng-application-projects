// FerriSimulation.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <vld.h>
#include <stdext.h>
#include <jcparam.h>

#include "csv_reader.h"
#include "sim_cache_mode.h"

LOCAL_LOGGER_ENABLE(_T("FerriSimulation"), LOGGER_LEVEL_DEBUGINFO);

const jcparam::CParameterDefinition line_parser( jcparam::CParameterDefinition::RULE()
	(_T("random"), _T('w'), jcparam::VT_BOOL, _T("Perform a random write simulation.") )
	(_T("range"), _T('r'), jcparam::VT_UINT, _T("Access range for random test. MB") )
	(_T("count"), _T('c'), jcparam::VT_UINT, _T("Command count.") )
	(_T("input"), _T('i'), jcparam::VT_STRING, _T("Input file for test.") )
  );

#if 1
LOGGER_TO_DEBUG(0, 0
	| CJCLogger::COL_FUNCTION_NAME 
	| CJCLogger::COL_TIME_STAMP, 0);

#else

LOGGER_TO_FILE(O, _T("FerriSimulation.log"), 
	//CJCLogger::COL_COMPNENT_NAME | 
	CJCLogger::COL_FUNCTION_NAME | 
	CJCLogger::COL_TIME_STAMP, 0);

#endif

//CSmiLT2244 * g_ferri = NULL;

void ProcessOneLine()
{

}

void RandomSimulate(jcparam::CArguSet & arg_set, CSimCacheMode & ferri)
{
	LOG_STACK_TRACE();

	// Perform a random test
	JCSIZE range = 2048;		// 2GB
	arg_set.GetValT(_T("range"), range);
	range *= 2048;		// MB to sectors

	JCSIZE cmd_count = 1;
	arg_set.GetValT(_T("count"), cmd_count); 

	FILE * dst_file = NULL;
	_tfopen_s(&dst_file, _T("random.txt"), _T("w+"));
	if (NULL == dst_file) THROW_ERROR(ERR_APP, _T("Open file %s failed"), _T("random.txt"));

	int pre_progress = 0;
	_tprintf(_T("running random simulation...\n"));

	srand(0);
	for (JCSIZE ii = 0; ii < cmd_count; ++ii)
	{
		JCSIZE r0 = rand(), r1 = rand();
		//JCSIZE rr = rand(), rand());
		FILESIZE lba = (r0<<7 | r1) % range;

		UINT time_ba = 0;
		ferri.SectorWrite(NULL, lba, 1);
		time_ba = ferri.GetLastInvokeTime();

		// cmd, lba, sectors, time_ac, time_ba, pair number 
		stdext::jc_fprintf(dst_file, _T("0, 0x%08X, 1, 0.0, %.2f, %d\n"),
			(UINT)lba, (float)(time_ba) / 1000.0, 
			ferri.GetCacheBlockCnt() );


		if ( 0 == (ii & 0xFF) )
		{
			fflush(dst_file);
			int progress = (ii*100 / cmd_count);
			if (progress > pre_progress)
			{
				pre_progress = progress;
				_tprintf(_T("\rprocessing ... %d %%"), progress);
			}
		}
	}
	_tprintf(_T("\ndone.\n"));

	fclose(dst_file);


}

void FileSimulate(jcparam::CArguSet & arg_set, CSimCacheMode & ferri)
{
	LOG_STACK_TRACE();
	
	CJCStringT str_src_fn;
	arg_set.GetValT(_T("input"), str_src_fn);

	FILE * src_file = NULL;
	FILE * dst_file = NULL;
	
	if (str_src_fn.empty())	THROW_ERROR(ERR_PARAMETER, _T("missing file name"));

	_tfopen_s(&src_file, str_src_fn.c_str(), _T("r") );
	if (NULL == src_file) THROW_ERROR(ERR_APP, _T("Open file %s failed"), str_src_fn.c_str());
	
	CJCStringT str_dst_fn = str_src_fn + _T(".out");
	_tfopen_s(&dst_file, str_dst_fn.c_str(), _T("w+"));
	if (NULL == dst_file) THROW_ERROR(ERR_APP, _T("Open file %s failed"), str_dst_fn.c_str());


	//CSimCacheMode	ferri;

	static const JCSIZE MAX_STR_BUF=1024;
	TCHAR line_buf[MAX_STR_BUF];
	// skip the first line
	_fgetts(line_buf, MAX_STR_BUF, src_file);


	_tprintf(_T("running trace simulation...\n"));
	JCSIZE ii = 0;
	while (1)
	{
		if ( _fgetts(line_buf, MAX_STR_BUF, src_file) == NULL) break;
		LPCTSTR first = line_buf, last = line_buf + _tcslen(line_buf);

		BYTE	cmd_id;
		JCSIZE	lba;
		JCSIZE	sectors;
		float	time_ac;	// busy time for ac in ms

		CTokenProperty prop;
		for ( int index = 0; index <= 7; ++index) 
		{
			memset(&prop, 0, sizeof(prop));

			LineParse(first, last, &prop);

			switch (index)
			{
			case 2:		// busy time
				JCASSERT(prop.m_id == ID_FLOAT);
				time_ac = (float)prop.m_double;
				break;

			case 3:		// command id
				JCASSERT(prop.m_id == ID_HEX);
				cmd_id = (BYTE)(prop.m_int & 0xFF);
				break;

			case 4:		// lab
				JCASSERT(prop.m_id == ID_HEX);
				lba = (JCSIZE) prop.m_int;
				break;

			case 5:		// sectors
				JCASSERT(prop.m_id == ID_DECIMAL);
				sectors = (JCSIZE) prop.m_int;
				break;
			}
		}

		UINT time_ba = 0;
		if ( (cmd_id = 0xCA) && (sectors > 0) )
		{
			ferri.SectorWrite(NULL, lba, sectors);
			time_ba = ferri.GetLastInvokeTime();
		}

		// cmd, lba, sectors, time_ac, perform_ac, time_ba, perform_ba, pair number 
		float f_time_ba = (float)(time_ba) / 1000.0;
		stdext::jc_fprintf(dst_file, _T("0x%02X, 0x%08X, %d, %.2f, %.4f, %.2f, %.4f, %d\n"),
			cmd_id, lba, sectors, time_ac, sectors * 0.5 / time_ac, f_time_ba, sectors * 0.5 / f_time_ba, 
			SPARE_CNT- ferri.GetCacheBlockCnt() );

		ii ++;
		if ( 0 == (ii & 0xFF) )
		{
			_tprintf(_T("\rprocessing %d lines"), ii);
		}
	}
	_tprintf(_T("\ndone.\n"));

	fclose(src_file);
	fclose(dst_file);
}

int _tmain(int argc, _TCHAR* argv[])
{
	LOG_STACK_TRACE();

	FILE * config_file = NULL;
	_tfopen_s(&config_file, _T("jclog.cfg"), _T("r"));
	if (config_file)
	{
		CJCLogger::Instance()->Configurate(config_file);
		fclose(config_file);
	}	

	CSimCacheMode	ferri;
	//g_ferri = & ferri;

	try
	{
		jcparam::CArguSet		arg_set;		// argument for process command line
		jcparam::CCmdLineParser::ParseCommandLine(
			line_parser, GetCommandLine(), arg_set);

		if ( arg_set.Exist(_T("random") ))	RandomSimulate(arg_set, ferri);
		if ( arg_set.Exist(_T("input"))	)	FileSimulate(arg_set, ferri);
	}
	catch (std::exception & err)
	{
		stdext::CJCException * jcerr = dynamic_cast<stdext::CJCException *>(&err);
		if (jcerr)
		{
		}
		printf("Error! %s\n", err.what());
		getchar();
	}	

	return 0;
}

