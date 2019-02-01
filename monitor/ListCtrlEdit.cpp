// ListCtrlEdit.cpp : implementation file
//

#include "stdafx.h"
#include "Monitor.h"
#include "ListCtrlEdit.h"
#include "InPlaceEdit.h"
#include "FontDlg.h"
#include "Printers.h"

#include <windows.h>
#include <winuser.h>
#include <winspool.h>
#include <string.h>
#include <stdlib.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CListCtrl* ListCtrlConfig;
extern CListCtrl* ListCtrlFonts;
extern CListCtrl* ListCtrlSignatures;

CEdit* pEdit;
int index, colnum;
int NumImpressora;

/////////////////////////////////////////////////////////////////////////////
// CListCtrlEdit

CListCtrlEdit::CListCtrlEdit()
{
}

CListCtrlEdit::~CListCtrlEdit()
{
}


BEGIN_MESSAGE_MAP(CListCtrlEdit, CListCtrl)
	//{{AFX_MSG_MAP(CListCtrlEdit)
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, OnEndlabeledit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

void CListCtrlEdit::FileDialogFontsConfig(void)
{
	CFileDialog *DialogBox;
	char str[512];
	int ret;

	DialogBox = new CFileDialog(TRUE, "cfg", NULL, OFN_FILEMUSTEXIST|OFN_EXPLORER,
								"Configuracions {*.cfg}|*.cfg;|All Files{*.*}|*.*||");
	
	ret = DialogBox->DoModal();
	if (ret == IDOK)
	{
		CString Path=DialogBox->GetPathName();
		strcpy(str, Path);

		ListCtrlConfig->SetItemText (index, 5, str);
	}

	delete DialogBox;
}

/////////////////////////////////////////////////////////////////////////////

// EditSubLabel		- Start edit of a sub item label
// Returns		- Temporary pointer to the new edit control
// nItem		- The row index of the item to edit
// nCol			- The column of the sub item.
CEdit* CListCtrlEdit::EditSubLabel( int nItem, int nCol )
{
	int i;
	char str[512];

	GetParent()->GetWindowText(str, 512);

	if (strcmp(str, "Monitor Xes2PCL")==0)
	{
		if (nCol==0) return pEdit; // Activa / No Activa

		if (nCol==6) // FileDialog de fitxer de configuració de fonts i signatures
		{
			FileDialogFontsConfig();

			return pEdit;
		}
		else if (nCol==4)
		{
			CPrinters dlg;

			dlg.DoModal();

			return pEdit;
		}
		else if (nCol==7)
		{
			CFontDlg dlg;

			NumImpressora=nItem;

			dlg.DoModal();

			return pEdit; // Finestra Fonts
		}
	}
	
	// The returned pointer should not be saved

	// Make sure that the item is visible
	if( !EnsureVisible( nItem, TRUE ) ) return NULL;

	// Make sure that nCol is valid
	CHeaderCtrl* pHeader = (CHeaderCtrl*)GetDlgItem(0);
	int nColumnCount = pHeader->GetItemCount();
	if( nCol >= nColumnCount || GetColumnWidth(nCol) < 5 )
		return NULL;

	// Get the column offset
	int offset = 0;
	for(i = 0; i < nCol; i++ )
		offset += GetColumnWidth( i );

	CRect rect;
	GetItemRect( nItem, &rect, LVIR_BOUNDS );

	// Now scroll if we need to expose the column
	CRect rcClient;
	GetClientRect( &rcClient );
	if( offset + rect.left < 0 || offset + rect.left > rcClient.right )
	{
		CSize size;
		size.cx = offset + rect.left;
		size.cy = 0;
		Scroll( size );
		rect.left -= size.cx;
	}

	// Get Column alignment
	LV_COLUMN lvcol;
	lvcol.mask = LVCF_FMT;
	GetColumn( nCol, &lvcol );
	DWORD dwStyle ;
	if((lvcol.fmt&LVCFMT_JUSTIFYMASK) == LVCFMT_LEFT)
		dwStyle = ES_LEFT;
	else if((lvcol.fmt&LVCFMT_JUSTIFYMASK) == LVCFMT_RIGHT)
		dwStyle = ES_RIGHT;
	else dwStyle = ES_CENTER;

	rect.left += offset+4;
	rect.right = rect.left + GetColumnWidth( nCol ) - 3 ;
	if( rect.right > rcClient.right) rect.right = rcClient.right;

	dwStyle |= WS_BORDER|WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL;
	CEdit *pEdit = new CInPlaceEdit(nItem, nCol, GetItemText( nItem, nCol ));
	pEdit->Create(dwStyle, rect, this, IDC_EDIT);

	return pEdit;
}

