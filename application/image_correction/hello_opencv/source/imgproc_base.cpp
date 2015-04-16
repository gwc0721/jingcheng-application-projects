#include "stdafx.h"

#include "imgproc_base.h"



///////////////////////////////////////////////////////////////////////////////
//--
CImageProcessorBase::CImageProcessorBase(void)
{
}

CImageProcessorBase::~CImageProcessorBase(void)
{
}

void CImageProcessorBase::ConnectOutput(IImageProcesser * proc)
{
	m_output_list.push_back(proc);
}


///////////////////////////////////////////////////////////////////////////////
//--
CPreProcessor::CPreProcessor(const CJCStringT & file_name)
	m_file_name(file_name)
{

}

CPreProcessor::~CPreProcessor(void)
{
}

void CPreProcessor::OnInitialize(void)
{
	char str_fn[MAX_PATH];
	stdext::UnicodeToUtf8(str_fn, MAX_PATH, m_file_name.c_str(), m_file_name.size());
	m_src_img = cv::imread(str_fn);

	cv::namedWindow("pre_processed");
}

void CPreProcessor::OnInputUpdated(void)
{
	// notify for updating
}

