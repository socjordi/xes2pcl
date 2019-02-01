// FontDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Monitor.h"
#include "FontDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CListCtrl* ListCtrlFonts;
CListCtrl* ListCtrlSignatures;

extern int NumImpressora;
extern CListCtrl* ListCtrlConfig;

int ReadLine(FILE *fp, char *linia);

/////////////////////////////////////////////////////////////////////////////
// CFontDlg dialog


CFontDlg::CFontDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFontDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFontDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CFontDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFontDlg)
	DDX_Control(pDX, IDC_LIST_SIGNATURES, m_List_Signatures);
	DDX_Control(pDX, IDC_LIST_FONTS, m_List_Fonts);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFontDlg, CDialog)
	//{{AFX_MSG_MAP(CFontDlg)
	ON_BN_CLICKED(IDC_BUTTON_SAVE, OnButtonSave)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

BOOL ExistFile(char *str)
{
	FILE *fp;

	fp=fopen(str, "rb"); if (fp==NULL) return 0;
	fclose(fp);

	return 1;
}

/////////////////////////////////////////////////////////////////////////////

void CFontDlg::LoadFontConfig(char *str)
{
	char strtipus, str0[512], str1[512], str2[512], str3[512], str4[512], linia[512];
	int NumFonts, NumSignatures;
	CEdit* EditCtrl;
	FILE *fp;

	fp=fopen(str, "rt");
	if (fp==NULL)
	{
		sprintf(str0, "ERROR: No es pot obrir el fitxer \"%s\" per lectura", str);
		MessageBox(str0);

		return;
	}

	ListCtrlFonts      -> DeleteAllItems();
	ListCtrlSignatures -> DeleteAllItems();

	NumFonts=0;
	NumSignatures=0;

	while (1)
	{
		if (ReadLine(fp, linia)<0) break;

		if (linia[0]=='#') continue;
		if (strlen(linia)==0) continue;

		strtipus = strtok(linia, "\t")[0];

		if (strtipus=='F') // Definició de font
		{
			// F	Titan10iso-P	(s0p10h10V	P	50

			strcpy(str0, strtok(NULL,  "\t"));
			strcpy(str1, strtok(NULL,  "\t"));
			strcpy(str2, strtok(NULL,  "\t"));
			strcpy(str3, strtok(NULL,  "\t"));
			strcpy(str4, strtok(NULL,  "\t"));

			ListCtrlFonts->InsertItem  (NumFonts, "");

			ListCtrlFonts->SetItemText (NumFonts, 0, str0);
			ListCtrlFonts->SetItemText (NumFonts, 1, str1);
			ListCtrlFonts->SetItemText (NumFonts, 2, str2);
			ListCtrlFonts->SetItemText (NumFonts, 3, str3);
			ListCtrlFonts->SetItemText (NumFonts, 4, str4);

			NumFonts++;
		}
		else if (strtipus=='S') // Definició de signatura
		{
			// S	ESCUT-P	A	E:\XES2PCL\Recursos\IMPRESSORA1\escut.prn	20	20

			strcpy(str0, strtok(NULL,  "\t"));
			strcpy(str1, strtok(NULL,  "\t"));
			strcpy(str2, strtok(NULL,  "\t"));
			strcpy(str3, strtok(NULL,  "\t"));
			strcpy(str4, strtok(NULL,  "\t"));

			ListCtrlSignatures->InsertItem  (NumSignatures, "");

			ListCtrlSignatures->SetItemText (NumSignatures, 0, str0);
			ListCtrlSignatures->SetItemText (NumSignatures, 1, str1);
			ListCtrlSignatures->SetItemText (NumSignatures, 2, str2);
			ListCtrlSignatures->SetItemText (NumSignatures, 3, str3);
			ListCtrlSignatures->SetItemText (NumSignatures, 4, str4);

			NumSignatures++;
		}
		else if (strtipus=='T') // Definició de safata
		{
			//	T	Sup	1
			//	T	Inf	2
			//	T	Man	5

			strcpy(str0, strtok(NULL,  "\t"));
			strcpy(str1, strtok(NULL,  "\t"));

			if      (strcmp(str0,"Sup")==0)
				EditCtrl=(CEdit*)GetDlgItem(IDC_SAFATA_SUP);
			else if (strcmp(str0,"Inf")==0)
				EditCtrl=(CEdit*)GetDlgItem(IDC_SAFATA_INF);
			else if (strcmp(str0,"Man")==0)
				EditCtrl=(CEdit*)GetDlgItem(IDC_SAFATA_MAN);
			else
			{
				sprintf(str0, "ERROR: Hi ha una definició de safata desconeguda: \"%s\"", linia);
				MessageBox(str0);
			}
			
			EditCtrl->SetSel(0,-1,0);
			EditCtrl->ReplaceSel(str1);
		}
		else
		{
			sprintf(str0, "ERROR: Hi ha una línia que no és ni signatura (S) ni font (F) ni safata (T): \"%s\"", linia);
			MessageBox(str0);
		}
	}

	ListCtrlFonts     ->InsertItem  (NumFonts,      " ");

	ListCtrlFonts     ->SetItemText (NumFonts,      0, " ");
	ListCtrlFonts     ->SetItemText (NumFonts,      1, " ");
	ListCtrlFonts     ->SetItemText (NumFonts,      2, " ");
	ListCtrlFonts     ->SetItemText (NumFonts,      3, " ");
	ListCtrlFonts     ->SetItemText (NumFonts,      4, " ");

	ListCtrlSignatures->InsertItem  (NumSignatures, " ");

	ListCtrlSignatures->SetItemText (NumSignatures, 0, " ");
	ListCtrlSignatures->SetItemText (NumSignatures, 1, " ");
	ListCtrlSignatures->SetItemText (NumSignatures, 2, " ");
	ListCtrlSignatures->SetItemText (NumSignatures, 3, " ");
	ListCtrlSignatures->SetItemText (NumSignatures, 4, " ");

	fclose(fp);
}

