// Printers.cpp : implementation file
//

#include "stdafx.h"
#include "Monitor.h"
#include "Printers.h"

#include <windows.h>
#include <winspool.h>
#include <string.h>
#include <stdlib.h>

#define DEFAULTDOMINI   "INTRANET"
#define DEFAULTSERVIDOR "PTBRCUES1"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CListCtrl* ListCtrlConfig;
extern int index;

/////////////////////////////////////////////////////////////////////////////
// CPrinters dialog


CPrinters::CPrinters(CWnd* pParent /*=NULL*/)
	: CDialog(CPrinters::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPrinters)
	//}}AFX_DATA_INIT
}


void CPrinters::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPrinters)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPrinters, CDialog)
	//{{AFX_MSG_MAP(CPrinters)
	ON_EN_KILLFOCUS(IDC_EDIT_SERVIDOR, OnKillfocusEditServidor)
	ON_EN_KILLFOCUS(IDC_EDIT_DOMINI, OnKillfocusEditDomini)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_PRINTERS, OnDblclkListPrinters)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ComparePrinter

int ComparePrinter(const void *arg1, const void *arg2)
{
	return _stricmp(((LPPRINTER_INFO_1)arg1)->pName, ((LPPRINTER_INFO_1)arg2)->pName);
}

/////////////////////////////////////////////////////////////////////////////

void CPrinters::UpdatePrinters(void)
{
	DWORD				dwSizeNeeded, dwNumItems, dwItem, ret;
	LPPRINTER_INFO_1	lpInfo = NULL;
	char				NTDomain[256], NTPrintServer[256], PrinterName[256];

	CListCtrl* ListPrinters = (CListCtrl*)GetDlgItem(IDC_LIST_PRINTERS);

	ListPrinters -> DeleteAllItems();

	CEdit* EditCtrl;
	
	EditCtrl=(CEdit*)GetDlgItem(IDC_EDIT_DOMINI);
	EditCtrl->SetSel(0,-1,0);
	EditCtrl->GetWindowText(NTDomain, 512);

	EditCtrl=(CEdit*)GetDlgItem(IDC_EDIT_SERVIDOR);
	EditCtrl->SetSel(0,-1,0);
	EditCtrl->GetWindowText(NTPrintServer, 512);

	sprintf(PrinterName, "Impresoras remotas de Windows NT!%s!\\\\%s", NTDomain, NTPrintServer);
	//sprintf(PrinterName, "Impresoras remotas de Windows NT!%s", NTDomain);

	ret=EnumPrinters(PRINTER_ENUM_NAME, PrinterName, 1, NULL, 0, &dwSizeNeeded, NULL);

	// Allocate memory
	lpInfo=(LPPRINTER_INFO_1)HeapAlloc(GetProcessHeap (), HEAP_ZERO_MEMORY, dwSizeNeeded);
	if (lpInfo==NULL ) return;

	ret=EnumPrinters(PRINTER_ENUM_NAME, PrinterName, 1, (LPBYTE)lpInfo, dwSizeNeeded, &dwSizeNeeded, &dwNumItems);
	if (ret==0) return;

	qsort(lpInfo, dwNumItems, sizeof(PRINTER_INFO_1), ComparePrinter);

	// display printers 
	for (dwItem = 0; (dwItem<dwNumItems); dwItem++)
	{
		ListPrinters->InsertItem  (dwItem, "");

		ListPrinters->SetItemText (dwItem, 0, lpInfo[dwItem].pName);
		ListPrinters->SetItemText (dwItem, 1, lpInfo[dwItem].pDescription);
	}

	// free memory
	HeapFree ( GetProcessHeap (), 0, lpInfo );
}

/////////////////////////////////////////////////////////////////////////////

BOOL CPrinters::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CListCtrl* ListPrinters = (CListCtrl*)GetDlgItem(IDC_LIST_PRINTERS);

	ListView_SetExtendedListViewStyle(ListPrinters->m_hWnd, WS_VISIBLE | LVS_REPORT | LVS_EX_GRIDLINES);

	ListPrinters->InsertColumn(0, _T("Nom"),        LVCFMT_LEFT, 200);
	ListPrinters->InsertColumn(1, _T("Descripció"), LVCFMT_LEFT, 300);

	CEdit* EditCtrl;
	
	EditCtrl=(CEdit*)GetDlgItem(IDC_EDIT_DOMINI);
	EditCtrl->SetSel(0,-1,0);
	EditCtrl->ReplaceSel(DEFAULTDOMINI);

	EditCtrl=(CEdit*)GetDlgItem(IDC_EDIT_SERVIDOR);
	EditCtrl->SetSel(0,-1,0);
	EditCtrl->ReplaceSel(DEFAULTSERVIDOR);

	UpdatePrinters();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////

void CPrinters::OnKillfocusEditServidor() 
{
	UpdatePrinters();
}

/////////////////////////////////////////////////////////////////////////////

void CPrinters::OnKillfocusEditDomini() 
{
	UpdatePrinters();
}

/////////////////////////////////////////////////////////////////////////////

void CPrinters::OnDblclkListPrinters(NMHDR* pNMHDR, LRESULT* pResult) 
{
	char str[512];

	CListCtrl* ListPrinters = (CListCtrl*)GetDlgItem(IDC_LIST_PRINTERS);

	ListPrinters->GetItemText (ListPrinters->GetNextItem(-1,LVNI_SELECTED), 0, str, 512);

	ListCtrlConfig->SetItemText (index, 3, str);

	*pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////

