#include "stdafx.h"
#include "LogFile.h"
#include "UpsnMessageDlg.h"

CLogFile::CLogFile(void)
{
}

CLogFile::~CLogFile(void)
{
}

bool CLogFile::SetLogPath(LPCTSTR path1, LPCTSTR path2)
{
	CTime	cur_time=CTime::GetCurrentTime();
	int		year =cur_time.GetYear() % 100;
	int		month = cur_time.GetMonth();
	int		day = cur_time.GetDay();

	m_path_log1.Format(_T("%s\\%s_%s_%s_%d%02d%02d.csv"), 
		path1, m_device_name, m_test_machine, m_tool_ver, year, month, day);
	m_path_log2 = path2;

	// check if log file 1 exist
	if ( GetFileAttributes(path1) == -1)		CreateDirectory(path1, NULL);
	if ( GetFileAttributes(m_path_log1) == -1)
	{	// file is not exist, cread file and write header
		FILE * file = NULL;
		_tfopen_s(&file, m_path_log1, _T("a+"));
		if (NULL == file)	return false;
		fprintf_s(file, "Item NO,,,,,,,, item:01-01\n");
		fprintf_s(file, "SerialNo,Date,StartTime,EndTime,Judge,TESTVer,Mode,Type,Data\n");
		fclose(file);
	}
	return true;
}

bool CLogFile::WriteLog1(const DEVICE_INFO & info, const CTime &start_time)
{
	FILE * file = NULL;
	_tfopen_s(&file, m_path_log1, _T("a+") );
	if (NULL == file)
	{
		CString err_msg;
		err_msg.Format(_T("Creat file %s Fail! "), m_path_log1);
		AfxMessageBox(err_msg, MB_OK);
		return false;
	}

	CTime	cur_time=CTime::GetCurrentTime();
	int		year =cur_time.GetYear();
	int		month = cur_time.GetMonth();
	int		day = cur_time.GetDay();

	int iEndHour=cur_time.GetHour();
	int iEndMinute=cur_time.GetMinute();
	int iEndSecond=cur_time.GetSecond();

	int iStartHour=start_time.GetHour();
	int iStartMinute=start_time.GetMinute();
	int iStartSecond=start_time.GetSecond();

	// date, start_time, end_time
	fprintf_s(file, "%S,%d/%d/%d,%d:%d:%d,%d:%d:%d,",
		info.m_serial_number, year, month, day, 
		iStartHour, iStartMinute, iStartSecond, 
		iEndHour, iEndMinute, iEndSecond);
	// result_code
	//if ( (UPSN_PASS == info.m_error_code )	|| (UPSN_VERIFYSN_PASS == info.m_error_code) )
	if (info.m_pass_fail)		fprintf_s(file, "Good,");
	else						fprintf_s(file, "NG- 0x%X,", info.m_error_code); 
	// program_version, mode, device_type, 
	fprintf_s(file, "%S,Test,%S,", m_tool_ver, m_device_type);
	// option(init bad), option2(new bad), option3(f/w ver)/
	fprintf_s(file, "%d,%d,%S\n", info.m_init_bad, info.m_new_bad, info.m_fw_version);
	fclose(file);
	return true;
}

bool CLogFile::WriteLog2(const DEVICE_INFO & info, const CTime &start_time)
{
	// create file name
	CString file_name;
	file_name.Format(_T("%s\\%s.001"), m_path_log2, info.m_serial_number);

	FILE * file = NULL;
	_tfopen_s(&file, file_name, _T("a+") );
	if (NULL == file)
	{
		CString err_msg;
		err_msg.Format(_T("Creat file %s Fail! "), file_name);
		AfxMessageBox(err_msg, MB_OK);
		return false;
	}
	
	CTime	cur_time=CTime::GetCurrentTime();
	int		year =cur_time.GetYear();
	int		month = cur_time.GetMonth();
	int		day = cur_time.GetDay();

	// line 1
	fprintf_s(file, "%S,%s,%d/%d/%d,%d:%d:%d,%d\n",
		info.m_serial_number, LOG_TEST_PROCESS, year, month, day,
		start_time.GetHour(), start_time.GetMinute(), start_time.GetSecond(),
		(UPSN_PASS == info.m_error_code)?(1):(0) );

	// line 2: 
	fprintf_s(file, "%S,%s,%S,%S,%d,%S\n",
		m_tool_ver, LOG_TEST_MODE, m_device_type, m_test_machine, 
		info.m_init_bad, info.m_fw_version);

	fclose(file);
	return true;
}