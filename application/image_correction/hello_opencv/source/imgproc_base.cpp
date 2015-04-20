#include "stdafx.h"

#include "imgproc_base.h"
#include "processor_factory.h"

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <math.h>

LOCAL_LOGGER_ENABLE(_T("image_processor"), LOGGER_LEVEL_DEBUGINFO);

///////////////////////////////////////////////////////////////////////////////
//--
LOG_CLASS_SIZE(CImageProcessorBase);

CImageProcessorBase::CImageProcessorBase(void)
: m_ref(1), m_proc_box(NULL)
{
	memset(m_source, 0, sizeof(IImageProcessor*) * MAX_SOURCE);
}

CImageProcessorBase::~CImageProcessorBase(void)
{
	LOG_STACK_TRACE();
}

void CImageProcessorBase::ConnectTo(int input_id, IImageProcessor * src)
{
	LOG_STACK_TRACE_EX(_T("input: %d, src: <%08X>"), input_id, (UINT)(src));

	JCASSERT(input_id < MAX_SOURCE);
	if (m_source[input_id] != NULL)
	{
		THROW_ERROR(ERR_PARAMETER, _T("source %d has already been set."), input_id);
	}
	m_source[input_id] = src;
}

void CImageProcessorBase::GetSource(int input_id, IImageProcessor * & src_proc)
{
	JCASSERT(src_proc == NULL);
	JCASSERT(input_id < MAX_SOURCE);
	src_proc = m_source[input_id];		JCASSERT(src_proc);
	src_proc->AddRef();
}

void CImageProcessorBase::OnInitialize(void)
{
	const PARAMETER_DEFINE * param_tab = NULL;
	JCSIZE param_num = GetParamDefineTab(param_tab);
	if ( (param_num > 0) && (param_tab) )
	{
		JCASSERT(m_proc_box);
		for (JCSIZE ii=0; ii < param_num; ++ii)
		{
			int default_val = *((int*)((char *)(this) + param_tab[ii].offset));
			m_proc_box->RegistTrackBar(param_tab[ii].name, 
				/*param_tab[ii].default_val,*/ default_val, param_tab[ii].max_val);
		}
	}
}

bool CImageProcessorBase::OnParameterUpdated(const char * name, int val)
{
	bool updated = false;
	const PARAMETER_DEFINE * param_tab = NULL;
	JCSIZE param_num = GetParamDefineTab(param_tab);
	if ( (param_num > 0) && (param_tab) )
	{
		JCASSERT(m_proc_box);
		for (JCSIZE ii=0; ii < param_num; ++ii)
		{
			if (strcmp(name, param_tab[ii].name) == 0)
			{
				*((int*)((char *)(this) + param_tab[ii].offset)) = val;
				updated = true;
				break;
			}
		}
	}
	return updated;
}

void CImageProcessorBase::write(cv::FileStorage & fs) const
{
	const PARAMETER_DEFINE * param_tab = NULL;
	JCSIZE param_num = GetParamDefineTab(param_tab);
	if ( (param_num > 0) && (param_tab) )
	{
		fs << GetProcTypeA();
		fs << "{";
		JCASSERT(m_proc_box);
		fs << "name" << m_proc_box->GetNameA();
		JCASSERT(m_proc_box);
		for (JCSIZE ii=0; ii < param_num; ++ii)
		{
			fs << param_tab[ii].name;
			fs << *((int*)((char *)(this) + param_tab[ii].offset));
			m_proc_box->RegistTrackBar(param_tab[ii].name, 
				param_tab[ii].default_val, param_tab[ii].max_val);
		}
		fs << "}";
	}

}

///////////////////////////////////////////////////////////////////////////////
//--
#define SCALE 3
LOG_CLASS_SIZE(CPreProcessor);

bool CreatePreProcessor(const CJCStringT & src_fn, IImageProcessor * & proc)
{
	JCASSERT(proc == NULL);
	proc = static_cast<IImageProcessor*>(new CPreProcessor(src_fn));
	return true;
}

const PARAMETER_DEFINE CPreProcessor::m_param_def[] = {
	{0, "blur", 1, 10, offsetof(CPreProcessor, m_blur_size) },
};
const JCSIZE CPreProcessor::m_param_table_size = sizeof(CPreProcessor::m_param_def) / sizeof(PARAMETER_DEFINE);

CPreProcessor::CPreProcessor(const CJCStringT & file_name)
	:m_file_name(file_name)
	, m_blur_size(1)
{
}

CPreProcessor::~CPreProcessor(void)
{
	LOG_STACK_TRACE();
}

