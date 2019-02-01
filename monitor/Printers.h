#if !defined(AFX_PRINTERS_H__31241E20_3AE1_11D5_BC3A_00010304A6EA__INCLUDED_)
#define AFX_PRINTERS_H__31241E20_3AE1_11D5_BC3A_00010304A6EA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Printers.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPrinters dialog

class CPrinters : public CDialog
{
// Construction
public:
	CPrinters(CWnd* pParent = NULL);   // standard constructor
	void UpdatePrinters(void);

// Dialog Data
	//{{AFX_DATA(CPrinters)
	enum { IDD = IDD_MONITOR_PRINTERS };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPrinters)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPrinters)
	virtual BOOL OnInitDialog();
	afx_msg void OnKillfocusEditServidor();
	afx_msg void OnKillfocusEditDomini();
	afx_msg void OnDblclkListPrinters(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PRINTERS_H__31241E20_3AE1_11D5_BC3A_00010304A6EA__INCLUDED_)
