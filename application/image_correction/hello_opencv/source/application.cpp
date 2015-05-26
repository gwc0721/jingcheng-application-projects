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
	//ARGU_DEF_ITEM(_T("dummy"),		_T('p'), CJCStringT,	m_dummy,		_T("load user mode driver.") )
	ARGU_DEF_ITEM(_T("review"),		_T('r'), int,	m_review_scale,		_T("scale for review.") )
	ARGU_DEF_ITEM(_T("parameter"),	_T('p'), CJCStringT,	m_param_filename,	_T("load parameter from file.") )
	ARGU_DEF_ITEM(_T("train"),		_T('t'), CJCStringT,	m_train_list,	_T("do train using a file list.") )
	//ARGU_DEF_ITEM(_T("config"),		_T('g'), CJCStringT,	m_config,		_T("configuration file name for driver.") )
END_ARGU_DEF_TABLE()


///////////////////////////////////////////////////////////////////////////////
//--

CHelloOpenCvApp::CHelloOpenCvApp(void)
: m_result_proc(NULL)
, m_active_box(NULL)
, m_review_scale(1)
{
}

void CHelloOpenCvApp::UpdateBox(JCSIZE start)
{
	JCSIZE ii = 0;
	for (JCSIZE ii = start; ii < m_processor_list.size(); ++ii)
	{
		CProcessorBox * box = m_processor_list.at(ii);		JCASSERT(box);
		box->Recalculate();
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

IImageProcessor * CHelloOpenCvApp::AddProcessor(const CJCStringT & type, const CJCStringT & name, BOX_PROPERTY prop, int src_num, ...)
{
	IImageProcessor * proc = NULL;
	CreateProcessor(type, proc);	JCASSERT(proc);
	va_list argptr;
	va_start(argptr, src_num);

	for (int ii=0; ii<src_num; ++ii)
	{
		//int input_id = va_arg(argptr, int);
		const TCHAR * src_name = va_arg(argptr, const TCHAR *);
		if (src_name && (src_name[0] != 0) )	Connect(proc, ii, src_name);
	}
	AddProcessor(name, proc, prop);
	return proc;
}

void CHelloOpenCvApp::AddProcessor(const CJCStringT & name,IImageProcessor * proc, BOX_PROPERTY prop)
{
	JCSIZE boxes = m_processor_list.size();
	// create box
	CProcessorBox * box = new CProcessorBox(boxes, name, proc, prop);
	// relate box to processor
	proc->SetBox(box);
	box->SetContainer(static_cast<IBoxContainer*>(this));
	box->SetReviewScale(m_review_scale);
	// add box to list
	// <PS> 注意：box插入list的顺序敏感。目前在代码中直接添加processor，手动确保顺序正确。
	//  将来需要自动编译计算有向无环图的顺序
	m_processor_list.push_back(box);
}



int CHelloOpenCvApp::Initialize(void)
{
	IImageProcessor * proc = NULL;
	CreateProcessor(_T("source"), proc);
	stdext::auto_interface<jcparam::IValue> val_fn( jcparam::CTypedValue<CJCStringT>::Create(m_src_fn) );
	proc->SetProperty(_T("file_name"), val_fn);
	AddProcessor(_T("source"), proc, BP_NONE);

#if 0		// 预处理
	// create processor
	AddProcessor(_T("pre-proces"), _T("pre_proc"), BP_SHOW_WND, 1, _T("source"));
	AddProcessor(_T("bright"), _T("bright"), BP_SHOW_WND, 1, _T("pre_proc"));
	AddProcessor(_T("mophology"), _T("mopho_1"), BP_SHOW_WND, 1, _T("bright"));
	AddProcessor(_T("thresh"), _T("thresh"), BP_SHOW_WND, 1, _T("mopho_1"));
#endif

#if 0		// 调试用处理器
	//AddProcessor(_T("roi"), _T("roi_mopho"), BP_SHOW_WND, 1, _T("mopho_1"));
	//// connect pre_proc to input 0 of canny processor
	//AddProcessor(_T("canny"), _T("canny"), 1, 0, _T("mopho_1"));
	//AddProcessor(_T("houghline"), _T("hough"), 2, 0, _T("canny"), 1, _T("pre_proc"));
	//AddProcessor(_T("rotation"), _T("rotation"), 2, 0, _T("pre_proc"), 1, _T("hough"));
	AddProcessor(_T("contour"), _T("contour"), BP_SHOW_WND, 2,_T("thresh"), _T("mopho_1"));
	//AddProcessor(_T("contour_match"), _T("matches"), BP_SHOW_WND, 2,_T("thresh"), _T("roi_mopho"));
	AddProcessor(_T("match"), _T("matches"), BP_SHOW_WND, 1,_T("mopho_1"));
	AddProcessor(_T("thresh"), _T("th_matches"), BP_SHOW_WND, 1,_T("matches"));

	//AddProcessor(_T("hist"), _T("hist"), 1, 0, _T("mopho_1"));
	//AddProcessor(_T("rotate_clip"), _T("rote-clip"), BP_SHOW_WND, 2, _T("pre_proc"), _T("contour"));
#endif

#if 0		// OCR
	AddProcessor(_T("ferri"), _T("ferri"), BP_SHOW_WND, 1, _T("mopho_1"));
#endif

#if 1		// 倾斜校正与剪切
	proc = AddProcessor(_T("correction"), _T("corr"), BP_SHOW_WND, 1, _T("source"));
	//proc->SetProperty(_T("out-fn"), val_fn);
	m_result_proc = proc;
	m_result_proc->AddRef();
	AddProcessor(_T("barcode"), _T("barcode"), BP_SHOW_WND, 1, _T("corr"));
#endif

#if 0
	AddProcessor(_T("split"), _T("split"), BP_SHOW_WND, 1, _T("source"));
#endif
	// load parameter from file
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
	std::wcout << _T("saving parameter to ") << fn;
	CJCStringA str_fn;
	stdext::UnicodeToUtf8(str_fn, fn.c_str());
	cv::FileStorage fs(str_fn, cv::FileStorage::WRITE);
	CProcessorBox * box;
	ErgodicProcessorBox(&CHelloOpenCvApp::SaveProcessor, (void*)(&fs), box);
	std::wcout << _T("done\n");
	return false;
}

bool CHelloOpenCvApp::LoadProcessor(CProcessorBox * proc, void * context)
{
	cv::FileStorage * pfs = reinterpret_cast<cv::FileStorage*> (context);
	CJCStringA proc_name = proc->GetNameA();
	cv::FileNode node = (*pfs)[proc_name];
	LOG_DEBUG(_T("read node '%S', size=%d"), proc_name.c_str(), node.size());
	if (node.empty()) return true;

	stdext::auto_interface<IImageProcessor> iproc;
	proc->GetProcessor(iproc);
	iproc->read(node);
	return true;
}

bool CHelloOpenCvApp::LoadParameterFrom(const CJCStringT & fn)
{
	CJCStringA str_fn;
	stdext::UnicodeToUtf8(str_fn, fn.c_str());
	cv::FileStorage fs;
	std::wcout << _T("loading parameter from ") << fn << _T(" ... ");
	fs.open(str_fn, cv::FileStorage::READ);
	CProcessorBox * box;
	ErgodicProcessorBox(&CHelloOpenCvApp::LoadProcessor, (void*)(&fs), box);
	std::wcout << _T("done\n");
	return false;
}

void CHelloOpenCvApp::SaveResults(void)
{
	// 产生文件名
	//if ( m_dst_fn.empty() ) m_dst_fn = m_src_fn;
	//JCSIZE cc = m_dst_fn.find_last_of(_T('.'));
	//TCHAR fn[MAX_PATH];
	//wmemset(fn, 0, MAX_PATH);
	//m_dst_fn.copy(fn, cc);

	CJCStringA fn_a;
	//stdext::UnicodeToUtf8(fn_a, fn, cc);

	// 输入文件名
	int cur_ch = m_result_proc->GetActiveChannel();
	cv::Mat img;
	m_result_proc->GetOutputImage(img);

	std::wcout << _T("file name to save image ") << cur_ch << _T(":") << std::endl;
	std::cin >> fn_a;
	cv::imwrite(fn_a, img);
	std::wcout << _T("done\n");

	

	//int result_num = m_result_proc->GetChannelNum();
	//for (int ii = 0; ii < result_num; ++ii)
	//{
		//char str_fn[MAX_PATH];
		//sprintf_s(str_fn, ("%s_%02d.jpg"), fn_a.c_str(), ii);
		//
		//std::cout << "saving image: " << str_fn << " ...";
		//cv::Mat img;
		//m_result_proc->GetOutputImage(ii, img);
		//cv::imwrite(str_fn, img);
		//std::cout << "done\n";
	//}
}

void CHelloOpenCvApp::SetActiveBox(IProcessorBox * box)
{
	JCASSERT(box);
	m_active_box = box;
	std::cout << "select active box: " << box->GetNameA() << std::endl;
}


int CHelloOpenCvApp::Run(void)
{
	std::cout	<< ("\t<ENTER>\t update all windows\n")
				<< ("\t'p'\t save parameters\n")
				<< ("\t'l'\t load parameters\n")
				<< ("\t<SPACE>\t exit and save results\n")
				<< ("\t<ESC>\t exit without save\n")
				<< ("\t's'\t save data\n");

	// initialize all boxes
	PROCESSOR_BOX_LIST::iterator it = m_processor_list.begin();
	PROCESSOR_BOX_LIST::iterator endit = m_processor_list.end();
	for (; it != endit; ++it)
	{
		CProcessorBox * box = *it;
		box->OnInitialize();
	}

	if ( !m_param_filename.empty() ) LoadParameterFrom(m_param_filename);

	UpdateBox(0);
	// running
	while (1)
	{
		int key_code = cv::waitKey();
		LOG_DEBUG(_T("on key pressed: %d"), key_code);
		switch (key_code)
		{
		case 27:	// <ESC>
			return 0;

		case 13:	// <ENTER>
			// update all 
			UpdateBox(0);
			break;

		case 'p':	{// <s> for save paramter
			CJCStringT fn;
			std::wcout << _T("parameter file name:");
			std::wcin >> fn;
			SaveParameterTo(fn);
			break;		}

		case 'l':	{
			CJCStringT fn;
			std::wcout << _T("parameter file name:");
			std::wcin >> fn;
			LoadParameterFrom(fn);
			break;		}

		case ' ':	// <SPACE>
			UpdateBox(0);
			SaveResults();
			//return 0;
			break;

		default:
			if (m_active_box)	m_active_box->OnKeyPressed(key_code);
		}
	}
	return 0;
}

void CHelloOpenCvApp::CleanUp(void)
{
	if (m_result_proc) m_result_proc->Release();
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