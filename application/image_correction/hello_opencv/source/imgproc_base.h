#pragma once

#include "iimage_proc.h"
#include <opencv2/core/core.hpp>
#include <vector>

#include <zbar.h>

#define MAX_SOURCE	8
#define MAX_PARAM_NAME	32

#define PARAMETER_TABLE_SUPPORT		\
	static const PARAMETER_DEFINE m_param_def[];	\
	static const JCSIZE m_param_table_size;		\
	virtual JCSIZE GetParamDefineTab(const PARAMETER_DEFINE * & tab) const				\
	{tab = m_param_def; return m_param_table_size;}			\


///////////////////////////////////////////////////////////////////////////////
//--
class PARAMETER_DEFINE
{
public:
	UINT id;
	char name[MAX_PARAM_NAME];
	int default_val;
	int max_val;
	JCSIZE offset;
};

class CImageProcessorBase : public IImageProcessor
{
public:
	CImageProcessorBase(void);
	virtual ~CImageProcessorBase(void);
	IMPLEMENT_INTERFACE;

public:
	virtual void SetBox(IProcessorBox * box)
	{	m_proc_box = box;	JCASSERT(m_proc_box); }
	virtual void ConnectTo(int input_id, IImageProcessor * src);
	virtual int GetChannelNum(void) const {return 1;};
	virtual void SetActiveChannel(int ch);
	virtual int GetActiveChannel(void) const {return m_active_channel;}

	virtual void GetOutputImage(cv::Mat & img);
	virtual bool GetOutputImage(int img_id, cv::Mat & img) {img = m_dst_img; return true;}
	virtual bool GetReviewImage(cv::Mat & img) {img = m_dst_img; return true;}

	virtual bool GetProperty(const CJCStringT & param_name, jcparam::IValue * & val) 	{return false;}
	virtual bool SetProperty(const CJCStringT & prop_name, const jcparam::IValue * val)  {return false;}

	virtual void OnInitialize(void);
	virtual bool OnParameterUpdated(const char * name, int val);
	virtual void OnRactSelected(const cv::Rect & rect);

	virtual void write(cv::FileStorage & fs) const;
	virtual void read(cv::FileNode & node);
	// 缺省实现用于保存current channel的result image
	virtual void SaveResult(const CJCStringA & fn);

public:
	virtual JCSIZE GetParamDefineTab(const PARAMETER_DEFINE * &) const
	{return 0;}
public:
	void GetSource(int input_id, IImageProcessor * & src_proc);
	void GetSourceImage(int inport, int channel, cv::Mat & img);

protected:
	IProcessorBox * m_proc_box;	//不需要引用计数管理
	IImageProcessor * m_source[MAX_SOURCE];
	cv::Mat		m_dst_img;
	int	m_active_channel;
	cv::Rect	m_roi;
};

///////////////////////////////////////////////////////////////////////////////
//--

///////////////////////////////////////////////////////////////////////////////
//--
class CProcSource : public CImageProcessorBase
{
public:
	CProcSource(void) {};
	virtual ~CProcSource(void) {};

public:
	virtual bool SetProperty(const CJCStringT & prop_name, const jcparam::IValue * val);
	virtual bool OnCalculate(void);
	virtual const char * GetProcTypeA(void) const {return "source";};

protected:
	CJCStringT	m_file_name;
};

///////////////////////////////////////////////////////////////////////////////
//--
class CPreProcessor : public CImageProcessorBase
{
public:
	CPreProcessor(void) {};
	virtual ~CPreProcessor(void) {};

public:
	virtual bool OnCalculate(void);
	virtual const char * GetProcTypeA(void) const {return "pre-proces";};

protected:
	PARAMETER_TABLE_SUPPORT;

protected:
	int m_blur_size;
};

