#pragma once

class CSmiCommand
{
protected:
	// 缺省的堆栈大小，当command的尺寸大于缺省值时，在堆上申请内存，
	// 否则直接使用栈上内存以提高效率。
	static const JCSIZE m_stack_size = 16;
	union
	{
		BYTE * b;
		WORD * w;
		DWORD * d;
	} m_cmd;
	JCSIZE m_len;
	BYTE m_stack[m_stack_size];

public:
	WORD id() { return m_cmd.w[0]; }

	inline void id(WORD ii)
	{
		m_cmd.b[0] = (ii >> 8) & 0xFF;
		m_cmd.b[1] = (ii & 0xFF);
	}
	BYTE & size() {return m_cmd.b[11]; }

public:
	inline CSmiCommand(JCSIZE count=m_stack_size) : m_len(count) 
	{ 
		m_cmd.b = (m_len > m_stack_size) ? (new BYTE[m_len]) : (m_stack); 
		Clear(); 
	}
	inline ~CSmiCommand(void)
	{
		if (m_len > m_stack_size)	 delete [] m_cmd.b;
	}
	inline JCSIZE length() const {return m_len;}
	inline const BYTE * data() const {return m_cmd.b;}
	inline BYTE & raw_byte(JCSIZE ii) { return m_cmd.b[ii]; };
	inline WORD raw_word(JCSIZE ii) { return MAKEWORD(m_cmd.b[ii*2+1], m_cmd.b[ii*2]); };

protected:
	inline void Clear(void) {memset(m_cmd.b, 0, m_len); }
};

// Command for block operations, such as Flash Read, Flash Write, ISP Read/Write, etc
class CSmiBlockCmd: public CSmiCommand
{
public:
	CSmiBlockCmd(JCSIZE count) : CSmiCommand(count) {};
	CSmiBlockCmd() : CSmiCommand() {};
	inline void block(WORD b) 
	{
		m_cmd.b[2] = (b >> 8) & 0xFF;
		m_cmd.b[3] = (b & 0xFF);
	}
	inline WORD block() { return ((WORD)m_cmd.b[2])<<8 | m_cmd.b[3]; }
	inline BYTE & page()		{ return m_cmd.b[4]; }
	inline BYTE & sector()		{ return m_cmd.b[5]; }
	inline BYTE & mu()			{ return m_cmd.b[6]; }
	inline BYTE & mode()		{ return m_cmd.b[8]; }
};

class CCmdIspBase : public CSmiBlockCmd
{
public:
	inline CCmdIspBase(JCSIZE count) : CSmiBlockCmd(count) { id(0xF10A); }
	inline WORD & isp_block()	{ return m_cmd.w[1]; }
};

class CCmdReadIsp : public CCmdIspBase
{	// 0xF10A
public:
	inline CCmdReadIsp(void) : CCmdIspBase(16) { id(0xF10A); }
};



class CCmdDownloadIsp : public CCmdIspBase
{	// 0xF10D / 0xF10B
public:
	inline CCmdDownloadIsp(bool b64k) : CCmdIspBase(512) 
	{ 
		id(b64k ? 0xF10D : 0xF10B); 
	}
};

//======
class CCmdReadFlash : public CSmiBlockCmd
{
public:
	inline CCmdReadFlash(void) : CSmiBlockCmd(16) { id(0xF00A); }
};

class CCmdWriteFlash : public CSmiBlockCmd
{	// 0xF10B
public:
	inline CCmdWriteFlash(void) : CSmiBlockCmd(16) { id(0xF10B); }
};

class CCmdEraseBlock : public CSmiBlockCmd
{	// 0xF00C
public:
	inline CCmdEraseBlock(void) : CSmiBlockCmd(16) { id(0xF00C); }
};