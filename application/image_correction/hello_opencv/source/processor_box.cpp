#include "stdafx.h"

#include "application.h"
#include <opencv2/highgui/highgui.hpp>

LOCAL_LOGGER_ENABLE(_T("box"), LOGGER_LEVEL_DEBUGINFO);

///////////////////////////////////////////////////////////////////////////////
//--
LOG_CLASS_SIZE(CTrackBar);

CTrackBar::CTrackBar(void)
{
}

void CTrackBar::Create(UINT id, const char * bar_name, CProcessorBox * box, int default_val, int max)
{
	m_name = bar_name;
	m_box = box;	JCASSERT(m_box);
	m_id = id;
	m_val = default_val;
	const char * win_name = m_box->GetNameA();
	cv::createTrackbar(m_name, win_name, &m_val, max, StaticCallBack, (void*)this);
}

void CTrackBar::OnTrackBarUpdated(int val)
{
	JCASSERT(m_box);
	m_box->OnTrackBarUpdated(m_id, m_name, val);
}

void CTrackBar::StaticCallBack(int val, void * context)
{
	CTrackBar * track_bar = reinterpret_cast<CTrackBar *>(context);
	track_bar->OnTrackBarUpdated(val);
}

///////////////////////////////////////////////////////////////////////////////
//--
LOG_CLASS_SIZE(CProcessorBox);

CProcessorBox::CProcessorBox(JCSIZE index, const CJCStringT & name, IImageProcessor * proc)
: m_index(index), m_box_name(name), m_processor(proc)
{
	JCASSERT(proc);
}

CProcessorBox::~CProcessorBox(void)
{
	std::vector<CTrackBar *>::iterator it = m_track_bar_list.begin();
	std::vector<CTrackBar *>::iterator endit = m_track_bar_list.end();
	for ( ; it!=endit; ++it)
	{
		CTrackBar * track = *it; JCASSERT(track);
		delete track;
	}
	m_processor->Release();
	cv::destroyWindow(m_name);
	// <TODO>
}

UINT CProcessorBox::RegistTrackBar(const char * name, int def_val, int max_val)
{
	CTrackBar * track_bar = new CTrackBar();	JCASSERT(track_bar);
	m_track_bar_list.push_back(track_bar);
	UINT id = m_track_bar_list.size();
	track_bar->Create(id, name, this, def_val, max_val);
	
	return id;
}

void CProcessorBox::OnTrackBarUpdated(UINT id, const CJCStringA & name, int val)
{
	JCASSERT(m_processor);
	bool updated = m_processor->OnParameterUpdated(name.c_str(), val);
	if (updated) OnUpdateBox();
}

void CProcessorBox::OnInitialize(void)
{
	// create window
	stdext::UnicodeToUtf8(m_name, m_box_name.c_str());
	cv::namedWindow(m_name);
	// initialize processor
	JCASSERT(m_processor);
	m_processor->OnInitialize();
}

bool CProcessorBox::OnUpdateBox(void)
{
	JCASSERT(m_processor);
	bool updated = false;
	try
	{
		updated = m_processor->OnCalculate();
		if (updated)
		{
			cv::Mat img;
			m_processor->GetOutputImage(img);
			cv::imshow(m_name, img);
		}
	}
	catch (const std::exception & ex)
	{
		std::cout << "[" << m_name << "] error:" << ex.what();
	}

	return updated;
}

void CProcessorBox::GetProcessor(IImageProcessor * & proc)
{
	proc = m_processor;	JCASSERT(proc);
	proc->AddRef();
}
