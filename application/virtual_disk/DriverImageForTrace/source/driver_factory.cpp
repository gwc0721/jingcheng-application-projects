#include "stdafx.h"
#include <stdext.h>
#include "driver_factory.h"
#include "DriverImage.h"

LOCAL_LOGGER_ENABLE(_T("factory"), LOGGER_LEVEL_ERROR);

CDriverFactory g_factory;

CDriverFactory::CDriverFactory(void)
{
	LOG_STACK_TRACE();
}

CDriverFactory::~CDriverFactory(void)
{
	LOG_STACK_TRACE();
}

bool CDriverFactory::CreateDriver(const CJCStringT &driver_name, const CJCStringT & config, IImage *&driver)
{
	LOG_STACK_TRACE();
	JCASSERT(driver == NULL);

	//CJCStringT file_name;
	//CJCStringT config_file_name;
	//ULONG64 file_size;

	driver = static_cast<IImage*>(new CDriverImageFile(config));

	return true;
}

UINT CDriverFactory::GetRevision() const
{
	return 1;
}
