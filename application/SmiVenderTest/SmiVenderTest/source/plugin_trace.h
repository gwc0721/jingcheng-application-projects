#pragma once

#include <jcparam.h>
#include <script_engine.h>
//#include "feature_base.h"

class CPluginTrace :
	virtual public jcscript::IPlugin,
	public CPluginBase2<CPluginTrace>
{
public:
	CPluginTrace(void);
	virtual ~CPluginTrace(void);

public:
	virtual bool Reset(void) {return true;} 
	
public:
	class ParserTrace;
	class BusHound;
	class BusDoctor;
	class VenderCmd;
	class LeCory;
	class LeCoryFis;
};

