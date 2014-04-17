#pragma once

// AdvHeaderCtrl.h : header file
/////////////////////////////////////////////////////////////////////////////
// CAdvHeaderCtrl window

class CAdvHeaderCtrl : public CHeaderCtrl
{
// Construction
public:
	CAdvHeaderCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	public:

// Implementation
public:
	BOOL Init(CHeaderCtrl *pHeader);
	virtual ~CAdvHeaderCtrl();

	// Generated message map functions
protected:
	void OnEndTrack(NMHDR * pNMHDR, LRESULT* pResult);
	int	 m_nSavedImage;			// Index of saved image	
	CImageList m_cImageList;	// Image list for this header control

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
