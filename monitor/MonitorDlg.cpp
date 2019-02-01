// MonitorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Monitor.h"
#include "MonitorDlg.h"

#include <windows.h>
#include <winspool.h>
#include <string.h>
#include <stdlib.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CListCtrl* ListCtrlConfig;
CListCtrl* ListCtrlLog;

/////////////////////////////////////////////////////////////////////////////
// CMonitorDlg dialog

CMonitorDlg::CMonitorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMonitorDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMonitorDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMonitorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMonitorDlg)
	DDX_Control(pDX, IDC_LIST_CONFIG, m_List_Config);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMonitorDlg, CDialog)
	//{{AFX_MSG_MAP(CMonitorDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_CONFIG_FILE, OnButtonConfigFile)
	ON_BN_CLICKED(IDC_BUTTON_SAVE, OnButtonSave)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

int ReadLine(FILE *fp, char *linia)
{
	int c, i;

	// 0D (13) = Carriage Return
	// 0A (10) = Line Feed

	i=0;

	while (1)
	{
	  c=fgetc(fp);
	  
	  if (c<0)   break; // Final de fitxer
	  if (c==10) break; // Line Feed

	  if (c!=13) linia[i++]=c;
	}

	linia[i]=0;

	return c;
}

/////////////////////////////////////////////////////////////////////////////

void CMonitorDlg::LoadConfig(void)
{
	FILE *fp;
	char linia[512], Estat, ImpressoraXES[512], ImpressoraPCL[512], FontsFile[512], EditConfigFile[512], Descripcio[512];
	CEdit* EditCtrl;
	int NumImpressores;

	EditCtrl=(CEdit*)GetDlgItem(IDC_EDIT_CONFIG_FILE);
	EditCtrl->SetSel(0,-1,0);

	EditCtrl->GetWindowText(EditConfigFile, 512);

	fp=fopen(EditConfigFile, "rt");
	if (fp==NULL)
	{
		sprintf(linia, "ERROR: No es pot obrir el fitxer \"%s\" per lectura", EditConfigFile);
		MessageBox(linia);

		return;
	}

	ListCtrlConfig->DeleteAllItems();

	NumImpressores=0;

	while (1)
	{
		if (ReadLine(fp, linia)<0) break;

		if (strlen(linia)==0) continue;
		if (linia[0]=='#') continue;

		Estat = strtok(linia, "\t")[0];

		strcpy(ImpressoraXES, strtok(NULL, "\t"));
		strcpy(Descripcio,    strtok(NULL, "\t"));
		strcpy(ImpressoraPCL, strtok(NULL, "\t"));
		strcpy(FontsFile,	  strtok(NULL, "\t"));

		//ListCtrlConfig->InsertItem (NumImpressores,    " ");
		ListCtrlConfig->InsertItem (LVIF_TEXT|LVIF_IMAGE, NumImpressores, " ", 0, 0, 1, NULL);

		ListView_SetItemState (ListCtrlConfig->m_hWnd, NumImpressores, (Estat - '0' + 1) << 12, LVIS_STATEIMAGEMASK);
		ListCtrlConfig->SetItemText (NumImpressores, 1, ImpressoraXES);
		ListCtrlConfig->SetItemText (NumImpressores, 2, Descripcio);
		ListCtrlConfig->SetItemText (NumImpressores, 3, ImpressoraPCL);
		ListCtrlConfig->SetItemText (NumImpressores, 5, FontsFile);

		NumImpressores++;
	}

	fclose(fp);

	ListCtrlConfig->InsertItem  (NumImpressores,    " ");
	ListCtrlConfig->SetItemText (NumImpressores, 1, " ");

	ListCtrlConfig->SetRedraw(TRUE);
}

/////////////////////////////////////////////////////////////////////////////

UINT MonitorLog( LPVOID pParam )
{
  HANDLE hPipe;
  unsigned char Buffer[512];
  DWORD numread, ret;
  char *ImpressoraPCL, *Job, *ImpressoraXES, *strdate, *strtime1, *strtime2, str[512], *NumBytes;

  hPipe=CreateNamedPipe("\\\\.\\pipe\\xes2pcl", 
					   PIPE_ACCESS_DUPLEX,
					   PIPE_TYPE_MESSAGE,
					   PIPE_UNLIMITED_INSTANCES,
					   0,
					   0,
					   60000,
					   NULL);

  if (hPipe==INVALID_HANDLE_VALUE)
  {
	  ret=GetLastError();
	  exit(1);
  }

  while (1)
  {
	while (!ConnectNamedPipe(hPipe, NULL));

    ret=ReadFile(hPipe, Buffer, sizeof(Buffer), &numread, NULL); Buffer[numread]=0;
	if ((ret==1)&&(numread>0))
	{
		strdate       = strtok((char *)Buffer, "\t");
	    strtime1      = strtok(NULL, "\t");
	    strtime2      = strtok(NULL, "\t");
		ImpressoraXES = strtok(NULL, "\t");
		Job           = strtok(NULL, "\t");
		ImpressoraPCL = strtok(NULL, "\t");
		NumBytes      = strtok(NULL, "\t");

		ListCtrlLog->DeleteItem   (0);
		ListCtrlLog->InsertItem   (MAXLOGITEMS, "");

		strcpy(str, strdate);
		strcat(str, " ");
		strcat(str, strtime1);
		ListCtrlLog->SetItemText  (MAXLOGITEMS, 0, str);

		strcpy(str, strdate);
		strcat(str, " ");
		strcat(str, strtime2);
		ListCtrlLog->SetItemText  (MAXLOGITEMS, 1, str);

		ListCtrlLog->SetItemText  (MAXLOGITEMS, 2, ImpressoraXES);
		ListCtrlLog->SetItemText  (MAXLOGITEMS, 3, Job);
		ListCtrlLog->SetItemText  (MAXLOGITEMS, 4, ImpressoraPCL);
		ListCtrlLog->SetItemText  (MAXLOGITEMS, 5, NumBytes);

		ListCtrlLog->EnsureVisible(MAXLOGITEMS, FALSE);	
	}

	while (!DisconnectNamedPipe(hPipe));

	Sleep(50); // Espera 50 ms
  }

  return 0;
}

