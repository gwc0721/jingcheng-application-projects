#pragma once



class CTraceInjApp : public jcapp::CJCAppicationSupport
{
public:
	CTraceInjApp(void);
public:
	virtual int Initialize(void);
	virtual int Run(void);
	virtual void CleanUp(void);
	virtual LPCTSTR AppDescription(void) const
	{
		static const TCHAR desc[] = 
			_T("Copy Compare Test Tool\n")
			_T("\t Silicon Motion\n");
		return desc;
	}

protected:
	typedef std::vector<CAtaTraceRow *> TRACE;
	void LoadPattern(const CJCStringT & fn, TRACE & trace);
	bool OpenDrive(TCHAR drive_letter, const CJCStringT & drive_type, IStorageDevice * &dev);
	bool RunTestPattern(IStorageDevice * dev, TRACE & trace);
	bool InvokeTrace(BYTE * buf, JCSIZE buf_secs, CAtaTraceRow * trace);
	// 如果data_pattern为NULL则使用trace所带的data，否则使用data_pattern指定的data
	//bool WriteTestPattern(IStorageDevice * dev, TRACE & trace, BYTE * data_pattern, LPCTSTR verb);
	//bool ReadCompareTest(IStorageDevice * dev, TRACE & trace);
	//bool BackupData(IStorageDevice * dev, TRACE & src_trace, TRACE & back_trace);
	bool PowerCycle(IStorageDevice * dev, UINT delay);

//
public:
	TCHAR	m_drive_letter;
	CJCStringT	m_drive_type;
	CJCStringT	m_test_script, m_data_pattern;		// file name of configuration.
	bool	m_backup, m_open_physical;
	WORD	m_clean_pattern;
	UINT	m_reset;
	FILESIZE	m_base_lba;
	static const TCHAR	LOG_CONFIG_FN[];

protected:
	TRACE	m_trace;
	TRACE	m_backup_trace;
	IStorageDevice * m_dev;
	// 用于读取data的临时buf，考虑到性能，申请的空间将按需扩大。
	BYTE *	m_temp_buf;
	JCSIZE	m_temp_len;
	CFileMapping * m_file_mapping;	// for data backup

	FILESIZE	m_total_read;
	FILESIZE	m_total_write;
	FILESIZE	m_dev_total_secs;
};

typedef jcapp::CJCApp<CTraceInjApp>	CApplication;