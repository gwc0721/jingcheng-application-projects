#include "iimage_proc.h"
#include <opencv2/core/core.hpp>
#include <vector>

#define MAX_SOURCE	8
#define MAX_PARAM_NAME	32

#define PARAMETER_TABLE_SUPPORT		\
	static const PARAMETER_DEFINE m_param_def[];	\
	static const JCSIZE m_param_table_size;		\
	virtual JCSIZE GetParamDefineTab(const PARAMETER_DEFINE * & tab) const				\
	{tab = m_param_def; return m_param_table_size;}			\


	//{tab = m_param_def; return sizeof(m_param_def) / sizeof(PARAMETER_DEFINE);}			\


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
	virtual void GetOutputImage(cv::Mat & img)
	{	img = m_dst_img;	}
	virtual bool GetCalParam(const CJCStringT & param_name, jcparam::IValue * & val) 
	{return false;}
	virtual void OnInitialize(void);
	virtual bool OnParameterUpdated(const char * name, int val);
	virtual void write(cv::FileStorage & fs) const;

public:
	virtual JCSIZE GetParamDefineTab(const PARAMETER_DEFINE * &) const
	{return 0;}

public:
	void GetSource(int input_id, IImageProcessor * & src_proc);

protected:
	IProcessorBox * m_proc_box;	//不需要引用计数管理
	IImageProcessor * m_source[MAX_SOURCE];
	cv::Mat		m_dst_img;
};

///////////////////////////////////////////////////////////////////////////////
//--
class CPreProcessor : public CImageProcessorBase
{
public:
	CPreProcessor(const CJCStringT & file_name);
	virtual ~CPreProcessor(void);

public:
	virtual void OnInitialize(void);
	//virtual bool OnParameterUpdated(const char * name, int val);
	virtual bool OnCalculate(void);
	virtual const char * GetProcTypeA(void) const {return "pre-processor";};

protected:
//	virtual JCSIZE GetParamDefineTab(const PARAMETER_DEFINE * & tab) const
//	{tab = m_param_def; return 1;}
//	static const PARAMETER_DEFINE m_param_def[];

	PARAMETER_TABLE_SUPPORT;

protected:
	CJCStringT	m_file_name;
	cv::Mat		m_src_img;

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
	//virtual void OnInitialize(void);
	//virtual bool OnParameterUpdated(const char * name, int val);
	virtual bool OnCalculate(void);
	virtual const char * GetProcTypeA(void) const {return "canny";};

protected:
//	virtual JCSIZE GetParamDefineTab(const PARAMETER_DEFINE * & tab) const
//	{tab = m_param_def; return sizeof(m_param_def) / sizeof(PARAMETER_DEFINE);}
//	static const PARAMETER_DEFINE m_param_def[];
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
	virtual bool GetCalParam(const CJCStringT & param_name, jcparam::IValue * & val);
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
class CProcContour : public CImageProcessorBase
{
	// input 0: result of canny out
	// input 1: source for show picture
public:
	CProcContour(void);
	virtual ~CProcContour(void){};

public:
	virtual bool OnCalculate(void);
	//virtual bool GetCalParam(const CJCStringT & param_name, jcparam::IValue * & val);
	virtual const char * GetProcTypeA(void) const {return "countour";};

//protected:
//	PARAMETER_TABLE_SUPPORT;

protected:
	int		m_threshold, m_length, m_gap;
	double m_theta;
};