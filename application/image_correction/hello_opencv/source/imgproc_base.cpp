#include "stdafx.h"

#include "imgproc_base.h"
#include "processor_factory.h"

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <math.h>

#include <fstream>

LOCAL_LOGGER_ENABLE(_T("image_processor"), LOGGER_LEVEL_DEBUGINFO);

///////////////////////////////////////////////////////////////////////////////
//--
LOG_CLASS_SIZE(CImageProcessorBase);

CImageProcessorBase::CImageProcessorBase(void)
: m_ref(1), m_proc_box(NULL)
, m_active_channel(0)
{
	memset(m_source, 0, sizeof(IImageProcessor*) * MAX_SOURCE);
}

CImageProcessorBase::~CImageProcessorBase(void)
{
	LOG_STACK_TRACE();
}

void CImageProcessorBase::ConnectTo(int input_id, IImageProcessor * src)
{
	LOG_STACK_TRACE();
	
	LOG_DEBUG(_T("connect src: <%08X> to dst: <%08X>, port %d"), (UINT)(src), (UINT)(this), input_id);

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
	if ( (param_num > 0) && (param_tab) && (m_proc_box) )
	{
		// m_proc_box允许为空，为空时忽略GUI处理
		//JCASSERT(m_proc_box);
		for (JCSIZE ii=0; ii < param_num; ++ii)
		{
			*((int*)((char *)(this) + param_tab[ii].offset)) = param_tab[ii].default_val;
			m_proc_box->RegistTrackBar(param_tab[ii].name, 
				param_tab[ii].default_val, param_tab[ii].max_val);
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
	// make name
	CJCStringA str_name;
	if (m_proc_box)	str_name = m_proc_box->GetNameA();
	else			str_name = GetProcTypeA();

	if ( (param_num > 0) && (param_tab) )
	{
		fs << str_name;
		fs << "{";
		// <TODO> 目前忽略porc名称，目标：自动生成临时名称
		fs << "proc_type" << GetProcTypeA();
		for (JCSIZE ii=0; ii < param_num; ++ii)
		{
			fs << param_tab[ii].name;
			int param_val = *((int*)((char *)(this) + param_tab[ii].offset));
			fs << param_val;
		}
		fs << "}";
	}
}

void CImageProcessorBase::read(cv::FileNode & node)
{
	LOG_STACK_TRACE();
	const PARAMETER_DEFINE * param_tab = NULL;
	JCSIZE param_num = GetParamDefineTab(param_tab);
	if ( (param_num > 0) && (param_tab) )
	{
		for (JCSIZE ii=0; ii < param_num; ++ii)
		{
			int &param_val = *((int*)((char *)(this) + param_tab[ii].offset));
			param_val = node[ param_tab[ii].name ];
			LOG_DEBUG(_T("read '%S' = %d"), param_tab[ii].name, param_val);
			if (m_proc_box) m_proc_box->UpdateTrackBar(ii, param_val);
		}
	}
	if (m_proc_box) m_proc_box->OnUpdateBox();
}

void CImageProcessorBase::GetSourceImage(int inport, int channel, cv::Mat & img)
{
	stdext::auto_interface<IImageProcessor> src_proc;
	GetSource(inport, src_proc);
	src_proc->GetOutputImage(img);
}

void CImageProcessorBase::SetActiveChannel(int ch) 
{
	int chs = GetChannelNum();
	m_active_channel = ch % chs;
}

void CImageProcessorBase::SaveResult(const CJCStringA & fn)
{
	CJCStringA _fn = fn + ".jpg";
	cv::Mat img;
	GetOutputImage(img);
	cv::imwrite(_fn, img);
}

void CImageProcessorBase::OnRactSelected(const cv::Rect & rect) 
{
	LOG_DEBUG(_T("ROI selected: (%d, %d) - (%d, %d)"), rect.x, rect.y, rect.width, rect.height);
	m_roi = rect;
}

void CImageProcessorBase::GetOutputImage(cv::Mat & img) 
{
	cv::Mat tmp;
	GetOutputImage(m_active_channel, tmp);
	if ( (m_roi.height != 0) && (m_roi.width !=0))	img = tmp(m_roi);
	else img = tmp;
}

///////////////////////////////////////////////////////////////////////////////
//--
LOG_CLASS_SIZE(CProcSource);

bool CProcSource::SetProperty(const CJCStringT & prop_name, const jcparam::IValue * val)
{
	if (prop_name == _T("file_name"))	val->GetValueText(m_file_name);
	else return __super::SetProperty(prop_name, val);
	return false;
}

bool CProcSource::OnCalculate(void)
{
	CJCStringA str_fn;
	stdext::UnicodeToUtf8(str_fn, m_file_name.c_str());
	m_dst_img = cv::imread(str_fn);
	int width = m_dst_img.cols, hight = m_dst_img.rows;
	LOG_DEBUG(_T("source image size: (%d, %d)"), width, hight);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//--
#define SCALE 4
LOG_CLASS_SIZE(CPreProcessor);

const PARAMETER_DEFINE CPreProcessor::m_param_def[] = {
	{0, "blur", 1, 10, offsetof(CPreProcessor, m_blur_size) },
};
const JCSIZE CPreProcessor::m_param_table_size = sizeof(CPreProcessor::m_param_def) / sizeof(PARAMETER_DEFINE);

bool CPreProcessor::OnCalculate(void)
{
	stdext::auto_interface<IImageProcessor> src;
	GetSource(0, src);

	cv::Mat tmp;
	src->GetOutputImage(tmp);
	int width = tmp.cols, hight = tmp.rows;
	LOG_DEBUG(_T("source image size: (%d, %d)"), width, hight);

	cv::Mat dst;
	//cv::resize(tmp, dst, cv::Size(width / SCALE, hight / SCALE) );
	//tmp = dst;
	cv::cvtColor(tmp, dst, CV_BGR2GRAY);

	// 降噪
	// kernel size 一定是单数
	if (m_blur_size == 0)
	{	// 不做降噪处理
		std::cout << "ignore blur\n";
		dst.copyTo(m_dst_img);
	}
	else
	{
		int blur = (m_blur_size-1) * 2 + 1;
		cv::GaussianBlur(dst, m_dst_img, cv::Size(blur, blur), 0, 0);
	}
	return true;
}

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
	{0, "thresh", 80, 500, offsetof(CProcHoughLine, m_threshold) },
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

bool CProcHoughLine::GetProperty(const CJCStringT & param_name, jcparam::IValue * & val)
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
	bool br = src_theta->GetProperty(_T("theta"), val);
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
	m_contours.clear();
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(canny, m_contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

	// 画轮廓
/*
	stdext::auto_interface<IImageProcessor> src_img;
	GetSource(1, src_img);		JCASSERT(src_img.valid());
	cv::Mat img;
	src_img->GetOutputImage(img);
	cv::cvtColor(img, m_dst_img, CV_GRAY2BGR);

	std::vector<cv::RotatedRect> minRect;
	float max_area = 0;
	int largest_contour = -1;
	for( JCSIZE ii = 0; ii< m_contours.size(); ii++ )
	{
		cv::Scalar color = cv::Scalar(0, 255, 0);
		cv::drawContours( m_dst_img, m_contours, ii, color, 1, 8, std::vector<cv::Vec4i>(), 0, cv::Point() );
		cv::RotatedRect rect = cv::minAreaRect(cv::Mat(m_contours[ii]));
		cv::Point2f rect_points[4];
		rect.points(rect_points);

		cv::Point2f p0 = rect_points[0];
		float len[4];
		for (int jj = 0; jj < 4; ++jj)
		{
			cv::Point2f p1 = rect_points[ (jj+1) & 0x03];
			//计算线段长度
			len[jj] = sqrt( (p0.x-p1.x)*(p0.x-p1.x) + (p0.y-p1.y)*(p0.y-p1.y));
			cv::line(m_dst_img, p0, p1, cv::Scalar(0, 0, 255), 2, 8);
			p0 = p1;
		}
		// 计算矩形面积
		//float area = len[0] * len[1];
		float area = rect.size.width * rect.size.height;
		LOG_DEBUG(_T("contour: %d, length: %f, %f, %f, %f"), ii, len[0], len[1], len[2], len[3]);
		LOG_DEBUG(_T("width: %f, height: %f, area: %f (%f)"), rect.size.width, rect.size.height, area, rect.size.area() );
		if (max_area < area) max_area = area, largest_contour = ii, m_max_rect = rect;
	}
	LOG_DEBUG(_T("largest contour: %d, area: %f"), largest_contour, max_area);
*/
	return true;
}

void CProcContour::DrawContour(cv::Mat & img, int index, const cv::Scalar & c_contour, const cv::Scalar & c_rect)
{
	cv::drawContours( img, m_contours, index, c_contour, 1, 8, std::vector<cv::Vec4i>(), 0, cv::Point() );
	cv::RotatedRect rect = cv::minAreaRect(cv::Mat(m_contours[index]));
	cv::Point2f rect_points[4];
	rect.points(rect_points);
	
	cv::Point2f p0 = rect_points[0];
	for (int jj = 0; jj < 4; ++jj)
	{
		cv::Point2f p1 = rect_points[ (jj+1) & 0x03];
		cv::line(img, p0, p1, c_rect, 2, 8);
		p0 = p1;
	}
}

bool CProcContour::GetReviewImage(cv::Mat & img)
{
	cv::Mat _img;
	GetSourceImage(1, 0, _img);
	if (_img.channels() < 3)	cv::cvtColor(_img, img, CV_GRAY2BGR);
	else					_img.copyTo(img);

	std::vector<cv::RotatedRect> minRect;
	float max_area = 0;
	int largest_contour = -1;
	for( JCSIZE ii = 0; ii< m_contours.size(); ii++ )
	{
		DrawContour(img, ii, cv::Scalar(0, 255, 0), cv::Scalar(0, 0, 255));
		//cv::Scalar color = cv::Scalar(0, 255, 0);
		//cv::drawContours( img, m_contours, ii, color, 1, 8, std::vector<cv::Vec4i>(), 0, cv::Point() );
		//cv::RotatedRect rect = cv::minAreaRect(cv::Mat(m_contours[ii]));
		//cv::Point2f rect_points[4];
		//rect.points(rect_points);

		//cv::Point2f p0 = rect_points[0];
		////float len[4];
		//for (int jj = 0; jj < 4; ++jj)
		//{
		//	cv::Point2f p1 = rect_points[ (jj+1) & 0x03];
		//	//计算线段长度
		//	//len[jj] = sqrt( (p0.x-p1.x)*(p0.x-p1.x) + (p0.y-p1.y)*(p0.y-p1.y));
		//	cv::line(img, p0, p1, cv::Scalar(0, 0, 255), 2, 8);
		//	p0 = p1;
		//}
		//// 计算矩形面积
		//float area = rect.size.area();
		////LOG_DEBUG(_T("contour: %d, length: %f, %f, %f, %f"), ii, len[0], len[1], len[2], len[3]);
		////LOG_DEBUG(_T("width: %f, height: %f, area: %f (%f)"), rect.size.width, rect.size.height, area, rect.size.area() );
		//LOG_DEBUG(_T("contour: %d, area: %f"), ii, area);
		//if (max_area < area) max_area = area, largest_contour = ii, m_max_rect = rect;
	}
	//LOG_DEBUG(_T("largest contour: %d, area: %f"), largest_contour, max_area);
	return true;
}

bool CProcContour::GetProperty(const CJCStringT & param_name, jcparam::IValue * & val)
{
	JCASSERT(val == NULL);
	if (param_name == _T("contour")) val = jcparam::CTypedValue<cv::RotatedRect >::Create(m_max_rect);
	else return false;
	return true;
}

void CProcContour::SaveResult(const CJCStringA & _fn)
{
	CJCStringA fn = _fn + ".xml";
	cv::FileStorage fs(fn, cv::FileStorage::WRITE);
	fs << "contours" << "[";
	std::vector<CONTOUR>::iterator cit = m_contours.begin();
	std::vector<CONTOUR>::iterator endcit = m_contours.end();
	int cc = 0;
	for (; cit!=endcit; ++cit, ++cc)
	{
		CONTOUR & contour = *cit;
		CONTOUR::iterator pit = contour.begin();
		CONTOUR::iterator end_pit = contour.end();
		fs << "{" << "points" << "[";
		for (; pit != end_pit; ++pit)
		{
			fs << "{";
			fs << "x" << (*pit).x;
			fs << "y" << (*pit).y;
			fs << "}";
		}
		fs  << "]" << "}";
	}
	fs << "]";
	//fs.release();
}

///////////////////////////////////////////////////////////////////////////////
//--

const PARAMETER_DEFINE CContourMatch::m_param_def[] = {
	{0, "thresh", 5, 20, offsetof(CContourMatch, m_match_th) },
};
const JCSIZE CContourMatch::m_param_table_size = sizeof(CContourMatch::m_param_def) / sizeof(PARAMETER_DEFINE);

CContourMatch::CContourMatch()
{
}

void CContourMatch::OnInitialize(void)
{
	// load reference data
	cv::FileStorage fs;
	fs.open("testdata\\contours_smi_logo.xml", cv::FileStorage::READ);
	cv::FileNode node = fs["contours"];
	cv::FileNodeIterator cit = node.begin();
	cv::FileNodeIterator end_cit = node.end();
	for ( ; cit != end_cit; ++ cit)
	{
		cv::FileNode & cnode = (*cit)["points"];
		cv::FileNodeIterator pit = cnode.begin();
		cv::FileNodeIterator end_pit = cnode.end();

		CONTOUR contour;
		for (; pit != end_pit; ++pit)
		{
			cv::FileNode & pnode = (*pit);
			cv::Point pp;
			pp.x = pnode["x"];
			pp.y = pnode["y"];
			contour.push_back(pp);
		}
		m_ref_contour.push_back(contour);
	}
	__super::OnInitialize();
}

bool CContourMatch::OnCalculate(void)
{
	bool br = __super::OnCalculate();

	double thresh = pow(10.0, -m_match_th);
	LOG_DEBUG(_T("threshold = %g"), thresh);
	m_matched_index.clear();

	cv::Mat match(m_contours.size(), m_ref_contour.size(), CV_64FC1);
	LOG_DEBUG(_T("created mat: row=%d, col=%d"), match.rows, match.cols);
	
	std::vector<CONTOUR>::iterator cit = m_contours.begin();
	std::vector<CONTOUR>::iterator end_cit = m_contours.end();
	for (int ii=0 ; cit != end_cit; ++cit, ++ii)
	{
		double m_val = 1;
		CONTOUR & contour = (*cit);
		std::vector<CONTOUR>::iterator rit = m_ref_contour.begin();
		std::vector<CONTOUR>::iterator end_rit = m_ref_contour.end();
		for (int jj=0 ; rit != end_rit; ++rit, ++jj)
		{
			CONTOUR & ref = (*rit);
			double mm = cv::matchShapes(ref, contour, CV_CONTOURS_MATCH_I2, 0);
			match.at<double>(ii, jj) = mm;
			m_val *= mm;
		}
		if (m_val < thresh)
		{
			LOG_DEBUG(_T("contour %d matched by %g"), ii, m_val);
			m_matched_index.push_back(ii);
		}
	}
	std::ofstream match_result("testdata\\contour_match.txt", std::ios::out);
	match_result << match;

	return br;
}

bool CContourMatch::GetReviewImage(cv::Mat & img)
{
	cv::Mat _img;
	GetSourceImage(1, 0, _img);
	if (_img.channels() < 3)	cv::cvtColor(_img, img, CV_GRAY2BGR);
	else					_img.copyTo(img);

	std::vector<int>::iterator it = m_matched_index.begin();
	std::vector<int>::iterator end_it = m_matched_index.end();
	for ( ; it != end_it; ++it)
	{
		int index = (*it);
		DrawContour(img, index, cv::Scalar(0, 255, 0), cv::Scalar(0, 0, 255) );
	}
	return true;
}





///////////////////////////////////////////////////////////////////////////////
//--
const PARAMETER_DEFINE CProcThresh::m_param_def[] = {
	{0, "thresh", 170, 500, offsetof(CProcThresh, m_thresh) },
	{1, "type", 1, 4, offsetof(CProcThresh, m_type) },
};

const JCSIZE CProcThresh::m_param_table_size = sizeof(CProcThresh::m_param_def) / sizeof(PARAMETER_DEFINE);

CProcThresh::CProcThresh(void)
: m_thresh(170), m_type(1)
{
}

bool CProcThresh::OnCalculate(void)
{
	stdext::auto_interface<IImageProcessor> src_proc;
	cv::Mat src;
	GetSource(0, src_proc);
	src_proc->GetOutputImage(src);
	cv::threshold(src, m_dst_img, m_thresh, 255, m_type);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//--
//const PARAMETER_DEFINE CProcThresh::m_param_def[] = {
//	{0, "thresh", 80, 500, offsetof(CProcThresh, m_thresh) },
//	{1, "type", 0, 4, offsetof(CProcThresh, m_type) },
//};
//
//const JCSIZE CProcThresh::m_param_table_size = sizeof(CProcThresh::m_param_def) / sizeof(PARAMETER_DEFINE);

#define HIST_H	400

CProcHist::CProcHist(void)
{
}

bool CProcHist::OnCalculate(void)
{
	stdext::auto_interface<IImageProcessor> src_proc;
	cv::Mat src;
	GetSource(0, src_proc);
	src_proc->GetOutputImage(src);

	m_hist_size = 255;
	int channel = 0;
	float range[] = {0, 255};
	const float * hist_range = {range};
	//cv::Mat m_hist;
	cv::calcHist(&src, 1, &channel, cv::Mat(), m_hist, 1, &m_hist_size, & hist_range, true, false);
	LOG_DEBUG(_T("hist size= %d"), m_hist_size);
	cv::Mat normal_hist;
	cv::normalize(m_hist, normal_hist, 0, HIST_H, cv::NORM_MINMAX, -1, cv::Mat());
	// 画直方图
	m_dst_img = cv::Mat(256, HIST_H, CV_8UC3, cv::Scalar(0, 0, 0));
	for (int ii = 1; ii< m_hist_size; ++ii)
	{
		line(m_dst_img, cv::Point(ii-1, HIST_H - (int)(normal_hist.at<float>(ii-1))),
			cv::Point(ii, HIST_H - (int)(normal_hist.at<float>(ii))), cv::Scalar(255, 255, 255), 2, 8, 0);
	}

	return true;
}

void CProcHist::write(cv::FileStorage & fs) const
{
	fs << GetProcTypeA();
	fs << "{";
	if (m_proc_box)		fs << "name" << m_proc_box->GetNameA();
	fs << "hist" << m_hist;
	fs << "}";

	std::ofstream hist_file("testdata\\hist.txt", std::ios::out);
	//JCSIZE hist_size = m_hist.size();
	for (int ii = 0; ii < m_hist_size; ++ii)
	{
		hist_file << m_hist.at<float>(ii) << std::endl;
	}
	hist_file.close();
}

///////////////////////////////////////////////////////////////////////////////
//--
//const PARAMETER_DEFINE CProcRotateClip::m_param_def[] = {
//	{0, "thresh", 170, 500, offsetof(CProcRotateClip, m_thresh) },
//	{1, "type", 1, 4, offsetof(CProcRotateClip, m_type) },
//};
//
//const JCSIZE CProcRotateClip::m_param_table_size = sizeof(CProcRotateClip::m_param_def) / sizeof(PARAMETER_DEFINE);

CProcRotateClip::CProcRotateClip(void)
{
}

bool CProcRotateClip::OnCalculate(void)
{
	stdext::auto_interface<IImageProcessor> src_img;
	stdext::auto_interface<IImageProcessor> src_theta;

	jcparam::IValue * val = NULL;
	GetSource(1, src_theta);		JCASSERT(src_theta.valid());
	bool br = src_theta->GetProperty(_T("contour"), val);
	jcparam::CTypedValue<cv::RotatedRect> * dval = dynamic_cast<jcparam::CTypedValue<cv::RotatedRect> *>(val);
	if ( (!br) || (dval == NULL) )
		THROW_ERROR(ERR_PARAMETER, _T("parameter theta does not exist in parent"));
	cv::RotatedRect & rect = (*dval);

	// 计算中心点, 倾角
	cv::Point2f center = rect.center;
	float angle = rect.angle;
	int left, right, top, bottom;
	LOG_DEBUG(_T("rect: width=%f, height=%f, angle=%f"), rect.size.width, rect.size.height, angle);
	// 计算旋转后的范围
	if (angle < -45)
	{
		angle += 90;
		left = (int)(center.x - rect.size.height / 2);
		right = (int)(center.x + rect.size.height / 2);
		top = (int)(center.y - rect.size.width / 2);
		bottom = (int)(center.y + rect.size.width / 2);
	}
	else
	{
		left = (int)(center.x - rect.size.width / 2);
		right = (int)(center.x + rect.size.width / 2);
		top = (int)(center.y - rect.size.height / 2);
		bottom = (int)(center.y + rect.size.height / 2);
	}

	// 计算旋转矩阵
	cv::Mat rot_mat(2, 3, CV_32FC1);
	rot_mat = cv::getRotationMatrix2D(center, angle, 1);

	cv::Mat src;
	GetSource(0, src_img);
	src_img->GetOutputImage(src);

	cv::Mat img(src.cols, src.rows, CV_8UC1);
	cv::warpAffine(src, img, rot_mat, src.size());
	cv::cvtColor(img, m_dst_img, CV_GRAY2BGR);
	cv::line(m_dst_img, cv::Point(left, top), cv::Point(right, top), cv::Scalar(0, 0, 255), 3, CV_AA);
	cv::line(m_dst_img, cv::Point(right, top), cv::Point(right, bottom), cv::Scalar(0, 0, 255), 3, CV_AA);
	cv::line(m_dst_img, cv::Point(right, bottom), cv::Point(left, bottom), cv::Scalar(0, 0, 255), 3, CV_AA);
	cv::line(m_dst_img, cv::Point(left, bottom), cv::Point(left, top), cv::Scalar(0, 0, 255), 3, CV_AA);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//--

CProcBarCode::CProcBarCode(void)
{
}

bool CProcBarCode::OnCalculate(void)
{
	stdext::auto_interface<IImageProcessor> src;
	GetSource(0, src);
	cv::Mat img;
	src->GetOutputImage(img);

	cv::Mat tmp;
	cv::cvtColor(img, tmp, CV_BGR2GRAY);		img = tmp;

	zbar::ImageScanner scanner;
	scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_ENABLE, 1);
	zbar::Image zimg(img.cols, img.rows, "Y800", img.data, img.cols * img.rows);
	int n = scanner.scan(zimg);

	cv::cvtColor(img, tmp, CV_GRAY2BGR);		img = tmp;
	for (zbar::Image::SymbolIterator symbol = zimg.symbol_begin(); symbol != zimg.symbol_end(); ++ symbol)
	{
		std::cout << "decoded" << symbol->get_type_name()
			<< ". symbol \"" << symbol->get_data() << "\"" << std::endl;
		zbar::Symbol::PointIterator pit = symbol->point_begin();
		//zbar::Symbol::PointIterator endpit = symbol->point_end();
		cv::Point p0((*pit).x, (*pit).y);
		cv::Point p00 = p0;
		int location_size = symbol->get_location_size();
		for (int ii=0; ii<location_size-1; ii++, ++pit)
		{
			cv::Point p0(symbol->get_location_x(ii), symbol->get_location_y(ii));
			cv::Point p1(symbol->get_location_x(ii+1), symbol->get_location_y(ii+1));
			cv::line(img, p0, p1, cv::Scalar(0, 255, 0), 2);
		}
	}
	cv::resize(img, m_dst_img, cv::Size(img.cols / 4, img.rows / 4) );
	return true;
}
///////////////////////////////////////////////////////////////////////////////
//-- Split, 用于将原始图分成不同颜色空间
const PARAMETER_DEFINE CProcSplit::m_param_def[] = {
	{0, "type", 0, 4, offsetof(CProcSplit, m_type) },
};
const JCSIZE CProcSplit::m_param_table_size = sizeof(CProcSplit::m_param_def) / sizeof(PARAMETER_DEFINE);

CProcSplit::CProcSplit(void)
{
}

bool CProcSplit::OnCalculate(void)
{
	m_channels.clear();
	cv::Mat src, tmp;
	GetSourceImage(0, 0, src);

	int cvt_type;
	switch (m_type)
	{
	case 0:	/*cvt_type = CV_BRG2GRAY;*/	break;	// Gray
	case 1:	cvt_type = CV_BGR2XYZ;	break;	// XYZ
	case 2:	cvt_type = CV_BGR2YCrCb;break;	// YCrCb
	case 3:	cvt_type = CV_BGR2HSV;	break;	// HSV
	case 4:	cvt_type = CV_BGR2HLS;	break;	// HLS
	}

	if (m_type != 0) cv::cvtColor(src, tmp, cvt_type);
	else				tmp = src;
	cv::split(tmp, m_channels);
	return true;
}

bool CProcSplit::GetReviewImage(cv::Mat & img)
{
	JCSIZE chs = m_channels.size();
	if (m_active_channel >= chs)	m_active_channel %= chs;
	return	GetOutputImage(m_active_channel, img);
}

bool CProcSplit::GetOutputImage(int img_id, cv::Mat & img)
{
	if (img_id >= m_channels.size() ) return false;
	img = m_channels.at(img_id);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//-- 调节亮度和对比度
const PARAMETER_DEFINE CProcBrightness::m_param_def[] = {
	{0, "alpha", 20, 200, offsetof(CProcBrightness, m_alpha) },
	{0, "beta", 100, 200, offsetof(CProcBrightness, m_beta) },
};
const JCSIZE CProcBrightness::m_param_table_size = sizeof(CProcBrightness::m_param_def) / sizeof(PARAMETER_DEFINE);

CProcBrightness::CProcBrightness(void)
{
}

bool CProcBrightness::OnCalculate(void)
{
	cv::Mat src;
	GetSourceImage(0, 0, src);
	src.convertTo(m_dst_img, -1, ((float)m_alpha) / 20.0, m_beta-100);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//-- 通过鼠标提取感兴趣的区域
bool CProcRoi::OnCalculate(void)
{
	LOG_DEBUG(_T("ROI = (%d, %d) - %d x %d"), m_roi.x, m_roi.y, m_roi.width, m_roi.height);
	GetSourceImage(0, 0, m_dst_img);
	return true;
}

bool CProcRoi::GetOutputImage(int img_id, cv::Mat & img)
{
	if ( (m_roi.height != 0) && (m_roi.width !=0))	img = m_dst_img(m_roi);
	else img = m_dst_img;
	return true;	
}

bool CProcRoi::GetReviewImage(cv::Mat & img)
{
	if (m_dst_img.channels() >= 3)	m_dst_img.copyTo(img);
	else							cv::cvtColor(m_dst_img, img, CV_GRAY2BGR);
	if ( (m_roi.height != 0) && (m_roi.width !=0))
	{
		cv::rectangle(img, m_roi, cv::Scalar(255, 0, 0), 3);
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//-- 图形匹配

const PARAMETER_DEFINE CProcMatch::m_param_def[] = {
	{0, "method", 2, 5, offsetof(CProcMatch, m_method) },
};
const JCSIZE CProcMatch::m_param_table_size = sizeof(CProcMatch::m_param_def) / sizeof(PARAMETER_DEFINE);

void CProcMatch::OnInitialize(void)
{
	cv::Mat img = cv::imread("testdata\\smi_logo.jpg");
	cv::cvtColor(img, m_ref_img, CV_BGR2GRAY);
	__super::OnInitialize();
}

bool CProcMatch::OnCalculate(void)
{
	//std::ofstream f_out("testdata\\match.txt", std::ios::out);
	LOG_DEBUG(_T("matching method (%d)"), m_method);
	cv::Mat src_img;
	GetSourceImage(0, 0, src_img);
	if (src_img.channels() >=3) cv::cvtColor(src_img, src_img, CV_BGR2GRAY);
	cv::Mat tmp;
	{
		LOG_STACK_TRACE();
		cv::matchTemplate(src_img, m_ref_img, tmp, m_method);
	}
	double min_val = 0, max_val = 0;
	cv::Point min_loc, max_loc;
	cv::minMaxLoc(tmp, &min_val, &max_val, &min_loc, &max_loc);
	LOG_DEBUG(_T("found min: val=%f, (%d,%d)"), min_val, min_loc.x, min_loc.y);
	LOG_DEBUG(_T("found max: val=%f, (%d,%d)"), max_val, max_loc.x, max_loc.y);
	//f_out << "mached" << std::endl << tmp << std::endl;
	cv::normalize(tmp, tmp, 0, 255, cv::NORM_MINMAX);
	//f_out << "normalized" << std::endl << tmp << std::endl;
	tmp.convertTo(m_dst_img, CV_8UC1);
	return true;
}


///////////////////////////////////////////////////////////////////////////////
//--
template <>	jcparam::VALUE_TYPE jcparam::type_id<cv::RotatedRect>::id(void)		{return jcparam::VT_UNKNOW;}

template <> void jcparam::CConvertor<cv::RotatedRect>::S2T(LPCTSTR str, cv::RotatedRect & typ)
{
}

template <> void jcparam::CConvertor<cv::RotatedRect>::T2S(const cv::RotatedRect & typ, CJCStringT & str)
{
}
	
