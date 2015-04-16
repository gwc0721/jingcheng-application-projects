// 定义image processor接口

#include <stdext.h>
#include <opencv2/core/core.hpp>

class IProcessorContainer : public IJCInterface
{
public:
	virtual void OutputChanged(void) = 0;

};

class IImageProcessor : public IJCInterface
{
public:
	virtual void OnParameterUpdated(void) = 0;
	virtual void OnInputUpdated(void) = 0;
	virtual void OnInitialize(void) = 0;

	virtual void SetContainer(IProcessorContainer * cont) = 0;

	virtual void SetInput(int index, IImageProcessor * proc) = 0;
	virtual void ConnectOutput(IImageProcesser * proc) = 0;
	virtual void GetImaeg(cv::Mat & img) = 0;
};
