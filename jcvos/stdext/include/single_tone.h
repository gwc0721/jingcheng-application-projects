#pragma once

#include "comm_define.h"

// 提供一个进程全局single tone的管理。任何基于single tone的全局对象，
// 在整个进程中确保只有一个实例，不论是静态库还是动态库

// Single Tone的使用方法：
//  1. 定义Single Tone类：
//		方法(1), Single Tone类从CSingleToneBase继承，并且实现所有方法以及静态函数：
//			const GUID & Guid(void)	用以返回类的GUID。
//			实现静态函数Instance()用于保存和返回Single Tone指针。
//		方法(2)，定义一个普通类MyClass，实现静态函数	const GUID & Guid(void)		
//			定义typedef CGlobalSingleTone<MyClass> MySingleTone.
//			MySingleTone既是MyClass的Single Tone对象类。
//			用CLocalSingleTone<MyClass> 既可替换实现局部的静态对象而不影响应用程序编译
//	2. 使用：
//		需要使用Single Tone时，可以通过静态函数MySingleTone::Instance()返回全局唯一的对象指针。
//		

class CSingleToneEntry;

class CSingleToneManager;

class CSingleToneBase
{
public:
	// 当程序（模块）推出前，删除single tone对象。目前必须用delete删除
	virtual void Release(void) = 0;
	// 返回对象所属类的GUID
	virtual const GUID & GetGuid(void) const = 0;
};

template <class T>
class CGlobalSingleTone : public T, public CSingleToneBase
{
public:
	CGlobalSingleTone<T>(void) {};
	virtual ~CGlobalSingleTone<T>(void) {};
	virtual void Release(void)	{delete this;};
	virtual const GUID & GetGuid(void) const {return T::Guid();};
	typedef CGlobalSingleTone<T>*	LPTHIS_TYPE;

	static LPTHIS_TYPE Instance(void)
	{
		static LPTHIS_TYPE instance = NULL;
		if (instance == NULL)	CSingleToneEntry::GetInstance<CGlobalSingleTone<T> >(instance);
		return instance;
	}
};

template <class T>
class CLocalSingleTone : public T
{
public:
	typedef CLocalSingleTone<T>*	LPTHIS_TYPE;
	typedef CLocalSingleTone<T>		THIS_TYPE;
	static LPTHIS_TYPE Instance(void)
	{
		static THIS_TYPE instance;
		return & instance;
	}
};

class CSingleToneEntry
{
public:
	CSingleToneEntry(void);
	~CSingleToneEntry(void);
	static CSingleToneEntry * CSingleToneEntry::Instance(void);
	bool QueryStInstance(const GUID & guid, CSingleToneBase * & obj);
	bool RegisterStInstance(const GUID & guid, CSingleToneBase * obj);


public:
	template <class T>
	static void GetInstance(T * & obj)
	{
		CSingleToneEntry * entry = Instance();
		JCASSERT(obj == NULL);
		CSingleToneBase * ptr = NULL;

		entry->QueryStInstance(T::Guid(), ptr);
		if (ptr == NULL)
		{
			ptr = static_cast<CSingleToneBase *>(new T);
			entry->RegisterStInstance(T::Guid(), ptr);
		}
		obj = dynamic_cast<T*>(ptr);
	}

protected:
	CSingleToneManager *	m_base;
	UINT m_entry_id;
};