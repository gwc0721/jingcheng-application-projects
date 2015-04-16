#include <jcapp.h>

#include "iimage_proc.h"
#include <vector>

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

public:
	static const TCHAR LOG_CONFIG_FN[];
	CJCStringT	m_dummy;

protected:
	//HMODULE				m_driver_module;
	std::vector<IImageProcesser *>	m_processor_list;
};