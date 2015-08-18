
// barcode_reader.cpp : 定义应用程序的类行为。
//

#include "stdafx.h"
#include "barcode_reader.h"
#include "barcode_reader_dlg.h"

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/highgui/highgui.hpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CBarCodeReaderApp

BEGIN_MESSAGE_MAP(CBarCodeReaderApp, CWinAppEx)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CBarCodeReaderApp 构造

CBarCodeReaderApp::CBarCodeReaderApp()
{
	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的一个 CBarCodeReaderApp 对象

CBarCodeReaderApp theApp;


// CBarCodeReaderApp 初始化

BOOL CBarCodeReaderApp::InitInstance()
{
	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControlsEx()。否则，将无法创建窗口。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 将它设置为包括所有要在应用程序中使用的
	// 公共控件类。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	AfxEnableControlContainer();

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO: 应适当修改该字符串，
	// 例如修改为公司或组织名
	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));

#if 0
	// opencv video test
	cv::namedWindow("win");

	cv::VideoCapture cap;
	cap.open(0);

	if (!cap.isOpened())
	{
		MessageBox(NULL, _T("open capture failed!"), _T("failed"), MB_OK);
		return FALSE;
	}

	cap.set(CV_CAP_PROP_FPS, 10);
	int width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	int height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);

	cv::Mat map_x, map_y;
	map_x.create(cv::Size(width, height), CV_32FC1);
	map_y.create(cv::Size(width, height), CV_32FC1);
	
	for (int yy=0; yy<height; ++yy)
	{
		for (int xx=0; xx<width; ++xx)
		{
			map_x.at<float>(yy,xx) = width - xx;
			map_y.at<float>(yy,xx) = yy;
		}
	}

    while(1) 
	{
		cv::Mat frame, dst;
		cap >> frame;
		cv::cvtColor(frame, frame, CV_BGR2GRAY);
		cv::remap(frame, dst, map_x, map_y, CV_INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
		
		cv::imshow("win", dst);
		Sleep(100);

		char c = cv::waitKey(50);
        if(c==27) break;
    }
	cv::destroyWindow("win");
	cap.release();
#endif

	CBarCodeReaderDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: 在此放置处理何时用
		//  “确定”来关闭对话框的代码
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: 在此放置处理何时用
		//  “取消”来关闭对话框的代码
	}

	// 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
	//  而不是启动应用程序的消息泵。
	return FALSE;
}
