#pragma once

#include <jcparam.h>

/*
class CLinkTable 
	: virtual public jcparam::IValueFormat, virtual public jcparam::IValue, 
	public CJCInterfaceBase, public jcparam::CTypedValueBase, public Factory1<JCSIZE, CLinkTable>
{
	friend class Factory1<JCSIZE, CLinkTable>;

public:

protected:
	CLinkTable(JCSIZE nn) : m_hblock_number(nn), m_hblock_list(NULL), m_original(NULL)
	{
		m_original = new BlockList[nn];
		m_hblock_list = new Hblock[nn];
	}
	virtual ~CLinkTable(void);

public:
	struct Fblock
	{
		Fblock(void) : m_index(0), m_id(0) {}
		Fblock(WORD index, BYTE id) : m_index(index), m_id(id>>4), m_ver(id & 0xF) {}
		Fblock(WORD index, BYTE id, BYTE serial) : m_index(index), m_id(id), m_ver(serial) {}
		void format(FILE * file, LPCTSTR sep)
		{
			if (m_index)
				_ftprintf(file, _T("0x%04X(%X:%X)%s"), m_index, m_id, m_ver, sep); 
			else
				_ftprintf(file, _T("           %s"), sep);
		}
		WORD m_index;
		BYTE m_id;
		BYTE m_ver;
	};

	typedef std::vector<Fblock>	BlockList;
	typedef std::vector<Fblock>::iterator	ITERATOR;

	struct Hblock
	{
		Hblock(void) : m_index(0xFFFF), m_type(MOTHER) {}
		enum HTYPE
		{
			MOTHER = 0,
			CHILD = 1,
			FAT = 2,
		};

		void AddFblock(const Fblock & src, Fblock & dst);

		WORD m_index;
		HTYPE m_type;
		Fblock m_mother;
		Fblock m_child;	// child or fat1
		Fblock m_temp;	// temp or fat2
		BlockList	m_obsolete;
	};

	virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr);
	virtual void Format(FILE * file, LPCTSTR format);
	virtual void WriteHeader(FILE * file) {}

	void AddHblock(WORD hblock, WORD fblock, BYTE id, BYTE serial);

protected:

	BlockList*	m_original;
	Hblock *	m_hblock_list;
	JCSIZE		m_hblock_number;
};
*/






