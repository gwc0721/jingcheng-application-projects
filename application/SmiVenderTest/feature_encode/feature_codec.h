#pragma once

#include <script_engine.h>

#define  RD2SYMB(rd)	(rd?_T('-'):_T('+'))

extern "C"
{
	void feature_codec_register(jcscript::IPluginContainer * plugin_container);
	WORD Bit8To10(BYTE val, WORD & rd);
};


