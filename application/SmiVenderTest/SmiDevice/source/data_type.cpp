#include "../include/data_type.h"


/*
///////////////////////////////////////////////////////////////////////////////
//--
bool CLinkTable::QueryInterface(const char * if_name, IJCInterface * &if_ptr)
{
	JCASSERT(NULL == if_ptr);
	bool br = false;
	if ( FastCmpA(jcparam::IF_NAME_VALUE_FORMAT, if_name) )
	{
		if_ptr = static_cast<IJCInterface*>(this);
		if_ptr->AddRef();
		br = true;
	}
	else br = __super::QueryInterface(if_name, if_ptr);
	return br;
}

void CLinkTable::Format(FILE * file, LPCTSTR format)
{
	static const LPCTSTR H_TYPE_STR_TAB[] = {
		_T("m"), _T("c"), _T("f"), 
	};
	if ( NULL == format || FastCmpT(EMPTY, format) )	format = _T("csv");
	
	if ( FastCmpT(_T("csv"), format) )
	{
		_ftprintf(file, _T("H-block, type, mother, child/fat1, temp/fat2, obsolete\n") );
		for (JCSIZE ii=0; ii < m_hblock_number; ++ ii)
		{
			Hblock * hb = m_hblock_list + ii;

			_ftprintf(file, _T("0x%04X, %s, "), ii, H_TYPE_STR_TAB[(int)hb->m_type]);
			hb->m_mother.format(file, _T(", "));
			hb->m_child.format(file, _T(", "));
			hb->m_temp.format(file, _T(", "));

			ITERATOR it = hb->m_obsolete.begin(); 
			ITERATOR endit = hb->m_obsolete.end();
			for ( ; it != endit; ++ it)
			{
				hb->m_mother.format(file, _T(" | "));
				//_ftprintf(file, _T("0x%04X(%02X), "), it->m_index, it->m_id);
			}
			_ftprintf(file, _T("\n"));
		}
	}
	else if ( FastCmpT(_T("org"), format) )
	{
		for (JCSIZE ii=0; ii < m_hblock_number; ++ ii)
		{
			_ftprintf(file, _T("0x%04X : "), ii);
			ITERATOR it = m_original[ii].begin(); 
			ITERATOR endit = m_original[ii].end();
			for ( ; it != endit; ++ it)		
			{
				it->format(file, _T(", "));
			}
			//_ftprintf(file, _T("0x%04X(%X, %X), "), it->m_index, it->m_id, it->m_ver);
			_ftprintf(file, _T("\n"));
		}
	}
}

CLinkTable::~CLinkTable(void) 
{
	delete [] m_hblock_list; 
	delete [] m_original;
}

void CLinkTable::Hblock::AddFblock(const Fblock & src, Fblock & dst)
{
	if (dst.m_index)
	{	// fat 存在，比较新旧
		if (src.m_ver > dst.m_ver)
		{
			m_obsolete.push_back(dst);
			dst = src;
		}
		else
		{
			m_obsolete.push_back(src);
		}
	}
	else dst = src;
}

void CLinkTable::AddHblock(WORD hblock, WORD fblock, BYTE id, BYTE serial)
{
	if ( hblock >= m_hblock_number ) return;
	Fblock fb(fblock, id, serial);

	// for debug
	m_original[hblock].push_back(fb);
	Hblock * hb = m_hblock_list + hblock;

	switch (fb.m_id)
	{
	case 2:		{	// mother or child
		Fblock & mother = hb->m_mother;
		//	mother不存在：插入mother
		if (mother.m_index == 0)	mother = fb;
		else		//	mother存在
		{
			//	版本比mother旧 : -> obsolete
			if (fb.m_ver < mother.m_ver)	hb->m_obsolete.push_back(fb);
			//	版本比mother新
			else	
			{
				Fblock & child = hb->m_child;
				if (child.m_index == 0)
				{	// child不存在 : 插入child
					child = fb;		
					hb->m_type = Hblock::CHILD;
				}
				else	// child存在
				{
					hb->m_obsolete.push_back(mother);
					//	比child旧 : mother -> obsolete, 插入mother
					if (fb.m_ver < child.m_ver)		mother = fb;
					//	比child新 : mother -> obsolete, child -> mother, 插入child
					else
					{
						mother = child;
						child = fb;
					}
				}
			}
		}
		break;
				}

	case 4:		// fat
		hb->m_type = Hblock::FAT;
		hb->AddFblock(fb, hb->m_child);
		break;

	case 8:
		hb->m_type = Hblock::FAT;
		hb->AddFblock(fb, hb->m_temp);
		break;

	case 0xA:
		hb->m_type = Hblock::CHILD;
		hb->AddFblock(fb, hb->m_temp);
		break;

	default:
		return;
	}

	hb->m_index = hblock;
}
*/


