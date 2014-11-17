
#include <guiddef.h>

#include "../include/single_tone.h"

// support WIN32 only

// {DE4ED559-8F43-4db7-90EB-A946E3D52170}
static const GUID SINGLE_TONE_GUID = 
{ 0xde4ed559, 0x8f43, 0x4db7, { 0x90, 0xeb, 0xa9, 0x46, 0xe3, 0xd5, 0x21, 0x70 } };

#ifdef _DEBUG
void LogVirtualMapping(const CJCStringT & fn, DWORD page_size)
{
	MEMORY_BASIC_INFORMATION mbi;
	// 内存测试
	FILE * file = NULL;
	_tfopen_s(&file, fn.c_str(), _T("w+"));
	fprintf_s(file, "add, size, status, type\n");
	for (ULONG ii = 0; ii < 0x80000000; )
	{
		JCSIZE ir = VirtualQuery((LPVOID)ii, &mbi, sizeof(mbi));
		if (ir)
		{
			fprintf_s(file, "%08X, %d, %X, %X\n", ii, mbi.RegionSize, mbi.State, mbi.Type);
			ii += mbi.RegionSize;
		}
		else	
		{
			fprintf_s(file, "%08X, %X, --, --\n", ii, page_size);
			ii += page_size;
		}
	}
	fclose(file);
}
#else
#define LogVirtualMapping(a, b)
#endif

#ifdef _DEBUG
#define _LOG_(...)	{			\
	TCHAR str_log[64] = {0};	\
	_stprintf_s(str_log, __VA_ARGS__);	\
	OutputDebugString(str_log);		}

#define _FATAL_(...)	{	\
	TCHAR str_log[64] = {0};	\
	_stprintf_s(str_log, __VA_ARGS__);	\
	_tprintf_s(__VA_ARGS__);	\
	exit(-1);				}

#else
#define _LOG_(...)

#define _FATAL_(...)	{	\
	_tprintf_s(__VA_ARGS__);	\
	exit(-1);				}

#endif

///////////////////////////////////////////////////////////////////////////////
//-- CSingleToneManager
#define	MAX_ENTRY		128
// 容纳Single Tone对象的数量，必须是2的整次幂
#define MAX_INSTANCE	512

class CSingleToneManager
{
public:
	GUID				m_signature;
	// 用于保护entry list
	CRITICAL_SECTION	m_critical;
	JCSIZE				m_entry_count;
	CSingleToneEntry *	m_entry_list[MAX_ENTRY];
	CSingleToneBase *	m_instance[MAX_INSTANCE];
	// 记录instance[i]是由哪个模块创建的，在模块unregister时，release instance。
	// 通过两个并行数组，为了使每个元素的地址对齐。
	BYTE				m_instance_entry[MAX_INSTANCE];

#ifdef _DEBUG
	// 用于HASH碰撞测试
	JCSIZE m_hash_conflict;
	JCSIZE m_total_instance;
#endif

private:
	CSingleToneManager(void) {};
	~CSingleToneManager(void) {};

public:
	static bool IsSingleToneManager(LPVOID ptr);
	void Initialize(JCSIZE size, CSingleToneEntry * ptr);
	void CleanUp(void);
	bool RegisterContainer(CSingleToneEntry* entry, UINT &id);
	// 返回剩余entry的个数，如果为0，则执行清除工作
	UINT UnRegisterContainer(UINT);
	bool RegisterStInstance(const GUID & guid, CSingleToneBase * obj, UINT entry_id);
	bool QueryStInstance(const GUID & guid, CSingleToneBase * & obj);
	WORD CalculateHash(const GUID & guid);
	//void RemoveStInstance(const GUID * guid);
};


bool CSingleToneManager::IsSingleToneManager(LPVOID ptr)
{
	CSingleToneManager * base = (CSingleToneManager *)(ptr);
	return IsEqualGUID(base->m_signature, SINGLE_TONE_GUID);
}

