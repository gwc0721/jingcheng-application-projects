#pragma once

struct DEVICE_INFO
{
	DWORD	m_size;
	char	m_drive_letter;
	CString m_drive_name;
	CString m_serial_number;
	CString m_model_name;
	DWORD	m_capacity;
	CString	m_fw_version;
	int		m_init_bad;
	int		m_new_bad;
	int		m_error_code;
};

class CLogFile
{
public:
	CLogFile(void);
	~CLogFile(void);

	bool SetLogPath(LPCTSTR path, LPCTSTR path2);
	bool WriteLog1(const DEVICE_INFO & info, const CTime &start_time);
	bool WriteLog2(const DEVICE_INFO & info, const CTime &start_time);

	void SetDeviceName(LPCTSTR str)	{m_device_name = str;}
	void SetDeviceType(int ii)	{m_device_type = ii;}
	void SetTestMachine(LPCTSTR str)	{m_test_machine = str;}
	void SetToolVer(LPCTSTR str)	{m_tool_ver = str;}

protected:
	CString		m_path_log1;
	CString		m_path_log2;

	CString		m_device_name;
	int			m_device_type;
	CString		m_test_machine;
	CString		m_tool_ver;
};
