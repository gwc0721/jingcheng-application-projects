#pragma once

#include <stdext.h>

// 词法定义
enum TOKEN_ID
{
	ID_WHITE_SPACE_ = 1,	// 空白
	ID_COMMA,				// 逗号
	ID_HEX,
	ID_DECIMAL,
	ID_NEW_LINE,
	ID_FLOAT,
	ID_OTHER,
};

class CTokenProperty
{
public:
	TOKEN_ID	m_id;
	//int			m_index;
	union
	{
		//CJCStringT	m_str;
		INT64		m_int;
		double		m_double;
	};
};

void LineParse(LPCTSTR &first, LPCTSTR last, CTokenProperty * prop);
