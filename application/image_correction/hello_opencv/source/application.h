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

	static void StaticCallBack(int, void *);
	
protected:
	CProcessorBox * m_box;
	CJCStringA	m_name;
	int	m_val;
	UINT		m_id;
};

///////////////////////////////////////////////////////////////////////////////
//--


class CProcessorBox : public IProcessorBox
{
public:
	CProcessorBox(JCSIZE index, const CJCStringT & name, IImageProcessor * proc);
	virtual ~CProcessorBox(void);

public:
	// 
	virtual UINT RegistTrackBar(const char * name, int def_val, int max_val);
	virtual const char * GetNameA(void) const {return m_name.c_str();}
	virtual void OnTrackBarUpdated(UINT id, const CJCStringA & name, int val);


public:
	void OnInitialize(void);
	// 启动processor计算，并且更新窗口，返回processor是否有更新
	bool OnUpdateBox(void);
	void GetNameT(CJCStringT & name) const {name = m_box_name;}
	void GetProcessor(IImageProcessor * & proc);

protected:
	IImageProcessor * m_processor;
	JCSIZE			m_index;
	CJCStringT		m_box_name;
	//char			m_name[MAX_STRING_LEN];
	CJCStringA		m_name;

	std::vector<CTrackBar *>	m_track_bar_list;
};

///////////////////////////////////////////////////////////////////////////////
//--

class CHelloOpenCvApp : public jcapp::CJCAppSupport<jcapp::AppArguSupport>
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

protected:
	void AddProcessor(const CJCStringT & name, IImageProcessor * proc);
	// update boxes for [start] 
	void UpdateBox(JCSIZE start);
	void Connect(IImageProcessor * dst_proc, int input_id, const CJCStringT & src_name); 
	// 遍历所有processor box
	typedef bool (CHelloOpenCvApp::* ERGODIC_CALLBACK)(CProcessorBox*, void *);
	bool ErgodicProcessorBox(ERGODIC_CALLBACK call_back, void * context, CProcessorBox * &box);
	bool FindBoxByName(CProcessorBox * proc, void * name);
	bool SaveParameterTo(const CJCStringT & fn);
	bool SaveProcessor(CProcessorBox * proc, void * context);

public:
	static const TCHAR LOG_CONFIG_FN[];
	CJCStringT	m_dummy;

protected:
	//HMODULE				m_driver_module;
	typedef std::vector<CProcessorBox *>	PROCESSOR_BOX_LIST;
	PROCESSOR_BOX_LIST	m_processor_list;
};