///////////////////////////////////////////////////////////////////////////////
//--
class CProcCanny : public CImageProcessorBase
{
	// input: 0 input image
public:
	CProcCanny(void);
	virtual ~CProcCanny(void);

public:
	virtual bool OnCalculate(void);
	virtual const char * GetProcTypeA(void) const {return "canny";};
	//virtual void SaveResult(const CJCStringA & fn);

protected:
	PARAMETER_TABLE_SUPPORT;

protected:
	int		m_kernel_size;
	int		m_threshold_low;
	int		m_threshold_hi;
};

///////////////////////////////////////////////////////////////////////////////
//--
class CProcMophology : public CImageProcessorBase
{
	// input: 0 input image
public:
	CProcMophology(void);
	virtual ~CProcMophology(void);

public:
	//virtual void OnInitialize(void);
	virtual bool OnParameterUpdated(const char * name, int val);
	virtual bool OnCalculate(void);
	virtual const char * GetProcTypeA(void) const {return "mophology";};

protected:
	PARAMETER_TABLE_SUPPORT;

protected:
	int		m_operator, m_kernel_size, m_element;
};

///////////////////////////////////////////////////////////////////////////////
//--
class CProcHoughLine : public CImageProcessorBase
{
	// input 0: result of canny out
	// input 1: source for show picture
public:
	CProcHoughLine(void);
	virtual ~CProcHoughLine(void) {};

public:
	virtual bool OnCalculate(void);
	virtual bool GetProperty(const CJCStringT & param_name, jcparam::IValue * & val);
	virtual const char * GetProcTypeA(void) const {return "hough-line";};

protected:
	PARAMETER_TABLE_SUPPORT;

protected:
	int		m_threshold, m_length, m_gap;
	double m_theta;
};

///////////////////////////////////////////////////////////////////////////////
//--
class CProcRotation : public CImageProcessorBase
{
	// input 0: source for rotate
	// input 1: theta
public:
	CProcRotation(void) {};
	virtual ~CProcRotation(void) {};

public:
	virtual bool OnCalculate(void);
	virtual const char * GetProcTypeA(void) const {return "rotation";};

protected:
	int		m_threshold, m_length, m_gap;
};

///////////////////////////////////////////////////////////////////////////////
//--
typedef std::vector<cv::Point>	CONTOUR;

class CProcContour : public CImageProcessorBase
{
	// input 0: result of canny out
	// input 1: source for show picture
public:
	CProcContour(void);
	virtual ~CProcContour(void){};

public:
	virtual bool OnCalculate(void);
	virtual bool GetProperty(const CJCStringT & param_name, jcparam::IValue * & val);
	virtual const char * GetProcTypeA(void) const {return "contour";};
	virtual void SaveResult(const CJCStringA & _fn);

	virtual bool GetReviewImage(cv::Mat & img);

protected:
	void DrawContour(cv::Mat & img, int index, const cv::Scalar & c_contour, const cv::Scalar & c_rect);

protected:
	cv::RotatedRect m_max_rect;
	std::vector<CONTOUR> m_contours;
};


///////////////////////////////////////////////////////////////////////////////
//--
class CContourMatch : public CProcContour
{
	// input 0: result of canny out
	// input 1: source for show picture
public:
	CContourMatch(void);
	virtual ~CContourMatch(void){};

public:
	virtual void OnInitialize(void);
	virtual bool OnCalculate(void);
	virtual const char * GetProcTypeA(void) const {return "contour_match";};
	virtual bool GetReviewImage(cv::Mat & img);

protected:
	PARAMETER_TABLE_SUPPORT;

protected:
	std::vector<CONTOUR> m_ref_contour;
	std::vector<int>	m_matched_index;
	int m_match_th;		// 匹配标准：match < 1e(-th)
};


///////////////////////////////////////////////////////////////////////////////
//--
class CProcThresh : public CImageProcessorBase
{
	// input 0: result of canny out
	// input 1: source for show picture
public:
	CProcThresh(void);
	virtual ~CProcThresh(void){};

public:
	virtual bool OnCalculate(void);
	virtual const char * GetProcTypeA(void) const {return "thresh";};

protected:
	PARAMETER_TABLE_SUPPORT;

protected:
	int		m_thresh, m_type;
};

