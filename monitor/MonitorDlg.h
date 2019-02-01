// MonitorDlg.h : header file
//

#define MAXLOGITEMS       100

#if !defined(AFX_MONITORDLG_H__51ED7D46_282B_11D5_BC2E_00010304A6EA__INCLUDED_)
#define AFX_MONITORDLG_H__51ED7D46_282B_11D5_BC2E_00010304A6EA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ListCtrlEdit.h"

/////////////////////////////////////////////////////////////////////////////
// CMonitorDlg dialog

class CMonitorDlg : public CDialog
{
// Construction
public:
	CMonitorDlg(CWnd* pParent = NULL);	// standard constructor
	void LoadConfig(void);
	BOOL CMonitorDlg::InitImageList(void);

// Dialog Data
	//{{AFX_DATA(CMonitorDlg)
	enum { IDD = IDD_MONITOR_DIALOG };
	CListCtrlEdit	m_List_Config;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMonitorDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CMonitorDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnButtonConfigFile();
	afx_msg void OnButtonSave();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MONITORDLG_H__51ED7D46_282B_11D5_BC2E_00010304A6EA__INCLUDED_)
