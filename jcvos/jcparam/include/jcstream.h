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
	CReadIterator(jcparam::IJCStream* stream);
	~CReadIterator(void);

public:
	CReadIterator& operator ++ (void);
	CReadIterator operator ++ (int);
	wchar_t & operator * (void);
	bool operator == (const CReadIterator *);
	operator const wchar_t*() const { return &m_cur; }

protected:
	jcparam::IJCStream * m_stream;
	wchar_t	m_cur;
};


///////////////////////////////////////////////////////////////////////////////
// -- streams

class CStreamStdIn	: public jcparam::IJCStream
{
protected:
	CStreamStdIn(void);
	~CStreamStdIn(void);

public:
	virtual void Put(wchar_t)	{};
	virtual wchar_t Get(void)	{};
	virtual void Put(const wchar_t * str, JCSIZE len) {};
	virtual JCSIZE Get(wchar_t * str, JCSIZE len) {};
	virtual void Format(LPCTSTR f, ...) {};
	virtual LPCTSTR GetName(void) const {return _T("#stdin"); };

protected:
	FILE * m_std_in;
};



///////////////////////////////////////////////////////////////////////////////
// -- file stream
void CreateStreamFile(const CJCStringT & file_name, jcparam::READ_WRITE, jcparam::IJCStream * & stream);


class CStreamFile : public jcparam::IJCStream
	, public CJCInterfaceBase
{
public:
	CStreamFile(const CJCStringT & file_name);
	CStreamFile(jcparam::READ_WRITE, FILE * file, const CJCStringT & file_name);
	virtual ~CStreamFile(void);

	static const wchar_t * STREAM_EOF;
public:
	virtual void Put(wchar_t);
	virtual wchar_t Get(void);

	virtual void Put(const wchar_t * str, JCSIZE len);
	virtual JCSIZE Get(wchar_t * str, JCSIZE len);
	virtual void Format(LPCTSTR f, ...);

	virtual bool IsEof(void)	{return m_first == STREAM_EOF;}
	virtual LPCTSTR GetName(void) const {return m_file_name.c_str();};


protected:
	bool ReadFromFile(void);

protected:
	CJCStringT	m_file_name;
	FILE * m_file;
	wchar_t * m_buf;
	wchar_t * m_first;
	wchar_t * m_last;

	char * m_f_buf;
	char * m_f_last;

	jcparam::READ_WRITE m_rd;
};

///////////////////////////////////////////////////////////////////////////////
// -- std out stream
void CreateStreamStdout(jcparam::IJCStream * & stream);

class CStreamStdOut : public CStreamFile
{
public:
	CStreamStdOut(void) : CStreamFile(jcparam::WRITE, stdout, _T("")) {}
	virtual ~CStreamStdOut(void) { m_file = NULL; }
	virtual LPCTSTR GetName(void) const {return _T("#stdout");};
};


///////////////////////////////////////////////////////////////////////////////
// -- string stream
void CreateStreamString(CJCStringT * str, jcparam::IJCStream * & stream);

class CStreamString : public jcparam::IJCStream
	, public CJCInterfaceBase
{
public:
	CStreamString(CJCStringT * str);
	~CStreamString(void);

public:
	void Put(wchar_t);
	wchar_t Get(void) {NOT_SUPPORT(wchar_t);}
	
	void Put(const wchar_t * str, JCSIZE len);
	JCSIZE Get(wchar_t * str, JCSIZE len) {NOT_SUPPORT(JCSIZE);}
	void Format(LPCTSTR fmt, ...);
	bool IsEof(void) {NOT_SUPPORT(bool);};
	virtual LPCTSTR GetName(void) const {return _T("#string");};

protected:
	CJCStringT * m_str;
	bool	m_auto_del;

};

///////////////////////////////////////////////////////////////////////////////
// -- binary file stream
void CreateStreamBinaryFile(const CJCStringT & file_name, jcparam::READ_WRITE, jcparam::IJCStream * & stream);
void CreateStreamBinaryFile(FILE * file, jcparam::READ_WRITE, jcparam::IJCStream * & stream);

class CStreamBinaryFile : public jcparam::IJCStream
	, public CJCInterfaceBase
{
public:
	CStreamBinaryFile(jcparam::READ_WRITE, FILE * file, const CJCStringT & file_name);
	virtual ~CStreamBinaryFile(void);

	static const wchar_t * STREAM_EOF;
public:
	virtual void Put(wchar_t);
	virtual wchar_t Get(void);

	virtual void Put(const wchar_t * str, JCSIZE len);
	virtual JCSIZE Get(wchar_t * str, JCSIZE len);
	virtual void Format(LPCTSTR f, ...)		{	NOT_SUPPORT0;	}

	virtual bool IsEof(void)	{return feof(m_file) != 0;}
	virtual LPCTSTR GetName(void) const {return m_file_name.c_str();};

protected:
	FILE * m_file;
	jcparam::READ_WRITE m_rd;
	CJCStringT m_file_name;
};