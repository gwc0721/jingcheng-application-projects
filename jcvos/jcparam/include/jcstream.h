#pragma once

#include "ivalue.h"

//using namespace jcparam;

class CEofException : public stdext::CJCException
{
public:
	CEofException() : stdext::CJCException(_T("EOF"), stdext::CJCException::ERR_APP, 1) {}
};

///////////////////////////////////////////////////////////////////////////////
// -- iterators

class CReadIterator 
{
public:
	typedef std::forward_iterator_tag iterator_category	;
	typedef	wchar_t value_type;
	typedef	wchar_t * difference_type;
	typedef	wchar_t * pointer;
	typedef	wchar_t & reference;

public:
	CReadIterator(const CReadIterator & it)	: m_stream(it.m_stream), m_cur(it.m_cur) {};
	// EOF iterator
	CReadIterator(void) : m_stream(NULL), m_cur(0) {};
	CReadIterator(jcparam::IStream* stream);
	~CReadIterator(void);

public:
	CReadIterator& operator ++ (void);
	CReadIterator operator ++ (int);
	wchar_t & operator * (void);
	bool operator == (const CReadIterator *);
	operator const wchar_t*() const { return &m_cur; }

protected:
	jcparam::IStream * m_stream;
	wchar_t	m_cur;
};


///////////////////////////////////////////////////////////////////////////////
// -- streams

class CStreamStdIn	: public jcparam::IStream
{
protected:
	CStreamStdIn(void);
	~CStreamStdIn(void);

public:
	//virtual void GetCurIterator(jcparam::READ_WRITE rd, jcparam::IIterator * & it) = 0;
	//virtual void GetLastIterator(jcparam::READ_WRITE rd, jcparam::IIterator * & it) = 0;
	virtual void Put(wchar_t)	{};
	virtual wchar_t Get(void)	{};
	virtual void Put(const wchar_t * str, JCSIZE len) {};
	virtual JCSIZE Get(wchar_t * str, JCSIZE len) {};
	virtual void Format(LPCTSTR f, ...) {};

protected:
	FILE * m_std_in;
};


class CStreamStdOut;

class CStreamFile;




void CreateStreamFile(const CJCStringT & file_name, jcparam::READ_WRITE, jcparam::IStream * & stream);


class CStreamFile : public jcparam::IStream
	, public CJCInterfaceBase
{
public:
	CStreamFile(const CJCStringT & file_name);
	CStreamFile(jcparam::READ_WRITE, FILE * file);
	~CStreamFile(void);

	static const wchar_t * STREAM_EOF;
public:
	//virtual void GetCurIterator(jcparam::READ_WRITE rd, jcparam::IIterator * & it);
	//virtual void GetLastIterator(jcparam::READ_WRITE rd, jcparam::IIterator * & it);
	virtual void Put(wchar_t);
	virtual wchar_t Get(void);

	virtual void Put(const wchar_t * str, JCSIZE len);
	virtual JCSIZE Get(wchar_t * str, JCSIZE len);
	virtual void Format(LPCTSTR f, ...);

	virtual bool IsEof(void)	{return m_first == STREAM_EOF;}

protected:
	bool ReadFromFile(void);

protected:
	FILE * m_file;
	wchar_t * m_buf;
	wchar_t * m_first;
	wchar_t * m_last;

	char * m_f_buf;
	char * m_f_last;

	jcparam::READ_WRITE m_rd;
};

class CStreamString;

