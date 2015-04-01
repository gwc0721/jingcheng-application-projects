#pragma once
#include "../../CoreMntLib/include/mntImage.h"

#define STATUS_SUCCESS		0
#define STATUS_INVALID_PARAMETER         (0xC000000DL)    // winnt


///////////////////////////////////////////////////////////////////////////////
//-- factory

class CDriverFactory : public IDriverFactory
{
public:
	CDriverFactory(void);
	~CDriverFactory(void);

public:
	inline virtual void AddRef()	{};			// static object
	inline virtual void Release(void)	{};		// static object
	virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr){return false;};

	//virtual bool	CreateDriver(const CJCStringT & driver_name, jcparam::IValue * param, IImage * & driver);
	virtual bool	CreateDriver(const CJCStringT & driver_name, const CJCStringT & config, IImage * & driver);
	virtual UINT	GetRevision(void) const;
};