/////////////////////////////////////////////////////////////////////////////
// CFontDlg message handlers

BOOL CFontDlg::OnInitDialog() 
{
	char str[512], str2[512];

	ListCtrlConfig->GetItemText(NumImpressora, 5, str, 512);
	if (ExistFile(str)==0)
	{
		sprintf(str2, "El fitxer de configuració %s no existeix", str);
		MessageBox(str2);
		EndDialog(0);
		return TRUE;
	}

	CDialog::OnInitDialog();

	// Configura els controls

	ListCtrlFonts      = (CListCtrl*)GetDlgItem(IDC_LIST_FONTS);
	ListCtrlSignatures = (CListCtrl*)GetDlgItem(IDC_LIST_SIGNATURES);

	ModifyStyle(0, WS_SIZEBOX);					// Permet canviar la mida de la finestra mare
	ListCtrlFonts     ->ModifyStyle(0, WS_SIZEBOX|LVS_EDITLABELS);
	ListCtrlSignatures->ModifyStyle(0, WS_SIZEBOX|LVS_EDITLABELS);

	ListView_SetExtendedListViewStyle(ListCtrlFonts     ->m_hWnd, WS_VISIBLE | LVS_REPORT | LVS_EX_GRIDLINES);
	ListView_SetExtendedListViewStyle(ListCtrlSignatures->m_hWnd, WS_VISIBLE | LVS_REPORT | LVS_EX_GRIDLINES);
	
	ListCtrlFonts     ->InsertColumn(0, _T("Nom"),        LVCFMT_LEFT,  150);
	ListCtrlFonts     ->InsertColumn(1, _T("Codis PCL"),  LVCFMT_LEFT,  250);
	ListCtrlFonts     ->InsertColumn(2, _T("P/L"),        LVCFMT_CENTER, 30);
	ListCtrlFonts     ->InsertColumn(3, _T("Amplada"),    LVCFMT_CENTER, 60);
	ListCtrlFonts     ->InsertColumn(4, _T("Interlínia"), LVCFMT_CENTER, 60);

	ListCtrlSignatures->InsertColumn(0, _T("Nom"),        LVCFMT_LEFT,  100);
	ListCtrlSignatures->InsertColumn(1, _T("Lletra"),     LVCFMT_CENTER, 40);
	ListCtrlSignatures->InsertColumn(2, _T("Camí macro"), LVCFMT_LEFT,  300);
	ListCtrlSignatures->InsertColumn(3, _T("DX"),         LVCFMT_CENTER, 50);
	ListCtrlSignatures->InsertColumn(4, _T("DY"),         LVCFMT_CENTER, 50);

	ListCtrlConfig->GetItemText(NumImpressora, 1, str, 512);
	GetDlgItem(IDC_IMPRESSORA_XES)->SetWindowText(str);

	ListCtrlConfig->GetItemText(NumImpressora, 5, str, 512);
	GetDlgItem(IDC_CONFIG_FONTS)->SetWindowText(str);

	LoadFontConfig(str);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////

void CFontDlg::OnButtonSave() 
{
	// Guardar configuració de la font	

	char linia[512], str0[512], str1[512], str2[512], str3[512], str4[512];
	CEdit* EditCtrl;
	FILE *fp;
	int i;

	ListCtrlConfig->GetItemText(NumImpressora, 5, str0, 512);

	fp=fopen(str0, "wt");
	if (fp==NULL)
	{
		sprintf(linia, "ERROR: No es pot obrir el fitxer \"%s\" per escriptura", str0);
		MessageBox(linia);

		return;
	}

	fprintf(fp, "####### Fitxer de configuració de fonts i signatures: %s\n", str0);
	fprintf(fp, "####### %s\n", CTime::GetCurrentTime().Format( "%d-%m-%Y %H:%M:%S" ));
	fprintf(fp, "#######\n");

	fprintf(fp, "####### Definició de fonts:\n");
	fprintf(fp, "#######\n");

	for (i=0; (i<ListCtrlFonts->GetItemCount()); i++)
	{
		//	F	Titan10iso-P	(s0p10h10V	P	30	50

		ListCtrlFonts->GetItemText (i, 0, str0, 512); // Nom
		ListCtrlFonts->GetItemText (i, 1, str1, 512); // Codis PCL
		ListCtrlFonts->GetItemText (i, 2, str2, 512); // Orientació (P/L)
		ListCtrlFonts->GetItemText (i, 3, str3, 512); // Amplada
		ListCtrlFonts->GetItemText (i, 4, str4, 512); // Interlínia

		if ((strlen(str1)>0)&&(strcmp(str1," ")!=0))
			fprintf(fp, "F\t%s\t%s\t%s\t%s\t%s\n", str0, str1, str2, str3, str4);
	}

	fprintf(fp, "#######\n");
	fprintf(fp, "####### Definició de signatures:\n");
	fprintf(fp, "#######\n");

	for (i=0; (i<ListCtrlSignatures->GetItemCount()); i++)
	{
		//	S	ESCUT-P	A	E:\XES2PCL\Recursos\IMPRESSORA1\escut.prn	20	20

		ListCtrlSignatures->GetItemText (i, 0, str0, 512); // Nom
		ListCtrlSignatures->GetItemText (i, 1, str1, 512); // Lletra
		ListCtrlSignatures->GetItemText (i, 2, str2, 512); // Camí macro
		ListCtrlSignatures->GetItemText (i, 3, str3, 512); // Desplaçament X
		ListCtrlSignatures->GetItemText (i, 4, str4, 512); // Desplaçament Y

		if ((strlen(str1)>0)&&(strcmp(str1," ")!=0))
			fprintf(fp, "S\t%s\t%s\t%s\t%s\t%s\n", str0, str1, str2, str3, str4);
	}

	fprintf(fp, "#######\n");
	fprintf(fp, "####### Definició de safates:\n");
	fprintf(fp, "#######\n");

	EditCtrl=(CEdit*)GetDlgItem(IDC_SAFATA_SUP);
	EditCtrl->SetSel(0,-1,0);
	EditCtrl->GetWindowText(str0, 512);
	fprintf(fp, "T\tSup\t%s\n", str0);

	EditCtrl=(CEdit*)GetDlgItem(IDC_SAFATA_INF);
	EditCtrl->SetSel(0,-1,0);
	EditCtrl->GetWindowText(str1, 512);
	fprintf(fp, "T\tInf\t%s\n", str1);

	EditCtrl=(CEdit*)GetDlgItem(IDC_SAFATA_MAN);
	EditCtrl->SetSel(0,-1,0);
	EditCtrl->GetWindowText(str2, 512);
	fprintf(fp, "T\tMan\t%s\n", str2);

	fprintf(fp, "#######\n");

	fclose(fp);
}

/////////////////////////////////////////////////////////////////////////////