void CPreProcessor::OnInitialize(void)
{
	// 打开文件
	CJCStringA str_fn;
	stdext::UnicodeToUtf8(str_fn, m_file_name.c_str());
	cv::Mat tmp;
	tmp = cv::imread(str_fn);
	int width = tmp.cols, hight = tmp.rows;
	LOG_DEBUG(_T("source image size: (%d, %d)"), width, hight)
	cv::resize(tmp, m_src_img, cv::Size(width / SCALE, hight / SCALE) );
	tmp = m_src_img;
	cv::cvtColor(tmp, m_src_img, CV_BGR2GRAY);

	// 注册滑动条
	CImageProcessorBase::OnInitialize();
	//JCASSERT(m_proc_box);
	//m_proc_box->RegistTrackBar("blur", m_blur_size, 10);
}

bool CPreProcessor::OnCalculate(void)
{
	// 降噪
	// cv::blur(m_src_img, m_dst_img, cv::Size(m_blur_size, m_blur_size));
	// kernel size 一定是单数
	if (m_blur_size == 0)
	{	// 不做降噪处理
		std::cout << "ignore blur\n";
		m_src_img.copyTo(m_dst_img);
	}
	else
	{
		int blur = (m_blur_size-1) * 2 + 1;
		cv::GaussianBlur(m_src_img, m_dst_img, cv::Size(blur, blur), 0, 0);
	}
	return true;
}

//bool CPreProcessor::OnParameterUpdated(const char * name, int val)
//{
//	if (strcmp(name, "blur") == 0)
//	{
//		m_blur_size = val;
//		return true;
//	}
//	return false;
//}

///////////////////////////////////////////////////////////////////////////////
//--
LOG_CLASS_SIZE(CProcCanny);

const PARAMETER_DEFINE CProcCanny::m_param_def[] = {
	{0, "kernel", 3, 10, offsetof(CProcCanny, m_kernel_size) },
	{1, "th_low", 90, 300, offsetof(CProcCanny, m_threshold_low) },
	{2, "th_hi", 270, 500, offsetof(CProcCanny, m_threshold_hi) },
};

const JCSIZE CProcCanny::m_param_table_size = sizeof(CProcCanny::m_param_def) / sizeof(PARAMETER_DEFINE);

CProcCanny::CProcCanny(void)
: m_kernel_size(3), m_threshold_low(90), m_threshold_hi(270)
{
}

CProcCanny::~CProcCanny(void)
{
}

//void CProcCanny::OnInitialize(void)
//{
//	JCASSERT(m_proc_box);
//	m_proc_box->RegistTrackBar("kernel", m_kernel_size, 10);
//	m_proc_box->RegistTrackBar("th_low", m_threshold_low, 300);
//}

//bool CProcCanny::OnParameterUpdated(const char * name, int val)
//{
//	bool updated = false;
//	if (strcmp(name, "kernel") == 0)
//	{
//		if ( (val <= 1) ) return false;
//		m_kernel_size = val, updated = true;
//	}
//	else if (strcmp(name, "th_low") == 0)	m_threshold_low = val, updated = true;
//	return updated;
//}

