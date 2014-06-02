#include "stdafx.h"
#include <stdext.h>

LOCAL_LOGGER_ENABLE(_T("test.stdext"), LOGGER_LEVEL_DEBUGINFO);

#define VIRTUAL_CLASS

#ifdef VIRTUAL_CLASS
#define _VIRTUAL_	virtual
#else
#define _VIRTUAL_
#endif

// 定义interface
class ITestInterface
{
public:
	inline virtual void AddRef()		=0;
	inline virtual void Release(void)	=0;
	//virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr) =0;
};

// 1级派生接口
class IVehicle : _VIRTUAL_ public ITestInterface
{
public:
	virtual void Drive(void)	 = 0;
};

// 2级派生接口
class ICar : /*_VIRTUAL_*/ public IVehicle
{
public:
	virtual void Run(void) = 0;
};


// 1级派生接口 2
class IPlane : _VIRTUAL_ public ITestInterface
{
public:
	virtual void Fly(void)	 = 0;
};

// 实现1
class CIf : _VIRTUAL_ public ITestInterface
{
protected:
	CIf(void) {m_ref=1;};
	 ~CIf(void) {};
public:
	static CIf * Create(void)	{
		return new CIf;
	}
	inline  void AddRef()		{
		m_ref++;
		LOG_DEBUG(_T("AddRef, ref=%d"), m_ref );
	};
	inline  void Release(void)	{
		m_ref--; 
		LOG_DEBUG(_T("Release, ref=%d"), m_ref);
		if (m_ref==0) delete this;
	}
protected:
	int m_ref;
};

template <class BASE>
class CTypedIf : public BASE
{
protected:
	CTypedIf(void) {m_ref=1;};
	virtual ~CTypedIf(void) {};
public:
	static CTypedIf<BASE> * Create(void)	{
		return new CTypedIf<BASE>;
	}
	inline virtual void AddRef()		{
		m_ref++;
		LOG_DEBUG(_T("AddRef, ref=%d"), m_ref );
	};
	inline virtual void Release(void)	{
		m_ref--; 
		LOG_DEBUG(_T("Release, ref=%d"), m_ref);
		if (m_ref==0) delete this;
	}
protected:
	int m_ref;
};


class CCar 
	:/* _VIRTUAL_*/ public CIf
	, /*_VIRTUAL_*/ public ICar
//#ifdef VIRTUAL_CLASS
//	, 
//#endif
{
protected:
	CCar(void) : m_speed (100)
		//, m_ref=1
	{LOG_STACK_TRACE();}
	virtual ~CCar(void) {
		LOG_DEBUG(_T("distruct"));
	};

public:
	friend bool CreateCar(ICar * &car);

public:
	virtual void Drive(void)	{LOG_STACK_TRACE();};
	virtual void Run(void) {};

	//inline virtual void AddRef()		{CIf::AddRef();}
	//inline virtual void Release()		{CIf::Release();}

//#ifndef VIRTUAL_CLASS
#if 0
	inline virtual void AddRef()		{
		m_ref++;
		LOG_DEBUG(_T("AddRef, ref=%d"), m_ref );
	};
	inline virtual void Release(void)	{
		m_ref--; 
		LOG_DEBUG(_T("Release, ref=%d"), m_ref);
		if (m_ref==0) delete this;
	}
protected:
	//int m_ref;
#endif

protected:
	int m_speed;
};


template <class BASE>
class CVehicle1 : public CTypedIf<BASE>
{
public:
	virtual void Drive(void) {
		LOG_STACK_TRACE();
	}

};

class CCarBase : public ICar
{
protected:
	CCarBase(void) {m_speed = 100;};
	virtual ~CCarBase(void) {
		LOG_DEBUG(_T("distruct"));
	};

public:
	//virtual void Drive(void)	{};
	virtual void Run(void) {};

protected:
	int m_speed;
};

//typedef CTypedIf<CCarBase>	CCar;

//typedef CVehicle1<CCarBase> CCar;

bool CreateCar(ICar * & car)
{
	car = new CCar;
	return true;
}

class CFlyCarBase : public ICar, public IPlane
{
protected:
	CFlyCarBase(void) {m_hight = 200;}
	virtual ~CFlyCarBase(void) {}

public:
	
protected:
	int m_hight;
};



void interface_test(void)
{
#ifdef VIRTUAL_CLASS
	LOG_DEBUG(_T("defined virtual class"));
#else
	LOG_DEBUG(_T("no virtual class"));
#endif
	LOG_CLASS_SIZE(ITestInterface);
	LOG_CLASS_SIZE(IVehicle);
	LOG_CLASS_SIZE(ICar);
	LOG_CLASS_SIZE(IPlane);
	LOG_CLASS_SIZE(CIf);
	LOG_CLASS_SIZE(CCar);

	CIf * cif = CIf::Create();

	//CCar * car = CCar::CreateCar();
	ICar * car = NULL;
	CreateCar(car);
	ITestInterface * ifcar = car;
	car->Drive();

	cif->Release();
	//memset(cif, 0xAA, sizeof(CIf) );
	ifcar->Release();
	//memset(car, 0xAA, sizeof(CCar) );
}