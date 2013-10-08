#include "stdafx.h"

#include "../include/value.h"
#include "../include/huge_table.h"
using namespace jcparam;

CHugeTable::CHugeTable(JCSIZE row_struct_size, JCSIZE rows)
{
}

CHugeTable::~CHugeTable(void)
{
}


bool CHugeTable::QueryInterface(const char * if_name, IJCInterface * &if_ptr)
{
	JCASSERT(NULL == if_ptr);
	bool br = false;
	if ( FastCmpA(IF_NAME_VALUE_FORMAT, if_name) )
	{
		if_ptr = static_cast<IJCInterface*>(this);
		if_ptr->AddRef();
		br = true;
	}
	else br = __super::QueryInterface(if_name, if_ptr);
	return br;
}