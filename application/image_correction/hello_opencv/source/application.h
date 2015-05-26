#pragma once


#include "iimage_proc.h"
#include <jcapp.h>
#include <vector>

#define MAX_STRING_LEN	256

class CProcessorBox;

///////////////////////////////////////////////////////////////////////////////
//--

class CTrackBar
{
public:
	CTrackBar(void);
	virtual ~CTrackBar(void) {};

public:
	void Create(UINT id, const char * bar_name, CProcessorBox * box, int default_val, int max);
	virtual void OnTrackBarUpdated(int val);
	void SetPosition(int val);

	static void StaticCallBack(int, void *);
	
protected:
	CProcessorBox * m_box;
	CJCStringA	m_name;
	int	m_val;
	UINT		m_id;
};

///////////////////////////////////////////////////////////////////////////////
//--

enum BOX_PROPERTY {
	BP_NONE = 0,
	BP_SHOW_WND = 0x80000000,
};

class CProcessorBox : public IProcessorBox
{
public:
	CProcessorBox(JCSIZE index, const CJCStringT & name, IImageProcessor * proc, BOX_PROPERTY prop);

	virtual ~CProcessorBox(void);

public:


public:
	// 
	virtual UINT RegistTrackBar(const char * name, int def_val, int max_val);
	virtual const char * GetNameA(void) const {return m_name.c_str();}
	virtual void OnTrackBarUpdated(UINT id, const CJCStringA & name, int val);
	virtual void SetContainer(IBoxContainer * cont)
	{m_container = cont; JCASSERT(m_container);}
	virtual void OnKeyPressed(int key_code);
	virtual void UpdateTrackBar(UINT id, int val);
	virtual void OnUpdateBox(void);

public:
	virtual void OnMouse(int mouse_event, int x, int y, int);

public:
	void OnInitialize(void);
	// 启动processor计算，并且更新窗口，返回processor是否有更新
	bool Recalculate(void);
	void GetNameT(CJCStringT & name) const {name = m_box_name;}
	void GetProcessor(IImageProcessor * & proc);
	void SetReviewScale(int scale) {m_review_scale = scale;}

	static void StaticOnMouse( int mouse_event, int x, int y, int, void* );

protected:
	void FillReviewBuf(cv::Mat & buf);

// 鼠标选取区域
protected:
	enum SELECT_STATUS {
		// 为选取， 开始选取，	选区中，		选区完成
		SS_NONE,	SS_STARTED, SS_CHANGING,	SS_SELECTED,
	}	m_select_st;
	cv::Point	m_rect1, m_rect2;

protected:
	IImageProcessor * m_processor;
	JCSIZE			m_index;
	CJCStringT		m_box_name;
	//char			m_name[MAX_STRING_LEN];
	CJCStringA		m_name;

	BOX_PROPERTY	m_property;

	std::vector<CTrackBar *>	m_track_bar_list;
	IBoxContainer * m_container;
	int m_review_scale;

	cv::Mat m_local_buf;
};

///////////////////////////////////////////////////////////////////////////////
//--

class CHelloOpenCvApp : public jcapp::CJCAppSupport<jcapp::AppArguSupport>,
	public IBoxContainer
{
public:
	CHelloOpenCvApp(void);
public:
	virtual int Initialize(void);
	virtual int Run(void);
	virtual void CleanUp(void);
	virtual LPCTSTR AppDescription(void) const {
		return _T("");
	};

public:
	virtual void SetActiveBox(IProcessorBox * box);


protected:
	void AddProcessor(const CJCStringT & name, IImageProcessor * proc, BOX_PROPERTY prop);
	// update boxes for [start] 
	void UpdateBox(JCSIZE start);
	void Connect(IImageProcessor * dst_proc, int input_id, const CJCStringT & src_name); 
	// 遍历所有processor box
	typedef bool (CHelloOpenCvApp::* ERGODIC_CALLBACK)(CProcessorBox*, void *);
	bool ErgodicProcessorBox(ERGODIC_CALLBACK call_back, void * context, CProcessorBox * &box);
	bool FindBoxByName(CProcessorBox * proc, void * name);
	bool SaveParameterTo(const CJCStringT & fn);
	bool LoadParameterFrom(const CJCStringT & fn);
	bool SaveProcessor(CProcessorBox * proc, void * context);
	bool LoadProcessor(CProcessorBox * proc, void * context);
	IImageProcessor * AddProcessor(const CJCStringT & type, const CJCStringT & name, BOX_PROPERTY prop, int src_num, ...);
	void SaveResults(void);

public:
	static const TCHAR LOG_CONFIG_FN[];
	CJCStringT	m_dummy;
	// 预览时的缩小比例，
	int	m_review_scale;
	CJCStringT	m_param_filename;
	CJCStringT	m_train_list;

protected:
	//HMODULE				m_driver_module;
	typedef std::vector<CProcessorBox *>	PROCESSOR_BOX_LIST;
	PROCESSOR_BOX_LIST	m_processor_list;
	IImageProcessor * m_result_proc;
	IProcessorBox * m_active_box;
};