void CSingleToneManager::Initialize(JCSIZE size, CSingleToneEntry * entry)
{
	memset(this, 0, size);
	memcpy_s(&m_signature, sizeof(GUID), &SINGLE_TONE_GUID, sizeof(GUID));
	InitializeCriticalSection(&m_critical);
	EnterCriticalSection(&m_critical);
	m_entry_count = 1;
	m_entry_list[0] = entry;

#ifdef _DEBUG
	m_hash_conflict = 0;
	m_total_instance = 0;
#endif
	LeaveCriticalSection(&m_critical);
}

bool CSingleToneManager::RegisterContainer(CSingleToneEntry* entry, UINT &id)
{
	if (m_entry_count >= MAX_ENTRY) return false;
	EnterCriticalSection(&m_critical);
	id = 0;
	// search for empty entry, 0 is creater, skip
	UINT ii =1;
	for (; ii<MAX_ENTRY; ++ii) if (m_entry_list[ii] == NULL) break;
	id = ii;
	m_entry_list[id] = entry;
	m_entry_count ++;
	LeaveCriticalSection(&m_critical);
	return true;
}

UINT CSingleToneManager::UnRegisterContainer(UINT id)
{
	_LOG_(_T("unregister st entry %d\n"), id);
	UINT remain = 0;
	if (id >= MAX_ENTRY) return false;
	EnterCriticalSection(&m_critical);
	if (m_entry_list[id])
	{
		m_entry_list[id] = NULL;
		m_entry_count --;
	}
	remain = m_entry_count;
	LeaveCriticalSection(&m_critical);
	return remain;
}

WORD CSingleToneManager::CalculateHash(const GUID & guid)
{
	static const int num = sizeof(GUID) / sizeof (WORD);
	const WORD *buf = reinterpret_cast<const WORD*>(&guid);
	WORD hash = 0;
	for (int ii = 0; ii < num; ++ii) hash += buf[ii];
	return hash;
}

bool CSingleToneManager::RegisterStInstance(const GUID & guid, CSingleToneBase * obj, UINT entry_id)
{
	// 采用Hash表，哈希算法：将GUID的各位（2字节）相加，去最低9位（512元素）。
	//  如果Hash值碰撞，则顺序向后搜索。
	WORD hash = CalculateHash(guid);
	hash &= (MAX_INSTANCE-1);

	WORD ii = hash;
	bool br = false;		// full or not

#ifdef _DEBUG
	m_total_instance ++;
#endif

	do
	{
		if ( InterlockedCompareExchangePointer((PVOID*)( m_instance + ii), obj, NULL) == NULL)
		{
			m_instance_entry[ii] = entry_id;
			br = true;	//
			break;
		}
#ifdef _DEBUG
		m_hash_conflict++;
#endif
		ii ++;	ii &= (MAX_INSTANCE-1);
		// == if (ii >= MAX_INSTANCE) ii = 0;
	}
	while (ii != hash);

	return br;

/*
	// find a empty entry
	EnterCriticalSection(&m_critical);
	UINT ii = 0;
	for (; ii<MAX_INSTANCE; ++ii)
	{
		if (m_instance[ii] == NULL) break;
	}
	if (ii < MAX_INSTANCE)
	{
		memcpy_s(m_instance_id + ii, sizeof(GUID), &guid, sizeof(GUID));
		m_instance[ii] = obj;
	}
	LeaveCriticalSection(&m_critical);
	return (ii < MAX_INSTANCE);
*/
}

bool CSingleToneManager::QueryStInstance(const GUID & guid, CSingleToneBase * & obj)
{
	WORD hash = CalculateHash(guid);
	hash &= (MAX_INSTANCE-1);

	WORD ii = hash;
	bool br = false;		// full or not

	EnterCriticalSection(&m_critical);
	do
	{
		if ( m_instance[ii] == NULL) break;		// 未找到。意味着表中不能有空隙，不能删除表项。
		if ( IsEqualGUID(m_instance[ii]->GetGuid(), guid) )
		{
			br = true;
			obj = m_instance[ii];
			break;
		}
		ii ++;	ii &= (MAX_INSTANCE-1);
		// == if (ii >= MAX_INSTANCE) ii = 0;
	}
	while (ii != hash);
	LeaveCriticalSection(&m_critical);

	return br;

/*
	// search guid by linear
	EnterCriticalSection(&m_critical);
	UINT ii = 0;
	for (; ii <MAX_INSTANCE; ++ii)
	{
		if (IsEqualGUID(guid, m_instance_id[ii])) break;
	}
	if (ii < MAX_INSTANCE)	obj = m_instance[ii];
	LeaveCriticalSection(&m_critical);
*/
}

