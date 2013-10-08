#pragma once
#pragma warning (disable : 4250)	//禁止多重继承警告

//#ifdef WIN32		// for win32
//#define		LockedIncrement(ii)		InterlockedIncrement(ii)
//#define 	LockedDecrement(ii)		InterlockedDecrement(ii)
//#else				// for linux
//#define		LockedIncrement(ii)		(++ii)
//#define 	LockedDecrement(ii)		(--ii)
//#endif

class IJCInterface
{
public:
	inline virtual void AddRef()		=0;
	inline virtual void Release(void)	=0;
	virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr) =0;
protected:
	virtual ~IJCInterface() {};
};

class CJCInterfaceBase : virtual public IJCInterface
{
protected:
	CJCInterfaceBase(void) : m_ref(1) {};
	virtual ~CJCInterfaceBase(void);

#ifdef _DEBUG
protected:
#else
private:
#endif

#ifdef WIN32
	mutable __declspec(align(4))	long	m_ref;
#else
	mutable long	m_ref;
#endif

public:
	inline virtual void AddRef() 
	{
		LockedIncrement(m_ref);
	}

	inline virtual void Release(void)		
	{
		if (LockedDecrement(m_ref) == 0) delete this;
	}
	
	virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr) 
	{
		if_ptr=NULL;
		return false;
	};
};

class CJCInterfaceStatic : virtual public IJCInterface
{
protected:
	CJCInterfaceStatic(void) {};
	virtual 
		~CJCInterfaceStatic(void) {};
public:
	inline virtual void AddRef() {};
	inline virtual void Release(void)	{};
	virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr) 
	{
		if_ptr=NULL;
		return false;
	};
};


///////////////////////////////////////////////////////////////////////////////
// -- Factories
template <class CLASSNAME>
class Factory0
{
public:
	static void Create(CLASSNAME * & ptr) { ptr = new CLASSNAME; }
};

template <typename TYPE1, class CLASSNAME>
class Factory1
{
public:
	static void Create(const TYPE1 & p1, CLASSNAME * & ptr) { ptr = new CLASSNAME(p1); }
};

template <typename TYPE1, typename TYPE2, class CLASSNAME>
class Factory2
{
public:
	static void Create(const TYPE1 & p1, const TYPE2 & p2, CLASSNAME * & ptr) 
	{ 
		ptr = new CLASSNAME(p1, p2); 
	}
};

template <typename TYPE1, typename TYPE2, typename TYPE3, class CLASSNAME>
class Factory3
{
public:
	static void Create(const TYPE1 & p1, const TYPE2 & p2, const TYPE3 & p3, CLASSNAME * & ptr) 
	{ 
		JCASSERT(NULL == ptr);
		ptr = new CLASSNAME(p1, p2, p3); 
	}
};