bool CProcCanny::OnCalculate(void)
{
	stdext::auto_interface<IImageProcessor> src_proc;
	//IImageProcessor * src_proc = NULL;
	cv::Mat src;
	GetSource(0, src_proc);
	src_proc->GetOutputImage(src);
	cv::Canny(src, m_dst_img, m_threshold_low, m_threshold_hi, m_kernel_size);
	//src_proc->Release();
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//--
LOG_CLASS_SIZE(CProcMophology);

const PARAMETER_DEFINE CProcMophology::m_param_def[] = {
	{0, "kernel", 10, 50, offsetof(CProcMophology, m_kernel_size) },
	{1, "element", 2, 2, offsetof(CProcMophology, m_element) },
	{2, "operator", 2, 6, offsetof(CProcMophology, m_operator) },
};
const JCSIZE CProcMophology::m_param_table_size = sizeof(CProcMophology::m_param_def) / sizeof(PARAMETER_DEFINE);

CProcMophology::CProcMophology(void)
: m_operator(2), m_kernel_size(10), m_element(2)
{
}

CProcMophology::~CProcMophology(void)
{
}

//void CProcMophology::OnInitialize(void)
//{
//	JCASSERT(m_proc_box);
//	m_proc_box->RegistTrackBar("operator", m_operator, 6);
//	m_proc_box->RegistTrackBar("element", m_element, 2);
//	m_proc_box->RegistTrackBar("kernel", m_kernel_size, 50);
//}

bool CProcMophology::OnParameterUpdated(const char * name, int val)
{
	bool updated = false;

	if (strcmp(name, "operator") == 0)
	{
		m_operator = val;
		_tprintf_s(_T("set mophology opeartor to : "));
		switch (m_operator)
		{
		case cv::MORPH_ERODE:		_tprintf_s(_T("erode\n")); break;
		case cv::MORPH_DILATE:		_tprintf_s(_T("dilate\n")); break;
		case cv::MORPH_OPEN:		_tprintf_s(_T("open\n")); break;
		case cv::MORPH_CLOSE:		_tprintf_s(_T("close\n")); break;
		case cv::MORPH_GRADIENT:	_tprintf_s(_T("gradient\n")); break;
		case cv::MORPH_TOPHAT:		_tprintf_s(_T("tophat\n")); break;
		case cv::MORPH_BLACKHAT:	_tprintf_s(_T("blackhat\n")); break;
		}
		return true;
	}
	else if (strcmp(name, "element") == 0)
	{
		m_element = val;
		_tprintf_s(_T("select mophology shart as : "));
		switch (m_element)
		{
		case cv::MORPH_RECT:	_tprintf_s(_T("rect\n"));	break;
		case cv::MORPH_CROSS:	_tprintf_s(_T("cross\n"));	break;
		case cv::MORPH_ELLIPSE:	_tprintf_s(_T("ellipse\n"));	break;
		}
		return true;
	}
	else if (strcmp(name, "kernel") == 0)
	{
		//if ( (val <= 1) ) return false;
		m_kernel_size = val, updated = true;
	}
	return updated;
}

bool CProcMophology::OnCalculate(void)
{
	stdext::auto_interface<IImageProcessor> src_proc;
	bool br = false;
	cv::Mat src;
	GetSource(0, src_proc);
	src_proc->GetOutputImage(src);		JCASSERT(src_proc.valid());
	cv::Mat element = cv::getStructuringElement(m_element, cv::Size(m_kernel_size, m_kernel_size)); 
	cv::morphologyEx(src, m_dst_img, m_operator, element);
	br = true;
	return br;
}	

///////////////////////////////////////////////////////////////////////////////
//--
LOG_CLASS_SIZE(CProcHoughLine);

const PARAMETER_DEFINE CProcHoughLine::m_param_def[] = {
	{0, "threshold", 80, 500, offsetof(CProcHoughLine, m_threshold) },
	{1, "length", 150, 500, offsetof(CProcHoughLine, m_length) },
	{2, "gap", 10, 50, offsetof(CProcHoughLine, m_gap) },
};

const JCSIZE CProcHoughLine::m_param_table_size = sizeof(CProcHoughLine::m_param_def) / sizeof(PARAMETER_DEFINE);

CProcHoughLine::CProcHoughLine(void)
: m_threshold(80), m_length(150), m_gap(10)
{
}

bool CProcHoughLine::OnCalculate(void)
{
	stdext::auto_interface<IImageProcessor> src_canny;
	stdext::auto_interface<IImageProcessor> src_img;

	cv::Mat canny;
	GetSource(0, src_canny);		JCASSERT(src_canny.valid());
	src_canny->GetOutputImage(canny);
	
	std::vector<cv::Vec4i> lines;
	HoughLinesP(canny, lines, 1, CV_PI / 180, m_threshold, m_length, m_gap);

	//画线
	cv::Mat img;
	GetSource(1, src_img);		JCASSERT(src_img.valid());
	src_img->GetOutputImage(img);
	cv::cvtColor(img, m_dst_img, CV_GRAY2BGR);
	
	size_t ii;
	double avg_theta = 0;
	for (ii = 0; ii < lines.size(); ++ii)
	{
		cv::Vec4i ll = lines[ii];
		cv::line(m_dst_img, cv::Point(ll[0], ll[1]), cv::Point(ll[2], ll[3]), cv::Scalar(0, 0, 128+ii*10), 3, CV_AA);
		// 计算线段的倾角
		int dx = ll[2] - ll[0];
		int dy = ll[3] - ll[1];
		double theta = atan2( (double)dy, (double)dx);
		if (theta > (CV_PI / 4) )
		{
			double th1 = -((CV_PI / 2) - theta);
			avg_theta += th1;
			LOG_DEBUG(_T("line: %d, theta= %f (+)=> %f"), ii, theta * 180 / CV_PI, th1 * 180 / CV_PI);
		}
		else if (theta < (CV_PI / -4))
		{
			double th1 = -( (CV_PI / -2 ) - theta);
			avg_theta += th1;
			LOG_DEBUG(_T("line: %d, theta= %f (-)=> %f"), ii, theta * 180 / CV_PI, th1 * 180 / CV_PI);
		}
		else
		{
			avg_theta += theta;
			LOG_DEBUG(_T("line: %d, theta= %f (*)"), ii, theta * 180 / CV_PI);
		}
	}
	avg_theta /= ii;
	m_theta = avg_theta;
	LOG_DEBUG(_T("average theta = %f"), m_theta);
	std::cout << m_theta;
	return true;
}

bool CProcHoughLine::GetCalParam(const CJCStringT & param_name, jcparam::IValue * & val)
{
	JCASSERT(val == NULL);
	if (param_name == _T("theta"))
	{
		val = jcparam::CTypedValue<double>::Create(m_theta);
	}
	else return false;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//--
LOG_CLASS_SIZE(CProcRotation);

bool CProcRotation::OnCalculate(void)
{
	stdext::auto_interface<IImageProcessor> src_img;
	stdext::auto_interface<IImageProcessor> src_theta;

	jcparam::IValue * val = NULL;
	GetSource(1, src_theta);		JCASSERT(src_theta.valid());
	bool br = src_theta->GetCalParam(_T("theta"), val);
	jcparam::CTypedValue<double> * dval = dynamic_cast<jcparam::CTypedValue<double> *>(val);

	if ( (!br) || (dval == NULL) )
		THROW_ERROR(ERR_PARAMETER, _T("parameter theta does not exist in parent"));
	double theta = *dval;
	LOG_DEBUG(_T("read theta from source = %f"), theta * 180 / CV_PI);
	val->Release();
	
	cv::Mat src;
	GetSource(0, src_img);		JCASSERT(src_img.valid());
	src_img->GetOutputImage(src);

	cv::Point center = cv::Point( src.cols / 2, src.rows / 2);
	cv::Mat rot_mat(2, 3, CV_32FC1);
	rot_mat = cv::getRotationMatrix2D(center, theta * 180 / CV_PI, 1);
	//dst = Mat::zeros(src.rows, src.cols, src.type() );
	m_dst_img = cv::Mat(src.cols, src.rows, CV_8UC1, cv::Scalar(255) );
	cv::warpAffine(src, m_dst_img, rot_mat, src.size());
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//--
CProcContour::CProcContour(void)
{
}

bool CProcContour::OnCalculate(void)
{
	stdext::auto_interface<IImageProcessor> src_canny;
	GetSource(0, src_canny);		JCASSERT(src_canny.valid());
	cv::Mat canny;
	src_canny->GetOutputImage(canny);

	// 计算轮廓
	std::vector<std::vector<cv::Point> > contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(canny, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

	// 画轮廓
	stdext::auto_interface<IImageProcessor> src_img;
	GetSource(1, src_img);		JCASSERT(src_img.valid());
	cv::Mat img;
	src_img->GetOutputImage(img);
	cv::cvtColor(img, m_dst_img, CV_GRAY2BGR);

	std::vector<cv::RotatedRect> minRect;
	for( JCSIZE ii = 0; ii< contours.size(); ii++ )
	{
		cv::Scalar color = cv::Scalar(0, 255, 0);
		cv::drawContours( m_dst_img, contours, ii, color, 1, 8, std::vector<cv::Vec4i>(), 0, cv::Point() );
		cv::RotatedRect rect = cv::minAreaRect(cv::Mat(contours[ii]));
		cv::Point2f rect_points[4];
		rect.points(rect_points);
		for (int jj = 0; jj < 4; ++jj)
		{
			cv::line(m_dst_img, rect_points[jj], rect_points[(jj+1)&0x3], cv::Scalar(0, 0, 255), 2, 8);
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//--
bool CreateProcessor(const CJCStringT & proc_name, IImageProcessor * & proc)
{
	JCASSERT(proc == NULL);
	if (proc_name == _T("canny"))			proc = static_cast<IImageProcessor*>(new CProcCanny);
	else if (proc_name == _T("mophology"))	proc = static_cast<IImageProcessor*>(new CProcMophology);
	else if (proc_name == _T("houghline"))	proc = static_cast<IImageProcessor*>(new CProcHoughLine);
	else if (proc_name == _T("rotation"))	proc = static_cast<IImageProcessor*>(new CProcRotation);
	else if (proc_name == _T("countour"))	proc = static_cast<IImageProcessor*>(new CProcContour);
	else
	{
		THROW_ERROR(ERR_PARAMETER, _T("processor: %s does not exist"), proc_name.c_str() );
		return false;
	}
	return true;
}
