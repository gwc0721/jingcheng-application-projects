#pragma once

class CSmiPluginBase
{
protected:
	CSmiPluginBase() : m_smi_dev(NULL) {};
protected:
	JCSIZE	BlockNum(void) {return m_card_info.m_f_block_num; }
	JCSIZE	PageSize(void) {return m_card_info.m_f_ckpp * m_card_info.m_f_spck; }
	JCSIZE	PageNum(void)	{return m_card_info.m_f_ppb;}
	JCSIZE	BlockSize(void) {return PageSize() * m_card_info.m_f_ppb; }

	JCSIZE	ChunkSize(void)	{return m_card_info.m_f_spck;}

	void IncreaseFlashAddress(CFlashAddress & add)
	{
		add.m_chunk ++;
		if (add.m_chunk >= m_card_info.m_f_ckpp)
		{
			add.m_chunk = 0;
			add.m_page ++;
			if (add.m_page >= m_card_info.m_f_ppb)
			{
				add.m_page = 0;
				add.m_block ++;
			}
		}
	};

protected:
	ISmiDevice	* m_smi_dev;
	CCardInfo	m_card_info;
};


