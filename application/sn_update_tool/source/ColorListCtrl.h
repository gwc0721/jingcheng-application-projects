// Author:      SFr frydaysoft@gmx.de
#pragma once

#include "color.h"
#include "COMMCTRL.H"
#include "AdvHeaderCtrl.h"	// Added by ClassView
#include <afxtempl.h>
/////////////////////////////////////////////////////////////////////////////
// class CMyLParam  

class CMyLParam  
{
public:
    CMyLParam(int column_count)
	{
		m_column_count=column_count;
        m_pBackColor=new ITEM_COLOR[column_count];
        m_pTextColor=new ITEM_COLOR[column_count];
		m_MyData= NULL;
        for (int x=0; x<m_column_count; x++)
        {
            m_pBackColor[x]=DEF_BACKGROUND;
            m_pTextColor[x]=DEF_TEXT;
        }
	}

	virtual   ~CMyLParam()                      
    {
        if (m_pBackColor) delete[] m_pBackColor;
        if (m_pTextColor) delete[] m_pTextColor;
    }

    BOOL  SetBackColor(ITEM_COLOR color, int iSub)	{   if (iSub<m_column_count) { m_pBackColor[iSub] = color; return TRUE;} else return FALSE;   }

    BOOL  SetTextColor(ITEM_COLOR color, int iSub)	{   if (iSub<m_column_count) { m_pTextColor[iSub] = color; return TRUE;} else return FALSE;   }
    
    COLORREF  GetBackColor(int iSub)  { if (iSub<m_column_count) return GetColorRef(m_pBackColor[iSub]); else return GetColorRef(DEF_BACKGROUND);}
	
    COLORREF  GetTextColor(int iSub)  { if (iSub<m_column_count) return GetColorRef(m_pTextColor[iSub]); else return GetColorRef(DEF_TEXT);}

	void	  SetData(DWORD Mydata)	            {m_MyData	= Mydata;}
	DWORD	  GetData()			                {return DWORD(m_MyData);}

protected:
	DWORD		 m_MyData;
	ITEM_COLOR	*m_pBackColor;
	ITEM_COLOR	*m_pTextColor;
    int          m_column_count;
};

/////////////////////////////////////////////////////////////////////////////
// class CColorListCtrl 

#define LISTCTRL_ERROR 0xFFFFFFFF

class CColorListCtrl : public CListCtrl
{
    int  m_columnSize;
    BOOL m_fullColumnLines;
    BOOL m_withSingleRect;
    BOOL m_BigFocusRect;
    BOOL IsCreated;

    // get Rect of single (sub)item
    CRect GetCellRect(int nItem, int nColumn, BOOL outside);
    // get Rect of Item-line
    CRect GetFullCellRect(int nItem, BOOL outside);
    // return internal data-member (Colors/ItemUserPointer)
    CMyLParam*	GetMyLParam(int Index);

    void InvalidateFullItem(int nItem, BOOL bErase=TRUE);

// Konstruktion
public:
    // colSize = number of columns
	CColorListCtrl(int colSize);

    // init the head of the CListCtrl
    int InitCtrl(CStringArray *phead, CByteArray *pProcent);
    //returns:    0: successfully
    //            1: phead or pProcent==NULL
    //            2: phead.GetSize()!=number of columns
    //            3: phead and pProcent have not same GetSize()
    //            4: sum of all elements from pProcent not 100%

    // add an item (+subitems) to the Ctrl
    int AddItem(CStringArray *line, int pos=LISTCTRL_ERROR);
    // return:   LISTCTRL_ERROR for an error else index of added item

    // delete all items from Ctrl
	BOOL	DeleteAllItems();

    // delete a single items from Ctrl
	BOOL	DeleteItem(int nItem );

    // set properties 
    void SetColumnType(BOOL bNew);
    void SetRectType(BOOL bNew);
    void SetFocusType(BOOL bNew);

    BOOL GetColumnType() {return m_fullColumnLines;}
    BOOL GetRectType()   {return m_withSingleRect;}
    BOOL GetFocusType()  {return m_BigFocusRect;}


// Attribute
public:

    // return the 32-bit value associated with the item
    DWORD GetItemData( int nItem ) const;
    // set the 32-bit value associated with the item
    BOOL  SetItemData( int nItem, DWORD dwData );


    BOOL SetItemTextColor(ITEM_COLOR color, int Index, int iSub);
    BOOL SetItemBackgndColor(ITEM_COLOR color, int Index, int iSub);
    COLORREF GetItemTextColor(int Index, int iSub);
    COLORREF GetItemBackgndColor(int Index, int iSub);
    void RepaintSelectedItems();
    BOOL SetSel(int pos, BOOL bSelect);

// Operationen
public:

	BOOL SetItemState(int nItem, LV_ITEM* pItem)
    {
	    if (LB_ERR	== nItem) return FALSE;
        InvalidateFullItem(nItem, TRUE);
        return CListCtrl::SetItemState(nItem, pItem);
    }
	BOOL SetItemState(int nItem, UINT nState, UINT nMask)
    {
	    if (LB_ERR	== nItem) return FALSE;
        InvalidateFullItem(nItem, TRUE);
        return CListCtrl::SetItemState(nItem, nState, nMask);
    }

    // these functions are disabled --> constant number of columns
	int InsertColumn(int /*nCol*/, const LV_COLUMN* /*pColumn*/){return -1;}
	int InsertColumn(int /*nCol*/, LPCTSTR /*lpszColumnHeading*/,	int /*nFormat = LVCFMT_LEFT*/, int /*nWidth = -1*/, int /*nSubItem = -1*/);//{return -1;}
	BOOL DeleteColumn(int /*nCol*/);

	// Vom Klassen-Assistenten generierte virtuelle Funktions・erschreibungen
	//{{AFX_VIRTUAL(CColorListCtrl)
	//}}AFX_VIRTUAL

// Implementierung
public:
	//BOOL ColumnShow(int nCol, int *ListShowIndex, int nLength );
	BOOL ColumnHide(int nCol , int *ListShowIndex , int nLength);
	void SetMaxColumnWidth(int nCol);
	void SetMaxColumnsWidth( int *ListShowIndex , int nLength);
	int g_FontSize;
	void SetMyFontSize(int nFontSize);
	virtual ~CColorListCtrl();
	void ResizeProg();
	void SetProgress(int Index, int prog);
	void CreateProgress(int Index);

	// Generierte Nachrichtenzuordnungsfunktionen
protected:

	class CProgressEntry {
	public:
		CProgressEntry(int Index, int SubIndex);
		virtual ~CProgressEntry();
		int m_Index;
		int m_SubIndex;
		CProgressCtrl m_Prog;
	};

	CMap <int, int, CProgressEntry*, CProgressEntry*& > m_ProgEntries;

	CAdvHeaderCtrl m_AdvHdr;

	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	//{{AFX_MSG(CColorListCtrl)
	afx_msg void MeasureItem ( LPMEASUREITEMSTRUCT lpMeasureItemStruct );
	afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnWindowPosChanged(WINDOWPOS FAR* lpwndpos);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

