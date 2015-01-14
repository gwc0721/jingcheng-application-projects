#include "stdafx.h"
#include <stdext.h>
#include "driver_factory.h"
#include "DriverImageFile.h"
#include "page_mode_driver.h"

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

bool CDriverFactory::CreateDriver(const CJCStringT &driver_name, jcparam::IValue *param, IImage *&driver)
{
	LOG_STACK_TRACE();
	JCASSERT(driver == NULL);

	CJCStringT file_name;
	CJCStringT config_file_name;
	ULONG64 file_size;
	if (param)
	{
		jcparam::GetSubVal(param, _T("FILENAME"), file_name);
		LOG_DEBUG(_T("got file name: %s"), file_name.c_str());

		jcparam::GetSubVal(param, _T("FILESIZE"), file_size);
		LOG_DEBUG(_T("got file size: %I64d secs"), file_size);

		jcparam::GetSubVal(param, _T("CONFIG"), config_file_name);
		LOG_DEBUG(_T("got config file name: %s"), config_file_name.c_str());

	}

	if (driver_name == _T("vendor_test"))
	{
		if (!config_file_name.empty())
			driver = static_cast<IImage*>(new CDriverImageFile(config_file_name));
		else
			driver = static_cast<IImage*>(new CDriverImageFile(file_name, file_size));
	}
	else if (driver_name == _T("page_mode"))
	{
		driver = new CPageModeDriver(config_file_name);
	}

	return true;
}

UINT CDriverFactory::GetRevision() const
{
	return 1;
}
