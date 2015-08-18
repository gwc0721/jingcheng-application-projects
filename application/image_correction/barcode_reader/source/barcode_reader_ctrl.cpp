#include "stdafx.h"
#include "../include/barcode_reader_ctrl.h"
#include <stdext.h>
#include <zbar.h>
#include <dshow.h>

#define VIDEO_TIMER	300

LOCAL_LOGGER_ENABLE(_T("barcodectrl"), LOGGER_LEVEL_NOTICE);

#ifdef _DEBUG
#define LOG_OUTPUT_MAT(mat_name, mat)		{		\
	std::stringstream	_str_mat_;	\
	_str_mat_ << mat;				\
	LOG_DEBUG(_T("%s=\n%S"), mat_name, _str_mat_.str().c_str());	}
#else
#define LOG_OUTPUT_MAT(...)
#endif

///////////////////////////////////////////////////////////////////////////////
// -- 
BEGIN_MESSAGE_MAP(CBarCodeReaderCtrl, CStatic)
	ON_WM_PAINT()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CBarCodeReaderCtrl::CBarCodeReaderCtrl(void)
{
	m_hold_time = 15;
	m_frame_interval = 100;		// 10 frame per second
	m_show_scale = 2;
	m_show_width = 0;
	m_show_height = 0;
	m_video_width = 0;
	m_video_height = 0;
	m_mirror=false;
	m_rotation = 0;
}

void CBarCodeReaderCtrl::OnTimer(UINT id)
{
	if (id == VIDEO_TIMER)
	{
		cv::Mat frame;
		m_capture >> frame;
		UpdateFrame(frame);
	}
}

void CBarCodeReaderCtrl::OnPaint()
{
	CPaintDC dc(this);
	if (!m_frame_show.empty())
	{
		ShowFrame(m_frame_show, &dc);
	}
}

void  CBarCodeReaderCtrl::FillBitmapInfo( BITMAPINFO* bmi, int width, int height, int bpp, int origin )
{
	JCASSERT( bmi && width >= 0 && height >= 0 && (bpp == 8 || bpp == 24 || bpp == 32));

	BITMAPINFOHEADER* bmih = &(bmi->bmiHeader);

	memset( bmih, 0, sizeof(*bmih));
	bmih->biSize = sizeof(BITMAPINFOHEADER);
	bmih->biWidth = width;
	bmih->biHeight = origin ? abs(height) : -abs(height);
	bmih->biPlanes = 1;
	bmih->biBitCount = (unsigned short)bpp;
	bmih->biCompression = BI_RGB;

	if( bpp == 8 )
	{
		RGBQUAD* palette = bmi->bmiColors;
		int i;
		for( i = 0; i < 256; i++ )
		{
			palette[i].rgbBlue = palette[i].rgbGreen = palette[i].rgbRed = (BYTE)i;
			palette[i].rgbReserved = 0;
		}
	}
}

void CBarCodeReaderCtrl::ShowFrame(cv::Mat & img, CDC * dc)
{
	uchar buffer[sizeof(BITMAPINFOHEADER) + 1024];
	BITMAPINFO* bmi = (BITMAPINFO*)buffer;
	int bmp_w = img.cols, bmp_h = img.rows;

	int bbp = img.channels()*8;

	FillBitmapInfo( bmi, bmp_w, bmp_h, bbp, 0 );
	int from_x = 0, from_y = 0;

	int sw = m_show_width;
	int sh = m_show_height;	
	HDC hdc = dc->GetSafeHdc();
	
	SetDIBitsToDevice(hdc, 0, 0, sw, sh, 0, 0, 0, sh,
		img.data, bmi, DIB_RGB_COLORS );
}

