#include "iimage_proc.h"
#include <opencv2/core/core.hpp>
#include <vector>

class CImageProcessorBase : public IImageProcessor
{
public:
	CImageProcessorBase(void);
	virtual ~CImageProcessorBase(void);
	IMPLEMENT_INTERFACE;

public:
	virtual void OnParameterUpdated(void) = 0;
	virtual void OnInputUpdated(void) = 0;

	virtual void SetInput(int index, IImageProcessor * proc) = 0;
	virtual void ConnectOutput(IImageProcesser * proc);
	virtual void GetImaeg(Mat & img) = 0;

protected:
	std::vector<IImageProcessor *> m_output_list;
};

///////////////////////////////////////////////////////////////////////////////
//--
class CPreProcessor : CImageProcessorBase
{
public:
	CPreProcessor(const CJCStringT & file_name);
	virtual ~CPreProcessor(void);

public:
	virtual void OnParameterUpdated(void) = 0;
	virtual void OnInputUpdated(void) = 0;
	virtual void OnInitialize(void);

	virtual void SetInput(int index, IImageProcessor * proc) {}
	virtual void GetImaeg(Mat & img) = 0;

protected:
	CJCStringT	m_file_name;
	cv::Mat		m_src_img;
	cv::Mat		m_dst_img;
};

