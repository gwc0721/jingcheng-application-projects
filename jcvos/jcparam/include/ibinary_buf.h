#pragma once

#include "ivalue.h"

namespace jcparam
{

	class IBinaryBuffer
		: public jcparam::IVisibleValue
	{
	public:
		// 获取从offset开始，len长度的指针。locked返回实际lock的长度，可能小于len
		// 都已sector为单位。
		//virtual BYTE * PartialLock(JCSIZE start, JCSIZE secs, JCSIZE & locked) = 0;
		virtual BYTE * Lock(void) = 0;
		virtual void Unlock(BYTE * ptr) = 0;

		// return size in byte
		virtual JCSIZE GetSize(void) const = 0;
	};

};