bool CBarCodeReaderCtrl::Open(int dev_id)
{
	bool br = m_capture.open(dev_id);
	if (!br) return false;
	if (!m_capture.isOpened() ) return false;

	m_video_width = (int)m_capture.get(CV_CAP_PROP_FRAME_WIDTH);
	m_video_height = (int)m_capture.get(CV_CAP_PROP_FRAME_HEIGHT);
	double frame_rate = m_capture.get(CV_CAP_PROP_FPS);
	LOG_DEBUG(_T("original frame rate = %f"), frame_rate);
	frame_rate = 1000.0 / (double)(m_frame_interval);
	m_capture.set(CV_CAP_PROP_FPS, frame_rate);
	frame_rate = m_capture.get(CV_CAP_PROP_FPS);
	LOG_DEBUG(_T("new frame rate = %f"), frame_rate);

	MakeAffineMatrix(m_rotation, m_mirror, false);

	// auto size mode
	SetWindowPos(NULL, 0, 0, m_show_width, m_show_height, 
		SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_SHOWWINDOW);

	UINT id = SetTimer(VIDEO_TIMER, m_frame_interval, NULL);
	return true;
}

void CBarCodeReaderCtrl::Close(void)
{
	KillTimer(VIDEO_TIMER);
	if (m_capture.isOpened()) m_capture.release();
}

size_t CBarCodeReaderCtrl::EnumerateCameras(StringArray & cameras)
{
	//枚举视频扑捉设备
	ICreateDevEnum *pCreateDevEnum;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum);
	if(hr != NOERROR)return -1;

	CComPtr<IEnumMoniker> pEm;
	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, 0);
	if(hr != NOERROR)return -1;

	pEm->Reset();

	int id = 0;
	ULONG cFetched;
	while(1)
	{
		CComPtr<IMoniker> pM;
		hr = pEm->Next(1, &pM, &cFetched);
		if (hr!= S_OK) break;
		if (cFetched != 1) continue;

		CComPtr<IPropertyBag> pBag;
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pBag);
		if( !SUCCEEDED(hr) )  continue;
		
		VARIANT var;
		var.vt = VT_BSTR;
		hr = pBag->Read(L"FriendlyName", &var, NULL);
		if (hr != NOERROR)	continue;
		id++;
		UINT str_len = ::SysStringLen(var.bstrVal);
		CJCStringT str((wchar_t*)(var.bstrVal), str_len);
		cameras.push_back(str);
		SysFreeString(var.bstrVal);
	}
	return id;
}

void CBarCodeReaderCtrl::MakeAffineMatrix(int rotation, bool mirror_h, bool mirror_v)
{
	int width_for_mirror = 0;

	cv::Mat rotate_mat(3,3, CV_32FC1);
	cv::Mat aff;
	switch (rotation & 0x3)
	{
	case 0:
		rotate_mat = (cv::Mat_<double>(3,3) <<
			1, 0, 0,
			0, 1, 0,
			0, 0, 1);
		width_for_mirror = m_video_width;
		m_show_width = m_video_width / m_show_scale;
		m_show_height = m_video_height / m_show_scale;
		break;

	case 1:		// 90 deg
		rotate_mat = (cv::Mat_<double>(3,3) << 
			0, 1, 0, 
			-1, 0, m_video_width, 
			0, 0, 1);
		width_for_mirror = m_video_height;
		m_show_height = m_video_width / m_show_scale;
		m_show_width = m_video_height / m_show_scale;
		break;

	case 2:		// 180 deg
		rotate_mat = (cv::Mat_<double>(3,3) <<
			-1, 0, m_video_width,
			0, -1, m_video_height,
			0, 0, 1);
		width_for_mirror = m_video_width;
		m_show_width = m_video_width / m_show_scale;
		m_show_height = m_video_height / m_show_scale;
		break;

	case 3:		// 270 deg
		rotate_mat = (cv::Mat_<double>(3,3) <<
			0, -1, m_video_height,
			1, 0, 0,
			0, 0, 1);
		width_for_mirror = m_video_height;
		m_show_height = m_video_width / m_show_scale;
		m_show_width = m_video_height / m_show_scale;
		break;

	default:	// for test
		break;
	}
	LOG_OUTPUT_MAT(_T("rotate mat"), rotate_mat); 

	if (mirror_h)
	{
		cv::Mat mirror=(cv::Mat_<double>(3,3) <<
			-1, 0, width_for_mirror,
			0, 1, 0,
			0, 0, 1);
		aff = (mirror * rotate_mat) / m_show_scale;
	}
	else aff = rotate_mat / m_show_scale;
	LOG_OUTPUT_MAT(_T("mirroed "), aff); 

	m_affine = cv::Mat(aff, cv::Rect(0, 0, 3, 2)).clone();
	LOG_OUTPUT_MAT(_T("affine "), m_affine); 
}


