#pragma once

#include "../../CoreMntLib/include/mntImage.h"
#include "../../DeviceConfig/DeviceConfig.h"

#include <ata_trace.h>


class CDriverImageFile : public IImage
{
public:
	// create from configuration file
	CDriverImageFile(const CJCStringT & config);
	~CDriverImageFile(void);

	IMPLEMENT_INTERFACE;

// type definition
protected:

// interface of IImage
public:
	virtual ULONG32	Read(UCHAR * buf, ULONG64 lba, ULONG32 secs);
	virtual ULONG32	Write(const UCHAR * buf, ULONG64 lba, ULONG32 secs);
	virtual ULONG32	FlushCache(void);
	virtual ULONG32	DeviceControl(ULONG code, READ_WRITE read_write, UCHAR * buf, ULONG32 & data_size, ULONG32 buf_size);
	virtual ULONG64	GetSize(void) const {return m_file_secs;}

// interface of ITestAuditPort
public:
	virtual void SetStatus(const CJCStringT & status, jcparam::IValue * param_set);
	virtual void SendEvent(void);

protected:
	bool Initialize(void);
	bool OpenStorageFile(const CJCStringT fn, ULONG64 total_sec/*, HANDLE & file, HANDLE & mapping*/);
	ULONG32 ScsiCommand(READ_WRITE rd_wr, UCHAR *buf, JCSIZE buf_len, UCHAR *cb, JCSIZE cb_length, UINT timeout);

	//LPVOID FindViewCache(ULONG64 lba);
	//LPVOID CreateViewCache(ULONG64 lba);

	void LogTrace(BYTE cmd, ULONG64 lba, ULONG32 secs);
	UCHAR * MappingFile(ULONG64 lba, ULONG32 secs, JCSIZE & data_len, JCSIZE & offset)
	{
		ULONG64 start = SECTOR_TO_BYTE(lba);
		data_len = SECTOR_TO_BYTE(secs);

		ULONG64 aligned_start = m_file_mapping->AligneLo(start);		JCASSERT(aligned_start <= start);
		offset = (JCSIZE)(start - aligned_start);

		ULONG64 end = start + data_len;
		ULONG64 aligned_end = m_file_mapping->AligneHi(end);	JCASSERT(aligned_end >= end);

		JCSIZE aligned_len = (JCSIZE)(aligned_end - aligned_start);

		// copyt data to buffer
		UCHAR * ptr = (UCHAR*)(m_file_mapping->Mapping(aligned_start, aligned_len) );
		return ptr;
	}

protected:
	CFileMapping *	m_file_mapping;
	ULONG64			m_file_secs;
	HANDLE			m_trace_file;
	double			m_ts_cycle;
	ULONG32			m_store;

	LONGLONG		m_start_ts;

	CDeviceConfig	m_config;
};


