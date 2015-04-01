#include "CoreMntTest.h"

LOCAL_LOGGER_ENABLE(_T("test_port"), LOGGER_LEVEL_DEBUGINFO);

DWORD WINAPI CCoreMntTestApp::StaticStartTestPort(LPVOID param)
{
	JCASSERT(param);
	CCoreMntTestApp* app = reinterpret_cast<CCoreMntTestApp*>(param);
	//app->RunTestPort(m_test_port);
	return 0;
}

#define MAX_LINE_BUF 1024

void CCoreMntTestApp::RunTestPort(ITestAuditPort * test_port)
{
	TCHAR line_buf[MAX_LINE_BUF];
	jcparam::CArguDefList argu_parser;	// parser for process command line

	while (1)
	{
		_tprintf(_T(">"));
		// one line command parse
		_getts_s(line_buf, MAX_LINE_BUF-1);
		JCSIZE len = _tcslen(line_buf);
		JCASSERT(len < MAX_LINE_BUF -1);
		if (len <= 0) continue;

		LPTSTR param_start = _tcschr(line_buf, _T(' '));
		param_start[0] = 0;
		param_start ++;

		jcparam::CArguSet argu_set;
		jcparam::CCmdLineParser::ParseCommandLine(argu_parser, param_start, argu_set);
		
		test_port->SetStatus(line_buf, static_cast<jcparam::IValue*>(&argu_set));
	}
}