///////////////////////////////////////////////////////////////////////////////
//--
class CProcHist : public CImageProcessorBase
{
	// input 0: result of canny out
	// input 1: source for show picture
public:
	CProcHist(void);
	virtual ~CProcHist(void){};

public:
	virtual bool OnCalculate(void);
	virtual const char * GetProcTypeA(void) const {return "hist";};
	virtual void write(cv::FileStorage & fs) const;

//protected:
//	PARAMETER_TABLE_SUPPORT;

protected:
	cv::Mat m_hist;
	int m_hist_size;
};

///////////////////////////////////////////////////////////////////////////////
//--
class CProcRotateClip : public CImageProcessorBase
{
	// input 0: result of canny out
	// input 1: source for show picture
public:
	CProcRotateClip(void);
	virtual ~CProcRotateClip(void){};

public:
	virtual bool OnCalculate(void);
	virtual const char * GetProcTypeA(void) const {return "hist";};
};

///////////////////////////////////////////////////////////////////////////////
//--
class CProcBarCode : public CImageProcessorBase
{
	// input 0: result of canny out
	// input 1: source for show picture
public:
	CProcBarCode(void);
	virtual ~CProcBarCode(void){};

public:
	virtual bool OnCalculate(void);
	virtual const char * GetProcTypeA(void) const {return "barcode";};

//protected:
//	PARAMETER_TABLE_SUPPORT;

protected:
	//int		m_thresh, m_type;
};

///////////////////////////////////////////////////////////////////////////////
//-- Split, 用于将原始图分成不同颜色空间
class CProcSplit : public CImageProcessorBase
{
public:
	CProcSplit(void);
	virtual ~CProcSplit(void){};

public:
	virtual bool OnCalculate(void);
	virtual const char * GetProcTypeA(void) const {return "split";};

	virtual bool GetReviewImage(cv::Mat & img);
	virtual int GetChannelNum(void) const
	{return m_channels.size();}
	virtual bool GetOutputImage(int img_id, cv::Mat & img);

protected:
	PARAMETER_TABLE_SUPPORT;

protected:
	int	m_type;		// 色彩空间类型
	std::vector<cv::Mat> m_channels;
};

///////////////////////////////////////////////////////////////////////////////
//-- 调节亮度和对比度
class CProcBrightness : public CImageProcessorBase
{
public:
	CProcBrightness(void);
	virtual ~CProcBrightness(void){};

public:
	virtual bool OnCalculate(void);
	virtual const char * GetProcTypeA(void) const {return "bright";};

protected:
	PARAMETER_TABLE_SUPPORT;

protected:
	int		m_alpha, m_beta;
};

///////////////////////////////////////////////////////////////////////////////
//-- 通过鼠标提取感兴趣的区域 -- 此功能有CImageProcessorBase实现，所有porc都可实现ROI功能
class CProcRoi : public CImageProcessorBase
{
public:
	CProcRoi(void) {};
	virtual ~CProcRoi(void) {};

public:
	virtual bool OnCalculate(void);
	virtual const char * GetProcTypeA(void) const {return "roi";};
	virtual void OnRactSelected(const cv::Rect & rect) {m_roi=rect;}
	virtual bool GetOutputImage(int img_id, cv::Mat & img);
	virtual bool GetReviewImage(cv::Mat & img);

protected:
	cv::Rect m_roi;
};

///////////////////////////////////////////////////////////////////////////////
//-- 图形匹配
class CProcMatch : public CImageProcessorBase
{
public:
	CProcMatch(void) {};
	virtual ~CProcMatch(void) {};

public:
	virtual void OnInitialize(void);
	virtual bool OnCalculate(void);
	virtual const char * GetProcTypeA(void) const {return "match";};
	//virtual bool GetOutputImage(int img_id, cv::Mat & img);
	//virtual bool GetReviewImage(cv::Mat & img);

protected:
	PARAMETER_TABLE_SUPPORT;

protected:
	cv::Mat m_ref_img;
	int m_method;
};
