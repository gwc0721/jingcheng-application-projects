#include "stdafx.h"

#include "application.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
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

void CTrackBar::SetPosition(int val)
{
	cv::setTrackbarPos(m_name, m_box->GetNameA(), val);
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

CProcessorBox::CProcessorBox(JCSIZE index, const CJCStringT & name, IImageProcessor * proc, BOX_PROPERTY prop)
: m_index(index), m_box_name(name), m_processor(proc)
, m_property(prop), m_container(NULL), m_select_st(SS_NONE)
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
	if (m_property & BP_SHOW_WND)
	{
		CTrackBar * track_bar = new CTrackBar();	JCASSERT(track_bar);
		m_track_bar_list.push_back(track_bar);
		UINT id = m_track_bar_list.size();
		track_bar->Create(id, name, this, def_val, max_val);
		return id;
	}
	else return UINT_MAX;
	
}

void CProcessorBox::OnTrackBarUpdated(UINT id, const CJCStringA & name, int val)
{
	JCASSERT(m_processor);
	bool updated = m_processor->OnParameterUpdated(name.c_str(), val);
	if (updated) Recalculate();
}

void CProcessorBox::UpdateTrackBar(UINT id, int val)
{
	JCASSERT(id < m_track_bar_list.size());
	CTrackBar * track_bar = m_track_bar_list.at(id);
	track_bar->SetPosition(val);
}

void CProcessorBox::OnInitialize(void)
{
	// create window
	if (m_property & BP_SHOW_WND)
	{
		stdext::UnicodeToUtf8(m_name, m_box_name.c_str());
		cv::namedWindow(m_name);
	}
	//
	cv::setMouseCallback(m_name, StaticOnMouse, (void*)this);
	// initialize processor
	JCASSERT(m_processor);
	m_processor->OnInitialize();
}

void CProcessorBox::OnUpdateBox(void)
{
	if ( (m_property & BP_SHOW_WND) )
	{
		if (m_local_buf.empty() )return;
		cv::Mat img;
		if (m_select_st == SS_CHANGING || m_select_st == SS_SELECTED )
		{
			m_local_buf.copyTo(img);
			cv::rectangle(img, m_rect1, m_rect2, cv::Scalar(255, 0, 0), 2);
		}
		else img = m_local_buf;

		cv::imshow(m_name, img);
	}
}

void CProcessorBox::FillReviewBuf(cv::Mat & buf)
{
	cv::Mat img;
	m_processor->GetReviewImage(img);
	cv::resize(img, img, cv::Size(img.cols / m_review_scale, img.rows / m_review_scale));
	if (img.channels() < 3)	cv::cvtColor(img, buf, CV_GRAY2BGR);
	else					img.copyTo(buf);
	//img.convertTo(buf, CV_8UC3);
}

bool CProcessorBox::Recalculate(void)
{
	JCASSERT(m_processor);
	bool updated = false;
	try
	{
		updated = m_processor->OnCalculate();
		FillReviewBuf(m_local_buf);
		if (updated)	OnUpdateBox();
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

void CProcessorBox::StaticOnMouse( int mouse_event, int x, int y, int z, void* context)
{
	CProcessorBox * box = reinterpret_cast<CProcessorBox*>(context);	JCASSERT(box);
	box->OnMouse(mouse_event, x, y, z);
}

void CProcessorBox::OnMouse(int mouse_event, int x, int y, int z)
{
//	LOG_STACK_TRACE();
	LOG_DEBUG_(1, _T("mouse event on win:%s, event=%d, at (%d,%d), %d"), m_box_name.c_str(), mouse_event, x, y, z );

	switch (mouse_event)
	{
	case CV_EVENT_LBUTTONDOWN:
		if (m_container) m_container->SetActiveBox(static_cast<IProcessorBox*>(this));
		if ( m_select_st == SS_NONE)
		{	// 开始选取区域
			m_rect1.x = x, m_rect1.y = y;
			m_rect2 = m_rect1;
			m_select_st = SS_STARTED;
			OnUpdateBox();
		}
		break;

	case CV_EVENT_LBUTTONUP:
		if ( m_select_st == SS_CHANGING)
		{	// 结束选取
			m_rect2.x = x, m_rect2.y = y;
			m_select_st = SS_SELECTED;
			OnUpdateBox();
			JCASSERT(m_processor);
			m_processor->OnRactSelected(cv::Rect(m_rect1 * m_review_scale, m_rect2 * m_review_scale));
		}
		else
		{
			m_select_st = SS_NONE;	// 取消选取
			OnUpdateBox();
			m_processor->OnRactSelected(cv::Rect(0, 0, 0, 0));
		}
		break;

	case CV_EVENT_MOUSEMOVE:
		if ( (m_select_st == SS_STARTED) || (m_select_st == SS_CHANGING) )
		{
			m_rect2.x = x, m_rect2.y = y;
			OnUpdateBox();
			m_select_st = SS_CHANGING;
		}
		break;

	case CV_EVENT_RBUTTONUP:
		m_select_st = SS_NONE;
		OnUpdateBox();
		JCASSERT(m_processor);
		m_processor->OnRactSelected(cv::Rect(0, 0, 0, 0));
		break;
	}
}

void CProcessorBox::OnKeyPressed(int key_code)
{
	JCASSERT(m_processor);
	switch (key_code)
	{
	case 0x250000:	// <LEFT ARROW>
		m_processor->SetActiveChannel(m_processor->GetActiveChannel() -1);
		FillReviewBuf(m_local_buf);
		OnUpdateBox();
		break;
	case 0x270000:	// <RIGHT ARROW>
		m_processor->SetActiveChannel(m_processor->GetActiveChannel() +1);
		FillReviewBuf(m_local_buf);
		OnUpdateBox();
		break;

	case 0x260000:	{	// <UP ARROW> zoom out
		m_review_scale --;
		if (m_review_scale < 1) m_review_scale = 1;
		FillReviewBuf(m_local_buf);
		OnUpdateBox();
		break;			}

	case 0x280000:	{	// <DOWN ARROW> zoom in
		m_review_scale ++;
		if (m_review_scale > 16) m_review_scale = 16;
		FillReviewBuf(m_local_buf);
		OnUpdateBox();
		break;			}

	case 'S':	{	// save result
		CJCStringA str_fn;
		std::cout << "save " << GetNameA() << " to file, file name: \n";
		std::cin >> str_fn;
		std::cout << "saving ... ";
		m_processor->SaveResult(str_fn);
		std::cout << "done\n";
		break;		}
	}
}