/////////////////////////////////////////////////////////////////////////////
// CListCtrlEdit message handlers

// HitTestEx	- Determine the row index and column index for a point
// Returns	- the row index or -1 if point is not over a row
// point	- point to be tested.
// col		- to hold the column index
int CListCtrlEdit::HitTestEx(CPoint &point, int *col) const
{
	int colnum = 0;
	int row = HitTest( point, NULL );
	
	if( col ) *col = 0;

	// Make sure that the ListView is in LVS_REPORT
	if( (GetWindowLong(m_hWnd, GWL_STYLE) & LVS_TYPEMASK) != LVS_REPORT )
		return row;

	// Get the top and bottom row visible
	row = GetTopIndex();
	int bottom = row + GetCountPerPage();
	if( bottom > GetItemCount() )
		bottom = GetItemCount();
	
	// Get the number of columns
	CHeaderCtrl* pHeader = (CHeaderCtrl*)GetDlgItem(0);
	int nColumnCount = pHeader->GetItemCount();

	// Loop through the visible rows
	for( ;row <=bottom;row++)
	{
		// Get bounding rect of item and check whether point falls in it.
		CRect rect;
		GetItemRect( row, &rect, LVIR_BOUNDS );
		if( rect.PtInRect(point) )
		{
			// Now find the column
			for( colnum = 0; colnum < nColumnCount; colnum++ )
			{
				int colwidth = GetColumnWidth(colnum);
				if( point.x >= rect.left 
					&& point.x <= (rect.left + colwidth ) )
				{
					if( col ) *col = colnum;
					return row;
				}
				rect.left += colwidth;
			}
		}
	}
	return -1;
}


void CListCtrlEdit::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CListCtrl::OnLButtonDown(nFlags, point);

	if ((index = HitTestEx(point, &colnum))!=-1)
	{
		if( GetWindowLong(m_hWnd, GWL_STYLE) & LVS_EDITLABELS )
			pEdit=EditSubLabel(index, colnum);
	}
}

void CListCtrlEdit::OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	char str[256];
	int num;

	GetParent()->GetWindowText(str, 512);

	// Si estem editant la darrera línia per primera vegada (estem inserint una línia), cal afegir-ne una de nova a sota

	num=GetItemCount();

	if (index+1==num)
	{
		if (strcmp(str, "Monitor Xes2PCL")==0)
		{
			ListCtrlConfig->InsertItem  (num, " ");
			ListCtrlConfig->SetItemText (num, 1, " ");
		}
		else if (strcmp(str, "Fonts i Signatures")==0)
		{
			if (this==ListCtrlFonts)
			{
				ListCtrlFonts->InsertItem  (num,    " ");
				ListCtrlFonts->SetItemText (num, 1, " ");
			}
			else if (this==ListCtrlSignatures)
			{
				ListCtrlSignatures->InsertItem  (num,    " ");
				ListCtrlSignatures->SetItemText (num, 1, " ");
			}
		}
	}

	pEdit->GetWindowText(str, 256);

	SetItemText(index, colnum, str);

	*pResult = 0;
}
