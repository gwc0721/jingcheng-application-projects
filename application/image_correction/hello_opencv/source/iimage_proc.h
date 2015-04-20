#pragma once
// 定义image processor接口

#include <stdext.h>
#include <opencv2/core/core.hpp>
#include <jcparam.h>

class IProcessorBox
{	// 不需要引用计数
public:
	// processor请求box创建track bar，返回track bar的id
	virtual UINT RegistTrackBar(const char * name, int def_val, int max_val) = 0;
	virtual const char * GetNameA(void) const = 0;
	virtual void OnTrackBarUpdated(UINT id, const CJCStringA & name, int val) = 0;
};

//class IProcessorContainer : public IJCInterface
//{
//public:
//	virtual void OutputChanged(void) = 0;
//
//};

class IImageProcessor : public IJCInterface
{
public:
	virtual void SetBox(IProcessorBox * box) = 0;
	// 指定processor的输入端(input_id)连接到输出processor，输出端只提供单一输出
	virtual void ConnectTo(int input_id, IImageProcessor * src) = 0; 

	// 启动processor计算，返回是否需要更新结果
	virtual bool OnCalculate(void) = 0;
	// 更新参数，返回是否需要更新结果
	virtual bool OnParameterUpdated(const char * name, int val) = 0;

	virtual void OnInitialize(void) = 0;
	virtual void GetOutputImage(cv::Mat & img) = 0;
	// 获取一些通过计算得到的参数，比如图像倾角等，如果不存在指定参数，侧返回false
	virtual bool GetCalParam(const CJCStringT & param_name, jcparam::IValue * & val) = 0;
	virtual const char * GetProcTypeA(void) const = 0;
	virtual void write(cv::FileStorage & fs) const = 0;
//	virtual void read(const cv::FileStorage & fs) const = 0;
};
