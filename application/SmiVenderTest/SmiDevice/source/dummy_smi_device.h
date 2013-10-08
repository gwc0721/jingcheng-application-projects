#pragma once

class CDummySmiDevice 
	: virtual public ISmiDevice
	, public CSmiDeviceBase
	, public CJCInterfaceBase
{
protected:
	CDummySmiDevice(IStorageDevice * dev)
		: m_dev(dev)
	{
		JCASSERT(m_dev);
		m_dev->AddRef();
	}

	virtual ~CDummySmiDevice(void)
	{
		if (m_dev)		m_dev->Release();
	}

	virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr)
	{
		JCASSERT(NULL == if_ptr);
		bool br = false;
		if (if_name == IF_NAME_STORAGE_DEVICE ||
			strcmp(if_name, IF_NAME_STORAGE_DEVICE) == 0)
		{
			if_ptr = static_cast<IJCInterface*>(m_dev);
			m_dev->AddRef();
			br = true;
		}
		else 
		{
			br = __super::QueryInterface(if_name, if_ptr);
			if (!br) br = m_dev->QueryInterface(if_name, if_ptr);
		}
		return br;
	}

public:
	static bool CreateDevice(IStorageDevice * storage, ISmiDevice *& smi_device)
	{
		JCASSERT(NULL == smi_device);
		JCASSERT(storage);
		smi_device =  static_cast<ISmiDevice*>(new CDummySmiDevice(storage));
		return true;
	}

	static bool Recognize(IStorageDevice * storage, BYTE * inquery)
	{
		return false;
	}

protected:
	IStorageDevice * m_dev;
};