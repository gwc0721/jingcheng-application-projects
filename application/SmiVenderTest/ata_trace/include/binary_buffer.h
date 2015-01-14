#pragma once

#include <jcparam.h>
#include <map>
#include <list>

#include "file_mapping.h"



void CreateFileMappingBuf(CFileMapping * mapping, JCSIZE offset_sec, JCSIZE secs, jcparam::IBinaryBuffer * & buf);



///////////////////////////////////////////////////////////////////////////////
//----  CBinaryBuffer  --------------------------------------------------------



class CBinaryBufferBase	: public jcparam::IBinaryBuffer
{
public:
	virtual void GetValueText(CJCStringT & str) const {};
	virtual void SetValueText(LPCTSTR str)  {};
	virtual void GetSubValue(LPCTSTR name, jcparam::IValue * & val) {};
	virtual void SetSubValue(LPCTSTR name, jcparam::IValue * val) {};

	virtual void ToStream(jcparam::IJCStream * stream, jcparam::VAL_FORMAT fmt, DWORD param) const;
	virtual void FromStream(jcparam::IJCStream * str, jcparam::VAL_FORMAT param) { NOT_SUPPORT0; };

protected:
	// 按照文本格式输出，
	//  offset：输出偏移量，字节为单位，行对齐（仅16的整数倍有效）
	//  len：输出长度，字节为单位，行对齐
	void OutputText(jcparam::IJCStream * stream, JCSIZE offset, JCSIZE len) const;
	void OutputBinary(jcparam::IJCStream * stream, JCSIZE offset, JCSIZE len) const;
};



///////////////////////////////////////////////////////////////////////////////
//----  file mapping buffer
//  支持file mapping的binary buffer。可以共享同一个file mapping。
//  自动处理地址对齐
class CFileMappingBuf	: public CBinaryBufferBase
{
public:
	CFileMappingBuf(CFileMapping * mapping, JCSIZE offset_sec, JCSIZE secs);
	virtual ~CFileMappingBuf(void);
	IMPLEMENT_INTERFACE;

public:
	virtual BYTE * Lock(void);
	virtual void Unlock(BYTE * ptr);
	virtual JCSIZE GetSize(void) const {return m_aligned_len / SECTOR_SIZE;}

protected:
	CFileMapping * m_mapping;
	FILESIZE	m_aligned_start;	// aligned offset in file in byte
	JCSIZE		m_offset;			// actual pointer's offset from aligned position in byte
	JCSIZE		m_aligned_len;		// aligned size in byte
	LONG		m_locked;			// locked counter
	BYTE*		m_ptr;				// mapped pointer
};

///////////////////////////////////////////////////////////////////////////////
//----  

// structure
//	binary_buffer
//		- address
//			- type (normal, lba, flash)
//			- value
//		- length	(read only)
/*
class CBinaryBuffer
	//: virtual public jcparam::IVisibleValue
	: virtual public IBinaryBuffer
	, public CJCInterfaceBase
	, public Factory1<JCSIZE, CBinaryBuffer>
{
	friend class Factory1<JCSIZE, CBinaryBuffer>;
protected:
	CBinaryBuffer(JCSIZE count);
	~CBinaryBuffer();

public:
	virtual void GetValueText(CJCStringT & str) const {};
	virtual void SetValueText(LPCTSTR str)  {};

	virtual void GetSubValue(LPCTSTR name, jcparam::IValue * & val);
	virtual void SetSubValue(LPCTSTR name, jcparam::IValue * val);

	virtual void ToStream(jcparam::IJCStream * stream, jcparam::VAL_FORMAT fmt, DWORD param) const;
	virtual void FromStream(jcparam::IJCStream * str, jcparam::VAL_FORMAT param) { NOT_SUPPORT0; };

protected:
	// 按照文本格式输出，
	//  offset：输出偏移量，字节为单位，行对齐（仅16的整数倍有效）
	//  len：输出长度，字节为单位，行对齐
	void OutputText(jcparam::IJCStream * stream, JCSIZE offset, JCSIZE len) const;
	void OutputBinary(jcparam::IJCStream * stream, JCSIZE offset, JCSIZE len) const;

public:
	BYTE* Lock(void)	{ return m_array; }
	//const BYTE* Lock(void) const {return m_array;}
	void Unlock(BYTE * ptr) {};

	JCSIZE GetSize(void) const { return m_size; }
	void GetAddress(IAddress * &add)
	{
		JCASSERT(NULL == add);
		add = m_address;
		if (add) add->AddRef();
	}
	void SetAddress(IAddress * add)
	{
		m_address = add;
		if (add) add->AddRef();
	}
	void SetMemoryAddress(JCSIZE add);
	void SetFlashAddress(const CFlashAddress & add);
	void SetBlockAddress(FILESIZE add);

protected:
	BYTE *		m_array;
	IAddress *	m_address;
	JCSIZE		m_size;		// always in byte
};
*/
