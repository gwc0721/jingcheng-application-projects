#include "stdafx.h"

#include "plugin_manager.h"
#include "plugin_debug.h"
#include "application.h"

#include "ata_trace.h"
#include "parser_bus_doctor.h"

LOCAL_LOGGER_ENABLE(_T("pi_trace"), LOGGER_LEVEL_WARNING);


///////////////////////////////////////////////////////////////////////////////
// -- plugin
CJCStringT CPluginBase2<CPluginTrace>::m_name(_T("trace"));

FEATURE_LIST CPluginBase2<CPluginTrace>::m_feature_list(
	FEATURE_LIST::RULE()
	(new CTypedFtInfo< CPluginTrace::ParserTrace::_BASE >( _T("load trace from file") ) ) 
	(new CTypedFtInfo< CPluginTrace::BusHound::_BASE >( _T("parse bushound's log") ) )
	(new CTypedFtInfo< CPluginTrace::BusDoctor::_BASE_TYPE >( _T("parse bus doctor's log") ) ) 
	(new CTypedFtInfo< CPluginTrace::VenderCmd::_BASE >( _T("parse smi vender command") ) ) 
	(new CTypedFtInfo< CPluginTrace::LeCory::_BASE_TYPE >( _T("parse lecory sata suite") ) ) 
	(new CTypedFtInfo< CPluginTrace::LeCoryFis::_BASE_TYPE >( _T("parse lecory sata suite") ) ) 
	//(new CTypedFtInfo< CPluginTrace::Connect::_BASE>(_T("connect to simulator device") ) )
	);

CPluginTrace::CPluginTrace(void)
{
};

CPluginTrace::~CPluginTrace(void)
{
}
