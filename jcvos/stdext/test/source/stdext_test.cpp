// stdext_test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdext.h>


// Test for jclogger
LOCAL_LOGGER_ENABLE(_T("test_stdext"), LOGGER_LEVEL_DEBUGINFO);

LOGGER_TO_FILE(0, _T("test_stdext.log"), 
		CJCLogger::COL_TIME_STAMP |
		CJCLogger::COL_FUNCTION_NAME | 
		CJCLogger::COL_REAL_TIME, 0);

// Test for LOG_CLASS_SIZE
class jclogger_class_size_test
{
	int m1;
	UINT64 m2;
	char m3[10]; 
};

LOG_CLASS_SIZE(jclogger_class_size_test);

void test_logger_stack()
{
	LOG_STACK_TRACE();
}

void test_jclogger()
{
	LOG_STACK_TRACE();
	LOG_DEBUG(_T("Testing jclogger"));

	// Test for log performance
	for (int ii=0; ii<32; ++ii)
	{
		test_logger_stack();
	}

	for (int ii=0; ii<32; ++ii)
	{
		LOG_DEBUG(_T("log performance"));
	}
}

// Test secure funtions
void test_secure_functions()
{
	LOG_STACK_TRACE();

	TCHAR str[256];
	TCHAR * str_end;

	LPCTSTR str01 = _T("123.456");
	double x01 = stdext::jc_str2f(str01);
	LOG_DEBUG(_T("Test function stdext::jc_str2f() : src = %ls, dst = %e"), str01, x01);


	LPCTSTR str02 = _T("1234567890");
	INT64 x02 = stdext::jc_str2ll(str02);
	LOG_DEBUG(_T("Test function stdext::jc_str2ll() : src = %ls, dst = %lld"), str02, x02);

	LPCTSTR str03 = _T("123456789");	
	UINT x03 = stdext::jc_str2ul(str03, &str_end);
	LOG_DEBUG(_T("Test function stdext::jc_str2l() : src = %ls, dst = %d"), str03, x03);

	LPCTSTR str04 = _T("abcdef12");			
	x03 = stdext::jc_str2ul(str04, &str_end, 16);
	LOG_DEBUG(_T("Test function stdext::jc_str2l() : src = %ls, dst = %x"), str04, x03);

	INT64 x05 = 0x1234567890abcdef;
	stdext::jc_int2str(x05, str);
	LOG_DEBUG(_T("Test function stdext::jc_int2str(10) : src = %lld, dst = %ls"), x05, str);

	stdext::jc_int2str(x05, str, 16);
	LOG_DEBUG(_T("Test function stdext::jc_int2str(16) : src = %llx, dst = %ls"), x05, str);
}



int _tmain(int argc, TCHAR* argv[])
{
	LOG_STACK_TRACE();
	
	test_jclogger();
	test_secure_functions();

	return 0;
}

/*
int main(int argc, char * argv[])
{
	_tmain(0, NULL);
}
*/
