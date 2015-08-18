#pragma once

#include "afxwin.h"

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <stdext.h>

#define BRN_SYMBOL_DETECT	0x101

struct BRC_SYMBOL_DETECT
{
	NMHDR hdr;
	std::string	symbol_data;
};

typedef std::vector<CJCStringT>	StringArray;
///////////////////////////////////////////////////////////////////////////////
// -- 
class CBarCodeReaderCtrl : public CStatic
{
public:
	CBarCodeReaderCtrl(void);

protected:
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT id);
	//afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	DECLARE_MESSAGE_MAP()

public:
	enum BCR_PROP {
		BCR_MIRROR,	BCR_HOLD_TIME, BCR_FRAME_RATE,BCR_ROTATION,
	};

public:
	size_t	EnumerateCameras(StringArray & cameras);
	bool Open(int dev_id);
	void Close(void);
	void SetBCRProperty(UINT prop, double val);
	bool IsOpened(void) {return m_capture.isOpened();};

protected:
	void UpdateFrame(cv::Mat & frame);
	void ShowFrame(cv::Mat & frame, CDC * dc);
	void FillBitmapInfo( BITMAPINFO* bmi, int width, int height, int bpp, int origin );
	void OnSymbolDetected(const std::string & symbol);
	void MakeAffineMatrix(int rotation, bool mirror_h, bool mirror_v);

protected:
	cv::VideoCapture m_capture;
	cv::Mat m_frame_show;		// 用于显示的当前frame
	cv::Mat m_affine;
	int m_show_width, m_show_height;
	int m_video_width, m_video_height;

	std::string m_cur_symbol;	// 当前监测到的符号
	int m_detect_hold;
// configurations
	int m_show_scale;
	bool m_mirror;
	int m_hold_time;	// 识别到bar code以后的保持时间。在保持时间内，即使识别不到，也不会更新结果。
	int m_frame_interval;		// frame rate的倒数，单位ms
	int m_rotation;
};