#if !defined(AFX_LISTCTRLEDIT_H__3EDA6CEF_28C5_11D5_BC2F_00010304A6EA__INCLUDED_)
#define AFX_LISTCTRLEDIT_H__3EDA6CEF_28C5_11D5_BC2F_00010304A6EA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ListCtrlEdit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CListCtrlEdit window

class CListCtrlEdit : public CListCtrl
{
// Construction
public:
	CListCtrlEdit();
	void ChoosePrinter(void);
	void FileDialogFontsConfig(void);
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListCtrlEdit)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CListCtrlEdit();
	int HitTestEx(CPoint &point, int *col) const;
	CEdit* EditSubLabel( int nItem, int nCol );
	// Generated message map functions
protected:
	//{{AFX_MSG(CListCtrlEdit)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LISTCTRLEDIT_H__3EDA6CEF_28C5_11D5_BC2F_00010304A6EA__INCLUDED_)