/////////////////////////////////////////////////////////////////////////////

void CarregaInicial(void)
{
	char linia[512], *flog, str[512];
	char strdatelog[512], strtimelog[512];
	char strdate[512], strtime1[512], strtime2[512];
	char strImpressoraXES[512], strJob[512], strImpressoraPCL[512], strNumBytes[512];
	FILE *fp;

	flog=getenv("XES2PCL_LOG");
	if (flog==NULL) return;

	fp=fopen(flog, "rt");
	if (fp==NULL) return;

	while (1)
	{
		if (ReadLine(fp, linia)<0) break;

		strcpy(strdatelog,		 strtok(linia, "\t"));
		strcpy(strtimelog,		 strtok(NULL,  "\t"));
		strcpy(strdate,			 strtok(NULL,  "\t"));
		strcpy(strtime1,		 strtok(NULL,  "\t"));
		strcpy(strtime2,		 strtok(NULL,  "\t"));
		strcpy(strImpressoraXES, strtok(NULL,  "\t"));
		strcpy(strJob,           strtok(NULL,  "\t"));
		strcpy(strImpressoraPCL, strtok(NULL,  "\t"));
		strcpy(strNumBytes,      strtok(NULL,  "\t"));

		ListCtrlLog->DeleteItem   (0);
		ListCtrlLog->InsertItem   (MAXLOGITEMS, "");

		strcpy(str, strdate);
		strcat(str, " ");
		strcat(str, strtime1);
		ListCtrlLog->SetItemText  (MAXLOGITEMS, 0, str);

		strcpy(str, strdate);
		strcat(str, " ");
		strcat(str, strtime2);
		ListCtrlLog->SetItemText  (MAXLOGITEMS, 1, str);

		ListCtrlLog->SetItemText  (MAXLOGITEMS, 2, strImpressoraXES);
		ListCtrlLog->SetItemText  (MAXLOGITEMS, 3, strJob);
		ListCtrlLog->SetItemText  (MAXLOGITEMS, 4, strImpressoraPCL);
		ListCtrlLog->SetItemText  (MAXLOGITEMS, 5, strNumBytes);

		ListCtrlLog->EnsureVisible(MAXLOGITEMS, FALSE);	
	}
}

/////////////////////////////////////////////////////////////////////////////
// CMonitorDlg message handlers

