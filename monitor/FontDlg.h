#if !defined(AFX_FONTDLG_H__89034184_397E_11D5_BC38_00010304A6EA__INCLUDED_)
#define AFX_FONTDLG_H__89034184_397E_11D5_BC38_00010304A6EA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FontDlg.h : header file
//

#include "ListCtrlEdit.h"

/////////////////////////////////////////////////////////////////////////////
// CFontDlg dialog

class CFontDlg : public CDialog
{
// Construction
public:
	CFontDlg(CWnd* pParent = NULL);   // standard constructor
	void LoadFontConfig(char *str);

// Dialog Data
	//{{AFX_DATA(CFontDlg)
	enum { IDD = IDD_MONITOR_FONT };
	CListCtrlEdit	m_List_Signatures;
	CListCtrlEdit	m_List_Fonts;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFontDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFontDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonSave();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FONTDLG_H__89034184_397E_11D5_BC38_00010304A6EA__INCLUDED_)