void CSingleToneManager::CleanUp(void)
{
	_LOG_(_T("clean up st manager\n"));
#ifdef _DEBUG
	_LOG_(_T("total hash registered: %d, conflicted: %d\n"), m_total_instance, m_hash_conflict)
#endif
	EnterCriticalSection(&m_critical);
	UINT ii = 0;
	for (; ii <MAX_INSTANCE; ++ii)
	{
		if (m_instance[ii])
		{
			m_instance[ii]->Release();
			m_instance[ii] = NULL;
			m_instance_entry[ii] = 0;
			//memset(m_instance_id + ii, 0, sizeof(GUID));
		}
	}
	LeaveCriticalSection(&m_critical);
	DeleteCriticalSection(&m_critical);
}

//void RemoveStInstance(const GUID * guid);


///////////////////////////////////////////////////////////////////////////////
//-- CSingleToneEntry
CSingleToneEntry::CSingleToneEntry(void)
: m_base(NULL), m_entry_id(UINT_MAX)
{
	_LOG_(_T("StEntry this = 0x%08X\n"), (UINT)this);
	_LOG_(_T("sizeof(CRITICAL_SECTION) = %d\n"), sizeof(CRITICAL_SECTION));

	// 取得系统Page大小
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	DWORD page_size = si.dwPageSize;
	if (sizeof(CSingleToneManager) >= page_size) _FATAL_(
		_T("CSingleToneManager (%d) is large than page size (%d)"), sizeof(CSingleToneManager), page_size)

	MEMORY_BASIC_INFORMATION mbi;
	LogVirtualMapping(_T("virtal_mapping_01.txt"), page_size);

	// 搜索所有region，查找single tone manager
	LPVOID ptr = si.lpMinimumApplicationAddress;
	for ( ; ptr < si.lpMaximumApplicationAddress; )
	{
		JCSIZE ir = VirtualQuery(ptr, &mbi, sizeof(mbi));
		if (ir == 0)	_FATAL_(_T("fatal error: VirtualQuery failed\n"));
		if ( (mbi.State & MEM_COMMIT) && CSingleToneManager::IsSingleToneManager(ptr) )
		{
			_LOG_(_T("found signature at add = 0x%08X\n"), (ULONG32)ptr);
			m_base = reinterpret_cast<CSingleToneManager*>(ptr);
			bool br = m_base->RegisterContainer(this, m_entry_id);
			if (!br) _FATAL_(_T("entry of single tone manager is full\n"))
			break;
		}
		ptr = (LPVOID)((ULONG32)ptr + mbi.RegionSize);
	}

	if (m_base == NULL)
	{
		m_base = (CSingleToneManager*)(VirtualAlloc(NULL, page_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		if (m_base == NULL)	_FATAL_(_T("allocate virtual page failed. error = %d\n"), GetLastError() );
		_LOG_(_T("allocated new virtual page at add = 0x%08X\n"), m_base);
		m_entry_id = 0;
		m_base->Initialize(page_size, this);
	}
}

CSingleToneEntry::~CSingleToneEntry(void)
{
	_LOG_(_T("distruct StEntry this = 0x%08X\n"), (UINT)this);
	UINT remain = UINT_MAX;
	if (m_base) remain = m_base->UnRegisterContainer(m_entry_id);
	if (remain == 0)
	{
		m_base->CleanUp();
	}
}

CSingleToneEntry * CSingleToneEntry::Instance(void)
{
	static CSingleToneEntry _single_tone_container;
	return & _single_tone_container;
}

bool CSingleToneEntry::QueryStInstance(const GUID & guid, CSingleToneBase * & obj)
{
	return m_base->QueryStInstance(guid, obj);
}

bool CSingleToneEntry::RegisterStInstance(const GUID & guid, CSingleToneBase * obj)
{
	return m_base->RegisterStInstance(guid, obj, m_entry_id);
}

