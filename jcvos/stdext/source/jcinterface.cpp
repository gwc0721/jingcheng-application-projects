#define LOGGER_LEVEL LOGGER_LEVEL_DEBUGINFO

//#ifdef WIN32
//#include <windows.h>
//#endif

#include "../include/comm_define.h"
#include "../include/jcinterface.h"
#include "../include/jclogger_base.h"


LOCAL_LOGGER_ENABLE(_T("Assertion"), LOGGER_LEVEL_DEBUGINFO);

LOG_CLASS_SIZE(CJCInterfaceBase)

CJCInterfaceBase::~CJCInterfaceBase(void)
{
#ifdef _DEBUG
	JCASSERT(0 == m_ref || 1 == m_ref);
#endif
}