void CBarCodeReaderCtrl::SetBCRProperty(UINT prop, double val)
{
	switch (prop)
	{
	case BCR_MIRROR: 
		m_mirror = (val>=1); 
		MakeAffineMatrix(m_rotation, m_mirror, false);
		break;
	// 保持时间，单位ms。转换为frame
	case BCR_HOLD_TIME: m_hold_time = (int)(val) / m_frame_interval; break;
	case BCR_FRAME_RATE: 
		m_frame_interval = (int)(1000.0/val);
		if (m_capture.isOpened())
		{
			KillTimer(VIDEO_TIMER);
			m_capture.set(CV_CAP_PROP_FPS, val);
			SetTimer(VIDEO_TIMER, m_frame_interval, NULL);
		}
		break;
	case BCR_ROTATION:
		m_rotation = (int)(val);
		MakeAffineMatrix(m_rotation, m_mirror, false);
		if (m_show_width !=0 && m_show_height!=0)
		{
			SetWindowPos(NULL, 0, 0, m_show_width, m_show_height, 
				SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_SHOWWINDOW);
		}
		break;
	};
}

void CBarCodeReaderCtrl::OnSymbolDetected(const std::string & symbol)
{
	int dlg_id = GetDlgCtrlID();
	HWND hwnd = GetSafeHwnd(); 

	BRC_SYMBOL_DETECT symbol_msg;
	symbol_msg.hdr.code = BRN_SYMBOL_DETECT;
	symbol_msg.hdr.hwndFrom = hwnd;
	symbol_msg.hdr.idFrom = dlg_id;
	symbol_msg.symbol_data = symbol;

	GetParent()->SendNotifyMessage(WM_NOTIFY, 
		dlg_id, (LPARAM)(&symbol_msg) );
}

void PointAffine(cv::Point & out, int x, int y, const cv::Mat & tran)
{
	cv::Mat pp = (cv::Mat_<double>(3, 1) << x, y, 1);
	//LOG_OUTPUT_MAT(_T("point"), pp);
	cv::Mat p1 = tran * pp;
	//LOG_OUTPUT_MAT(_T("new point"), p1);
	out.x = (int)(p1.at<double>(0, 0));
	out.y = (int)(p1.at<double>(1, 0));
}

void CBarCodeReaderCtrl::UpdateFrame(cv::Mat & img)
{
	LOG_STACK_TRACE();
	// convert to gray
	cv::cvtColor(img, img, CV_BGR2GRAY);
	// zbar decode
	zbar::ImageScanner scanner;
	scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_ENABLE, 1);
	zbar::Image zimg(img.cols, img.rows, "Y800", img.data, img.cols * img.rows);
	int n = scanner.scan(zimg);

	m_frame_show.release();

	// create image for show
	cv::Mat tmp;
	cv::warpAffine(img, tmp, m_affine, cv::Size(m_show_width, m_show_height) );
	cv::cvtColor(tmp, m_frame_show, CV_GRAY2BGR);

	if (n>0)
	{
		zbar::Image::SymbolIterator symbol = zimg.symbol_begin();
		std::string str_symbol = symbol->get_data();
		if (m_cur_symbol != str_symbol)
		{
			m_cur_symbol = str_symbol;
			m_detect_hold = m_hold_time;
			OnSymbolDetected(m_cur_symbol);
		}
		// 画出识别区域
		int location_size = symbol->get_location_size();

		cv::Point p0;
		PointAffine(p0, symbol->get_location_x(location_size-1), symbol->get_location_y(location_size-1), m_affine);
		for (int ii=0; ii<location_size; ii++)
		{
			cv::Point p1;
			PointAffine(p1, symbol->get_location_x(ii), symbol->get_location_y(ii), m_affine);
			cv::line(m_frame_show, p0, p1, cv::Scalar(0, 255, 0), 1);
			p0 = p1;
		}
	}
	else
	{
		if (!m_cur_symbol.empty())
		{
			if (m_detect_hold > 0)	m_detect_hold--;
			else	m_cur_symbol.clear();
		}
	}
	RedrawWindow();
}	