BOOL CMonitorDlg::OnInitDialog()
{
	int i;

	CDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	//SetIcon(m_hIcon, FALSE);		// Set small icon

	// Configura els controls

	ListCtrlConfig = (CListCtrl*)GetDlgItem(IDC_LIST_CONFIG);
	ListCtrlLog    = (CListCtrl*)GetDlgItem(IDC_LIST_LOG);

	ListView_SetExtendedListViewStyle(ListCtrlConfig->m_hWnd, LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES);

	ListCtrlConfig->InsertColumn(0, _T("Activa"),             LVCFMT_RIGHT, 42);
	ListCtrlConfig->InsertColumn(1, _T("Impressora XES"),     LVCFMT_LEFT, 100);
	ListCtrlConfig->InsertColumn(2, _T("Descripció"),		  LVCFMT_LEFT, 150);
	ListCtrlConfig->InsertColumn(3, _T("Impressora PCL"),     LVCFMT_LEFT, 150);
	ListCtrlConfig->InsertColumn(4, _T("..."),                LVCFMT_LEFT,  21);
	ListCtrlConfig->InsertColumn(5, _T("Fonts i Signatures"), LVCFMT_LEFT, 150);
	ListCtrlConfig->InsertColumn(6, _T("..."),                LVCFMT_LEFT,  21);
	ListCtrlConfig->InsertColumn(7, _T("Cart."),              LVCFMT_LEFT,  42);

	/*
	CRect rect;

	rect.left=1;
	rect.right=10;
	rect.top=1;
	rect.bottom=10;

	CButton Button;
	Button.Create("", BS_CHECKBOX, rect, GetActiveWindow(), 453);
	*/

	ListView_SetExtendedListViewStyle(ListCtrlLog->m_hWnd, LVS_EX_GRIDLINES);

	ListCtrlLog->InsertColumn(0, _T("Hora Inici"),       LVCFMT_LEFT,  105);
	ListCtrlLog->InsertColumn(1, _T("Hora Final"),       LVCFMT_LEFT,  105);
	ListCtrlLog->InsertColumn(2, _T("Impressora Xerox"), LVCFMT_LEFT,  150);
	ListCtrlLog->InsertColumn(3, _T("Job"),              LVCFMT_RIGHT,  50);
	ListCtrlLog->InsertColumn(4, _T("Impressora PCL"),   LVCFMT_LEFT,  150);
	ListCtrlLog->InsertColumn(5, _T("Bytes"),			 LVCFMT_RIGHT,  70);

	for (i=0; (i<=MAXLOGITEMS); i++)
	{
		ListCtrlLog->InsertItem   (i, "");

		ListCtrlLog->SetItemText  (i, 0, "");
		ListCtrlLog->SetItemText  (i, 1, "");
		ListCtrlLog->SetItemText  (i, 2, "");
		ListCtrlLog->SetItemText  (i, 3, "");
		ListCtrlLog->SetItemText  (i, 4, "");
		ListCtrlLog->SetItemText  (i, 5, "");
	}

	ListCtrlLog->EnsureVisible(i, FALSE);

	char *fconfig=getenv("XES2PCL_CONFIG");
	if (fconfig==NULL)
	{
		MessageBox("ATENCIÓ: No es pot llegir la variable d'entorn XES2PCL_CONFIG");
		OnButtonConfigFile();
	}
	else
	{
		CEdit* EditCtrl=(CEdit*)GetDlgItem(IDC_EDIT_CONFIG_FILE);
		EditCtrl->SetSel(0,-1,0);
		EditCtrl->ReplaceSel(fconfig);

		LoadConfig();
	}

	CarregaInicial();

	// Engega un thread que actualitza el ListControl de log

	AfxBeginThread(MonitorLog, NULL);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

/////////////////////////////////////////////////////////////////////////////
// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMonitorDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

/////////////////////////////////////////////////////////////////////////////
// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMonitorDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

/////////////////////////////////////////////////////////////////////////////

void CMonitorDlg::OnButtonConfigFile() 
{
	CFileDialog *DialogBox;
	int ret;

	DialogBox = new CFileDialog(TRUE, "cfg", "", OFN_FILEMUSTEXIST|OFN_EXPLORER,
								"Xes2PCL Configuration {*.cfg}|*.cfg;|All Files{*.*}|*.*||");	

	ret = DialogBox->DoModal();
	if (ret == IDOK)
	{
		CEdit* EditCtrl;
	
		EditCtrl=(CEdit*)GetDlgItem(IDC_EDIT_CONFIG_FILE);
		EditCtrl->SetSel(0,-1,0);
		EditCtrl->ReplaceSel(DialogBox->GetPathName());

		LoadConfig();
	}

	delete DialogBox;
}

/////////////////////////////////////////////////////////////////////////////

void CMonitorDlg::OnButtonSave() 
{
	// Guardar configuració	

	char linia[512], str0[512], str1[512], str2[512], str3[512], str4[512];
	FILE *fp;
	CEdit* EditCtrl;
	int i;

	EditCtrl=(CEdit*)GetDlgItem(IDC_EDIT_CONFIG_FILE);
	EditCtrl->SetSel(0,-1,0);

	EditCtrl->GetWindowText(str0, 512);

	fp=fopen(str0, "wt");
	if (fp==NULL)
	{
		sprintf(linia, "ERROR: No es pot obrir el fitxer \"%s\" per escriptura", str0);
		MessageBox(linia);

		return;
	}

	fprintf(fp, "####### Fitxer de configuració d'impressores: %s\n", str0);
	fprintf(fp, "####### %s\n", CTime::GetCurrentTime().Format( "%d-%m-%Y %H:%M:%S" ));
	fprintf(fp, "#######\n");

	for (i=0; (i<ListCtrlConfig->GetItemCount()); i++)
	{
		//	1	IMPRESSORA1	\\PTBRCUES1\PTPRN810	E:\XES2PCL\Recursos\IMPRESSORA1\Fonts.cfg

		int ret=ListView_GetCheckState(ListCtrlConfig->m_hWnd, i);

		sprintf(str0, "%i", ret);					   // Activa / Desactiva
		ListCtrlConfig->GetItemText (i, 1, str1, 512); // Impressora XES
		ListCtrlConfig->GetItemText (i, 2, str2, 512); // Impressora PCL
		ListCtrlConfig->GetItemText (i, 3, str3, 512); // Impressora PCL
		ListCtrlConfig->GetItemText (i, 5, str4, 512); // Fitxer de configuració de fonts

		if ((strlen(str1)>0)&&(strcmp(str1," ")!=0)) fprintf(fp, "%s\t%s\t%s\t%s\t%s\n", str0, str1, str2, str3, str4);
	}

	fprintf(fp, "#######\n");

	fclose(fp);
}

/////////////////////////////////////////////////////////////////////////////
