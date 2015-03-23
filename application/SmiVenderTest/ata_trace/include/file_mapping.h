#pragma once

#include <stdext.h>
#include <vector>


///////////////////////////////////////////////////////////////////////////////
//---- file mapping  
//  file mapping类，用于实现File Mapping。
//	  实现并且管理对同一个文件的多份mapping。请求mapping时，自动实现地址对齐。
//	  实现对同一影射的自动缓存以及缓存管理。
//  
class CFileMapping : public IJCInterface
{
public:
	CFileMapping(const CJCStringT & fn);
	CFileMapping(HANDLE file, FILESIZE set_size);
	virtual ~CFileMapping(void);
	IMPLEMENT_INTERFACE;

protected:
	struct VIEW_NODE
	{
		VIEW_NODE(BYTE * _ptr, FILESIZE _start, JCSIZE len) 
			: ptr(_ptr), start(_start), length(len), ref_cnt(1)
		{ }
		BYTE * ptr;
		FILESIZE start;
		JCSIZE length;
		LONG ref_cnt;
	};
	//typedef std::map<JCSIZE, VIEW_NODE>	VIEW_MAP;
	typedef std::vector<VIEW_NODE>		VIEW_LIST;

public:
	virtual void SetSize(FILESIZE size);
	virtual FILESIZE GetSize(void) const {return m_file_size;}
	virtual LPVOID Mapping(FILESIZE offset, JCSIZE len);
	virtual void Unmapping(LPVOID ptr);
	// 向下对其
	template <typename T>	T AligneLo(T val)
	{
		T aligned = (val & (T)m_grn_mask);
		return aligned;
	}
	// 向上对其
	template <typename T>	T AligneHi(T val)
	{
		T aligned = ((val -1) & (T)m_grn_mask) + (T)m_granularity;
		return aligned;
	}
protected:
	VIEW_NODE * FindView(FILESIZE start, JCSIZE len);
	void InsertView(BYTE * ptr, FILESIZE start, JCSIZE len);

protected:
	HANDLE	m_src_file;
	HANDLE	m_mapping;
	UINT64	m_file_size;
	//VIEW_MAP m_view_map;
	VIEW_LIST m_view_list;

protected:
	static UINT64	m_granularity;
	static UINT64	m_grn_mask;
};

//extern "C" 
//{
	void CreateFileMappingObject(const CJCStringT & fn, CFileMapping * & mapping);
	void CreateFileMappingObject(HANDLE file, FILESIZE set_size, CFileMapping * & mapping);
//};