#include "stdafx.h"

#include "application.h"
#include "processor_factory.h"
#include <opencv2/highgui/highgui.hpp>

#include <vld.h>
#include <stdext.h>

LOCAL_LOGGER_ENABLE(_T("hello_opencv"), LOGGER_LEVEL_DEBUGINFO);

const TCHAR CHelloOpenCvApp::LOG_CONFIG_FN[] = _T("driver_test.cfg");
typedef jcapp::CJCApp<CHelloOpenCvApp>	CApplication;
#define _class_name_	CApplication
CApplication _app;

BEGIN_ARGU_DEF_TABLE()
	ARGU_DEF_ITEM(_T("dummy"),		_T('p'), CJCStringT,	m_dummy,		_T("load user mode driver.") )
	//ARGU_DEF_ITEM(_T("config"),		_T('g'), CJCStringT,	m_config,		_T("configuration file name for driver.") )
END_ARGU_DEF_TABLE()


///////////////////////////////////////////////////////////////////////////////
//--

CHelloOpenCvApp::CHelloOpenCvApp(void)
{
}

void CHelloOpenCvApp::AddProcessor(const CJCStringT & name,IImageProcessor * proc)
{
	JCSIZE boxes = m_processor_list.size();
	// create box
	CProcessorBox * box = new CProcessorBox(boxes, name, proc);
	// relate box to processor
	proc->SetBox(box);
	// add box to list
	// <PS> 注意：box插入list的顺序敏感。目前在代码中直接添加processor，手动确保顺序正确。
	//  将来需要自动编译计算有向无环图的顺序
	m_processor_list.push_back(box);
}

void CHelloOpenCvApp::UpdateBox(JCSIZE start)
{
	JCSIZE ii = 0;
	for (JCSIZE ii = start; ii < m_processor_list.size(); ++ii)
	{
		CProcessorBox * box = m_processor_list.at(ii);		JCASSERT(box);
		box->OnUpdateBox();
	}
}


bool CHelloOpenCvApp::ErgodicProcessorBox(ERGODIC_CALLBACK call_back, void * context, CProcessorBox * &box)
{
	PROCESSOR_BOX_LIST::iterator it = m_processor_list.begin();
	PROCESSOR_BOX_LIST::iterator endit = m_processor_list.end();
	for (; it != endit; ++it)
	{
		box = *it;	JCASSERT(box);
		bool run = (this->*call_back)(box, context);
		if (!run) return false;
	}
	return true;
}

bool CHelloOpenCvApp::FindBoxByName(CProcessorBox * proc, void * name)
{
	const TCHAR * str_name = reinterpret_cast<const TCHAR *>(name);
	CJCStringT proc_name;
	proc->GetNameT(proc_name);
	if (proc_name == (str_name) ) return false;
	return true;
}

void CHelloOpenCvApp::Connect(IImageProcessor * dst_proc, int input_id, const CJCStringT & src_name)
{
	// search for source proc box
	CProcessorBox * box = NULL;
	bool not_found = ErgodicProcessorBox(&CHelloOpenCvApp::FindBoxByName, (void*)(src_name.c_str()), box);
	if (not_found) THROW_ERROR(ERR_PARAMETER, _T("not found processor %s"), src_name.c_str());
	// get processor from box
	JCASSERT(box);
	IImageProcessor * proc = NULL;
	box->GetProcessor(proc);
	// connect
	dst_proc->ConnectTo(input_id, proc);
	proc->Release();
}

int CHelloOpenCvApp::Initialize(void)
{
	IImageProcessor * proc = NULL;
	// loop
		// create processor
		CreatePreProcessor(m_src_fn, proc);
		// connect processor
		// create box
		AddProcessor(_T("pre_proc"), proc);
		// ...
		proc = NULL;
		CreateProcessor(_T("mophology"), proc);
		//CreateProcessorMophology(proc);
		Connect(proc, 0, _T("pre_proc"));
		AddProcessor(_T("mopho_1"), proc);

		proc = NULL;
		CreateProcessor(_T("canny"), proc);
		//CreateCannyProcessor(proc);
		// connect pre_proc to input 0 of canny processor
		Connect(proc, 0, _T("mopho_1"));
		AddProcessor(_T("canny"), proc);

		proc = NULL;
		CreateProcessor(_T("houghline"), proc);
		Connect(proc, 0, _T("canny"));
		Connect(proc, 1, _T("pre_proc"));
		AddProcessor(_T("hough"), proc);

		proc = NULL;
		CreateProcessor(_T("rotation"), proc);
		Connect(proc, 0, _T("pre_proc"));
		Connect(proc, 1, _T("hough"));
		AddProcessor(_T("rotation"), proc);

		proc = NULL;
		CreateProcessor(_T("countour"), proc);
		Connect(proc, 0, _T("canny"));
		Connect(proc, 1, _T("pre_proc"));
		AddProcessor(_T("countour"), proc);

	// end loop
	return 1;
}

bool CHelloOpenCvApp::SaveProcessor(CProcessorBox * proc, void * context)
{
	cv::FileStorage * pfs = reinterpret_cast<cv::FileStorage*> (context);
	stdext::auto_interface<IImageProcessor> iproc;
	proc->GetProcessor(iproc);
	iproc->write(*pfs);
	return true;
}

bool CHelloOpenCvApp::SaveParameterTo(const CJCStringT & fn)
{
	CJCStringA str_fn;
	stdext::UnicodeToUtf8(str_fn, fn.c_str());
	cv::FileStorage fs(str_fn, cv::FileStorage::WRITE);
	CProcessorBox * box;
	ErgodicProcessorBox(&CHelloOpenCvApp::SaveProcessor, (void*)(&fs), box);
	return false;
}

int CHelloOpenCvApp::Run(void)
{
	// initialize all boxes
	PROCESSOR_BOX_LIST::iterator it = m_processor_list.begin();
	PROCESSOR_BOX_LIST::iterator endit = m_processor_list.end();
	for (; it != endit; ++it)
	{
		CProcessorBox * box = *it;
		box->OnInitialize();
	}

	UpdateBox(0);

	// running
	while (1)
	{
		int key_code = cv::waitKey();
		switch (key_code)
		{
		case 27:	// <ESC>
			return 0;
		case 13:	// <ENTER>
			// update all 
			UpdateBox(0);
			break;
		case 's':	{// <s> for save paramter
			SaveParameterTo(_T("testdata\\param.xml"));
			break;		}
		}

	}
	return 0;
}

void CHelloOpenCvApp::CleanUp(void)
{
	PROCESSOR_BOX_LIST::iterator it = m_processor_list.begin();
	PROCESSOR_BOX_LIST::iterator endit = m_processor_list.end();
	for (; it != endit; ++it)
	{
		CProcessorBox * box = *it;	JCASSERT(box);
		delete box;
	}
}


int _tmain(int argc, _TCHAR* argv[])
{
	return jcapp::local_main(argc, argv);
}