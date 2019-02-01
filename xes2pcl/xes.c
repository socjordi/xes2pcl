//-------------------------------------------------------------------------------------------------------------------------------------------------
//
//	xes.c
//
//	Conversió de format XES (Xerox Escape Sequence) a format PCL (HP Printer Command Language)
//
//	Abril 2001
//
//-------------------------------------------------------------------------------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <time.h>

#include "xes.h"

//-------------------------------------------------------------------------------------------------------------------------------------------------

// VARIABLES D'ENTORN:
// ------------------

// XES2PCL_CONFIG
//		Conté el camí del fitxer de configuració
//		Si no existeix, genera un error i surt del programa

// XES2PCL_ERRORLOG
//		Conté el camí del fitxer de log d'errors
//		Si no existeix, s'intenta fer servir C:\xes2pcl_error.log com fitxer de log d'errors

// XES2PCL_LOG
//		Conté el camí del fitxer de log
//		Si no existeix, s'intenta fer servir C:\xes2pcl.log com fitxer de log

// XES2PCL_PIPE
//		Nom del pipe que es fa servir per monitoritzar les impressions, mitjançant la rutina Monitor
//		Es recomana \\.\pipe\xes2pcl

//-------------------------------------------------------------------------------------------------------------------------------------------------

// MIDA DE LA PÀGINA i RESOLUCIÓ
// -----------------------------

// El programa està dissenyat per a pàgina DIN A4
// Les mides d'una pàgina DIN A4 són: 210 mm x 297 mm = 8.25 polzades x 11.75 polzades => 3525 punts alçada (a 300 punts per polzada)
// La resolució amb què es treballa és 300 punts per polzada

//-------------------------------------------------------------------------------------------------------------------------------------------------

// FORMAT XES en AS/400 i en HOST:
// ------------------------------

// El format XES permet enviar comandes directament (STAT_PRINTABLE), en notació hexadecimal (STAT_COMMAND_HEX) o
// en notació apòstrof (STAT_COMMAND_APOSTROF)

// La seqüència compresa entre dos caràcters escape inicials i un caràcter escape final s'espera en notació hexadecimal
// Dintre d'aquesta seqüència, els caràcters compresos entre un apòstrof inicial i un apòstrof final s'envien com caràcters ASCII
// Per exemple, %%1B 75 'u'% enviaria la seqüència Escape+K+'u'

// Els programes de l'AS/400 fan ús del format STAT_COMMAND_HEX i STAT_COMMAND_APOSTROF
// La variable global TipusDocument indica si el document és d'AS/400 (TIPUS_AS400) o de host (TIPUS_HOST)
// La rutina ComprovarDefinicioEscape comprova si una línia conté els strings "=UDK=" o "&&??" i actualitza la variable global TipusDocument:
// Si conté "=UDK=", fem TipusDocument=TIPUS_HOST
// Si conté "&&??",  fem TipusDocument=TIPUS_AS400

// La rutina de conversió XES a PCL espera que el flux de dades estigui com STAT_PRINTABLE
// La rutina Homogeneitzar converteix una línia STAT_COMMAND_HEX i STAT_COMMAND_APOSTROF a format STAT_PRINTABLE
// Si TipusDocument=TIPUS_AS400, convertim les ocurrències "1B'" a EscapeCharacter, i 
//								 eliminem "'0D0A" i també eliminem les ocurrències EscapeCharacter

//-------------------------------------------------------------------------------------------------------------------------------------------------

// ACTUALITZACIONS:
//
// 18- 4-2002 Hi havia problema en la generació de macros PCL (només enviava les que es feien servir des del darrer reset)
//  7- 5-2002 Per a permetre línies més llargues de 1024 bytes, s'amplien els arrays a 2048
// 10- 5-2002 Si la línia conté més de 132 caràcters imprimibles, es genera un salt de línia
//			  Per a permetre línies més llargues de 2048 bytes, s'amplien els arrays a 4096
//  4- 7-2002 En alguns casos ve una coma entre & i x (línies horitzontals) o entre & i y (línies verticals)
//  4- 7-2002 Si venia un espai darrera d'un & (per exemple, "GMBH & CO"), es descartava la resta de línia, la qual podia tenir el salt de pàgina
//  8- 7-2002 No feia el marge (coordenada 350) per defecte en el cas d'AS400, i no mantenia la negreta en més d'una línia en AS400
// 14- 1-2003 La captura SGI10020 (carta de pagament, arquitectura, AS400) no s'imprimia bé (ocupava més d'1 full).
//            La captura SGIPRHC1 (carta de pagament, arquitectura, host) s'imprimia bé
//            S'ha afegit a la funció Homogeneitzar que també s'ignori el 10 (Line Feed) (només s'ignorava el 13, Carriage Return)
// 17- 1-2003 El canvi del 14-1-2003 donava problemes en alguns documents, s'intenta solucionar
// 21- 1-2003 S'afegeix un salt de pàgina quan arriba un reset (Esc+X) en el cas AS400 (només estava en el cas de host)
// 20- 5-2003 Per a permetre línies més llargues de 16384 bytes, s'amplien els arrays a 16384 (el document SGI1CA96 conté una línia de 6736 caràcters)

//-------------------------------------------------------------------------------------------------------------------------------------------------

// DEFINES
// -------

// Origen del document, host o AS/400

#define TIPUS_HOST  1
#define TIPUS_AS400 2

// Notació de les comandes al flux de dades, directa, en notació hexadecimal o en notació apòstrof

#define STAT_PRINTABLE        1
#define STAT_COMMAND_HEX      2
#define STAT_COMMAND_APOSTROF 3

// Dimensió de l'array Fonts (màxim de nombre de fonts per a una impressora qualsevol)

#define MAX_FONTS 50

// Identificador de log d'impressió (PRINTLOG) o de log d'errors (ERRORLOG)

#define PRINTLOG 1
#define ERRORLOG 2

// Mida vertical de la pàgina A4 (en punts, 1 polzada=300 punts)
// PAGSIZEY_L: Marge esquerra orientació Landscape
// PAGSIZEY:   Utilitzat en canvi d'origen en la conversió de XES a PCL

#define PAGSIZEY_L 3350
#define PAGSIZEY   3450

// Si DELFILE està definit, el fitxer que s'imprimeix s'elimina

#define DELFILE

// Si FILEIN està definit en el moment de compilació, es genera fitxer que conté l'entrada estàndard

//#define FILEIN "D:\\xes2pcl\\in.prn"

// Si LOGIN  està definit, s'escriu cada línia d'entrada  al log
// Si LOGOUT està definit, s'escriu cada línia de sortida al log

//#define LOGIN
//#define LOGOUT

// Si DELLOG està definit, el log es neteja abans de processar

//#define DELLOG

// Si PRINT està definit, el fitxer s'imprimeix

#define PRINT

//-------------------------------------------------------------------------------------------------------------------------------------------------

// VARIABLES GLOBALS
// -----------------

// Nom de la impressora PCL on s'enviarà la sortida, normalment una cua d'impressió Windows. Per exemple, \\PTBRCUES1\PTPRN810

char ImpressoraPCL[256];

// Array de fonts per la impressora Xerox emulada

struct
{
	char nom[256];		// Nom de la font XES. Exemples: Titan10iso-P, ESCUT-P
	char tipus;			// Tipus de font: font ('F') o signatura ('S')
	char codis[256];	// Codis PCL per activar la font (si tipus=='F'), o bé camí del fitxer de macro (si tipus=='S')
	int  num_macro;		// Número de font XES. Correspon al número de macro PCL. Només si tipus=='S'
	int  enviar_macro;	// Indica si cal enviar-ne la definició a la impressora (>0) o no (0)
	char orientacio;	// Orientació de la font: portrait ('P') o landscape ('L'). Només si tipus=='F'
	int  size;			// Mida en cas de font, offset hortizontal en cas de signatura
	int  linespacing;	// Espai entre línies (si tipus=='F'), o bé offset vertical (tipus=='S')
	char lletra;		// Caràcter que activa la signatura (tipus=='S')
}
Fonts[MAX_FONTS];

// NumFonts: Nombre d'entrades a l'array Fonts

int  NumFonts;

// NumFont: Número d'entrada del font actual a l'array Fonts. Inicialment val -1

int  NumFont;

// NumMacros: Nombre total de macros PCL per enviar

int  NumMacros;

// px, py: Coordenades X i Y de la posició actual a la pàgina PCL

int  px, py;

// TipusDocument: Indica l'origen del document (TIPUS_HOST o TIPUS_AS400)

int  TipusDocument;

// EscapeCharacter: Guarda el caràcter d'escape

char EscapeCharacter;

// Notacio: Indica el tipus de notació actual de les comandes XES (STAT_PRINTABLE, STAT_COMMAND_HEX o STAT_COMMAND_APOSTROF)

int  Notacio;

// LastFont: Guarda el número de font del darrer font seleccionat al codi PCL, s'utilitza per no repetir selecció de font si no cal

int  LastFont;

// SafataSuperior: Codi de la safata superior
char SafataSuperior[256];

// SafataInferior: Codi de la safata inferior
char SafataInferior[256];

// SafataManual: Codi de la safata manual
char SafataManual[256];

char *RedMon_Printer;

char *RedMon_Job;

char UnderLine;

char HiHaTextALaPagina;

// delta, deltay: Desplaçaments en el posicionament

int deltax;
int deltay;

int maxlen;

char Bold;

int PageFeed;

//-------------------------------------------------------------------------------------------------------------------------------------------------

// Log: Escriu un missatge al fitxer de log
// El fitxer de log és l'especificat a la variable d'entorn XES2PCL_LOG o XESPCL_ERRORLOG
// Si no es pot llegir aquesta variable, es guarda a C:\xes2pcl.log o c:\xes2pcl_error.log
// Si no es pot obrir aquest fitxer, surt del programa
// El missatge es guarda precedit per la data i l'hora

void Log(int LogType, char *str)
{
	FILE *fplog;
	char *flog, str_date[256], str_time[256];

	_strdate(str_date); // Llegeix el dia actual a str_date
    _strtime(str_time); // Llegeix l'hora actual a str_time

	// Si LogType és PRINTLOG, escriu el missatge al fitxer de camí i nom continguts a la variable XES2PCL_LOG
	// Si no es pot llegir aquesta variable, s'escriu al fitxer c:\xes2pcl.log
	// Si no es pot obrir el fitxer, se surt del programa

	// Si LogType és ERRORLOG, escriu el missatge al fitxer de camí i nom continguts a la variable XES2PCL_LOG
	// Si no es pot llegir aquesta variable, s'escriu al fitxer c:\xes2pcl.log
	// Si no es pot obrir el fitxer, se surt del programa

	// Si LogType no és PRINTLOG ni ERRORLOG, se surt del programa

	if (LogType==PRINTLOG)
	{
		flog=getenv("XES2PCL_LOG");
		if (flog==NULL) fplog=fopen("C:\\xes2pcl.log", "at");
		else fplog=fopen(flog, "at");
	}
	else if (LogType==ERRORLOG)
	{
		flog=getenv("XES2PCL_ERRORLOG");
		if (flog==NULL) fplog=fopen("C:\\xes2pcl_error.log", "at");
		else fplog=fopen(flog, "at");
	}
	else
	{
		fplog=fopen("C:\\xes2pcl.log", "at");
		fprintf(fplog, "%s\t%s\tERROR: LogType=%i\n", str_date, str_time, LogType);
		fclose(fplog);
		exit(1);
	}

	if (fplog==NULL) exit(1);

	// El missatge s'escriu precedit del dia i l'hora actuals

	fprintf(fplog, "%s\t%s\t%s\n", str_date, str_time, str);

	fclose(fplog);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

void LlistarFonts(void)
{
	int i;
	char str[512];

	for (i=0; (i<NumFonts); i++)
	{
		sprintf(str, "%02i\t%s\ttipus=%c\torientacio=%c", i, Fonts[i].nom, Fonts[i].tipus, Fonts[i].orientacio);
		Log(PRINTLOG, str);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

// CalcularOrigen: Inicialitza les variables px i py a les coordenades de l'origen de la pàgina
// Només cal fer-ho si ambdues contenen el valor inicial -1 i NumFont no val -1 (ja s'ha definit alguna font i coneixem l'orientació del font actual)

void CalcularOrigen(void)
{
	//char str[512];

	if ((px<0)&&(py<0))
	{
		if (Fonts[NumFont].orientacio=='L') { px=0; py=PAGSIZEY_L; }
		else								{ px=0; py=300;        } // 300 ???
	}

	//sprintf(str, "CalcularOrigen px=%i py=%i", px, py);
	//Log(PRINTLOG, str);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

// StrReplace: Substitueix al string str totes les ocurrències del substring substr i les substitueix per rep

void StrReplace(char *str, char *substr, char *rep)
{
	char *p, strnou[16384];

	while (1)
	{
		p=strstr(str, substr); if (p==NULL) return;

		strcpy(strnou, str); strnou[p-str]=0;
		strcat(strnou, rep);
		strcat(strnou, &(str[p-str+strlen(substr)]));

		strcpy(str, strnou);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

// LlegirLinia: llegeix una línia del fitxer fp a la variable linia
// El final de línia es detecta amb el byte 10 (line feed)
// S'ignora el caràcter 13 (Carriage Return)
// Retorna el darrer caràcter llegit de la línia (-1 si detecta final de fitxer)

int LlegirLinia(FILE *fp, char *linia)
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

	  linia[i++]=c;
	}

	if ((i>0)&&(c==10)&&(linia[i-1]==13)) i--;

	linia[i]=0;

	if (i>maxlen) maxlen=i;

	return c;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

// GetParam: Extreu un paràmetre numèric dins el string linia
// S'espera que el caràcter indicat per pos sigui numèric
// La variable pos guarda la posició actual a llegir (la retorna actualitzada indicant el primer caràcter no numèric trobat)

int GetParam(char *linia, int *pos, int *param)
{
	int i1, i2, ret;
	char str[16384];

	i1=*pos;

	while (linia[i1]==' ') i1++;

	for (i2=i1; (i2<(int)strlen(linia)); i2++)
	  if ((linia[i2]<'0')||(linia[i2]>'9')) break;

	strcpy(str, &(linia[i1]));
	str[i2-i1]=0;

	ret=0;

	if (strlen(str)==0) { *param=0; ret=1; }
	else sscanf(str,"%d",param);

	if (TipusDocument==TIPUS_AS400)
	{
		if (linia[i2]==',') i2++;
	}
	if (TipusDocument==TIPUS_HOST)
	{
		if (linia[i2]!=0) i2++;
	}

	*pos=i2;

	return ret;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

// BuscarFontPerNom: Buscar font per nom (NomFont) a l'array Fonts
// Retorna el número de font a l'array
// Retorna -1 si no troba el font

int BuscarFontPerNom(char *NomFont)
{
	int i;

	for (i=0; (i<NumFonts); i++)
		if (strcmp(NomFont, Fonts[i].nom)==0) return i;

	return -1;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

// BuscarFontPerNum: Buscar font per número
// Retorna el número de font a l'array havent-li passat el número de font XES

int BuscarFontPerNum(int NumFont)
{
	int i;

	for (i=0; (i<NumFonts); i++)
		if (Fonts[i].num_macro==NumFont) return i;

	return -1;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

void EscriuGrafic(char *liniaout)
{
	char str[16384];
	int x, y;

	sprintf(str,"\033*r0F");	// Raster Graphics Presentation: Follows Orientation
	strcat(liniaout,str);

	// Inicialitza les variables px i py segons l'orientació del font actual, Fonts[NumFont]
	CalcularOrigen();

	x=px+deltax; if (x<0) x=0;
	y=py+deltay; if (y<0) y=0;

	sprintf(str,"\033*p%05ix%05iY", x+Fonts[NumFont].size, y+Fonts[NumFont].linespacing);  // Move Positioning By Dot
	strcat(liniaout,str);

	sprintf(str,"\033*r1A");	// Start Graphics At Current Position
	strcat(liniaout,str);

	//sprintf(str,"\033&f%iY", Fonts[NumFont].num_macro);			  // Macro ID #
	sprintf(str,"\033&f%iY", Fonts[NumFont].enviar_macro);			  // Macro ID #
	strcat(liniaout,str);

	sprintf(str,"\033&f2X");	// Execute Macro
	strcat(liniaout,str);

	sprintf(str,"\033*r1B");	// End Raster Graphics
	strcat(liniaout,str);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

// ConvertirLinia: Converteix el string liniain (en format XES) en liniaout (en format PCL)
//				liniain:  línia d'entrada
//				liniaout: línia de sortida

void ConvertirLinia(char *liniain, char *liniaout)
{
	int pos, pos0, x, y, l, t, s, SeleccionaFont, nFont, Posicionat, NumCaractersALaLinia, ret;
	char c, NomFont[16384], str[16384], HiHaTextALaLinia;


	CalcularOrigen();

	// pos: Posició del caràcter actual dins la línia actual

	pos=0;			

	// SeleccionaFont: Indica si s'ha seleccionat una font (0: no s'ha seleccionat, 1: s'ha seleccionat)
	//				   Si es selecciona una font, hi ha salt de línia

	SeleccionaFont=0;

	HiHaTextALaLinia=0;
	Posicionat=0;

	strcpy(liniaout,"");

	NumCaractersALaLinia=0;

	while(1)
	{
		if (liniain[pos]==0) break; // Final de línia

		if (liniain[pos]==13) // Carriage Return (sense Line Feed)
		{
			if ((Fonts[NumFont].tipus=='S')||(Fonts[NumFont].orientacio=='P'))
			{
				if (TipusDocument==TIPUS_AS400) px=350;
				else						    px=0;
			}
			else
			{
				py=PAGSIZEY_L;
			}

			// Si l'orientació és 'Landscape', enviem comanda d'orientació 0 graus
			if (Fonts[NumFont].orientacio=='L') strcat(liniaout,"\033&a0P");

			x=px+deltax; if (x<0) x=0;
			y=py+deltay; if (y<0) y=0;

			sprintf(str,"\033*p%05ix%05iY", x, y); // Move Positioning By Dot
			strcat(liniaout, str);

			HiHaTextALaLinia=0;

			pos++;

			continue;
		}

		if (liniain[pos]==12) // Page feed
		{
			if (HiHaTextALaPagina) strcat(liniaout, "\014");

			if (Fonts[NumFont].orientacio=='P')
				py=0;
			else
				px=0;

			HiHaTextALaPagina=0;
			
			pos++; 
			
			continue;
		}

		if (liniain[pos]==EscapeCharacter)
		{
			pos++;
			c=liniain[pos];
			
			pos++;

			switch(c)
			{
			case 'b':	// Begin Boldface (comença negreta)

						sprintf(str,"\033(s3B"); strcat(liniaout, str); Bold=1;

						break;

			case 'p':	// End Boldface (acaba negreta)

						sprintf(str,"\033(s0B"); strcat(liniaout, str); Bold=0;

						break;

			case 'u':	// Select Single Floating Underline (comença subratllat)

						//sprintf(str,"\033&d3D"); strcat(liniaout, str);
						UnderLine=1;

						break; 

			case 'w':	// Turn Off All Underline (acaba subratllat)

						sprintf(str,"\033&d@");  strcat(liniaout, str);
						UnderLine=0;

						break;

			case 'a':	// Text Placement Absolute (posicionament)

						GetParam(liniain, &pos, &x);
						GetParam(liniain, &pos, &y);

						px=x;
						py=PAGSIZEY-y;

						x=px+deltax; if (x<0) x=0;
						y=py+deltay; if (y<0) y=0;

						sprintf(str,"\033*p%05ix%05iY", x, y); // Move Positioning By Dot
						strcat(liniaout, str);

						if (liniain[pos]==0) Posicionat=1;

						break;

			case 'x':	// Line Draw X (línia horitzontal)

						s=0;

						ret=GetParam(liniain, &pos, &x);		// X Coord 
						if (ret) GetParam(liniain, &pos, &x);	// X Coord 
						GetParam(liniain, &pos, &y);			// Y Coord
						GetParam(liniain, &pos, &l);			// Length
						GetParam(liniain, &pos, &t);			// Width (>=2)
						GetParam(liniain, &pos, &s);			// Shading (0-15)

						//sprintf(str, "x=%i\ty=%i\tl=%i\tt=%i\ts=%i",x,y,l,t,s);
						//Log(PRINTLOG,str);

						y=PAGSIZEY-y-t;

						x=x+deltax;	if (x<0) x=0;
						y=y+deltay; if (y<0) y=0;

						sprintf(str,"\033&a0P");			   // Print Direction:  0 degrees
						strcat(liniaout, str);

						sprintf(str,"\033*p%05ix%05iY", x, y); // Move Positioning By Dot
						strcat(liniaout, str);

						if (s==0)
							sprintf(str,"\033*c%05ia%05ib%01iP", l, t, s);       // Rectangle width l, height t (dots)
						else
							sprintf(str,"\033*c%05ig%05ia%01ib2P", s*10, l, t);

						strcat(liniaout, str);

						break;

			case 'y':	// Line Draw Y (línia vertical)

						s=0;

						ret=GetParam(liniain, &pos, &x);		// X Coord 
						if (ret) GetParam(liniain, &pos, &x);	// X Coord 
						GetParam(liniain, &pos, &y);			// Y Coord
						GetParam(liniain, &pos, &l);			// Length
						GetParam(liniain, &pos, &t);			// Width (>=2)
						GetParam(liniain, &pos, &s);			// Shading (0-15)

						//sprintf(str, "x=%i\ty=%i\tl=%i\tt=%i\ts=%i",x,y,l,t,s);
						//Log(PRINTLOG,str);

						y=PAGSIZEY-y-l;

						x=x+deltax;	if (x<0) x=0;
						y=y+deltay; if (y<0) y=0;

						sprintf(str,"\033&a0P");		// Print Direction:  0 degrees
						strcat(liniaout, str);

						sprintf(str,"\033*p%05ix%05iY",x,y); // Move Positioning By Dot
						strcat(liniaout, str);

						sprintf(str,"\033*c%05ib%05ia%01iP",l,t,s); // Rectangle width l, height t (dots)
						strcat(liniaout, str);

						break;

			case '+':	// Definició de fonts

						// S'ignoren: Esc+A Esc+B Esc+C Esc+E Esc+F Esc+H Esc+J Esc+K Esc+M Esc+n Esc+N Esc+P
						//			  Esc+Q Esc+R Esc+T Esc+U Esc+V

						if ((liniain[pos]=='A')||
						    (liniain[pos]=='B')||
						    (liniain[pos]=='C')||
						    (liniain[pos]=='E')||
						    (liniain[pos]=='F')||
						    (liniain[pos]=='H')||
						    (liniain[pos]=='J')||
						    (liniain[pos]=='K')||
						    (liniain[pos]=='M')||
						    (liniain[pos]=='n')||
						    (liniain[pos]=='N')||
						    (liniain[pos]=='P')||
						    (liniain[pos]=='Q')||
						    (liniain[pos]=='R')||
						    (liniain[pos]=='T')||
						    (liniain[pos]=='U')||
						    (liniain[pos]=='V'))
						{ liniain[pos]=0; break; }

						// S'ignoren: Esc+D Esc+G (Esc+X també s'ignorava fins el 21-1-2003)

						if ((liniain[pos]=='D')||(liniain[pos]=='G')) { pos++; break; }

						if (liniain[pos]=='X')
						{
							if (TipusDocument==TIPUS_AS400)
							{
								PageFeed=1;
								ResetFonts();
							}

							pos++; break; 
						}

						// S'ignora: Esc+I

						if ((liniain[pos]=='I')) { pos+=2; break; }

						// Definició de fonts: Esc+0 Esc+1 Esc+2 Esc+3 Esc+4 Esc+5 Esc+6 Esc+7 Esc+8 Esc+9

						if ((liniain[pos]>='0')&&(liniain[pos]<='9'))
						{
							// Extreu el nom del font: NomFont, llegim fins caràcter escape, espai o final de línia

							pos0=pos;
							while ((liniain[pos]!=EscapeCharacter)&&
								   (liniain[pos]!=' ')&&
								   (liniain[pos]!='\001')&&
								   (liniain[pos]!=13)&&
								   (liniain[pos]!=10)&&
								   (liniain[pos]!=0)) pos++;

							nFont=liniain[pos0++]-'0';
							strcpy(NomFont,&(liniain[pos0]));
							NomFont[pos-pos0]=0;

							//sprintf(str, "nomFont='%s' %i %i", NomFont, NomFont[strlen(NomFont)-2], NomFont[strlen(NomFont)-1]);
							//Log(PRINTLOG, str);
							
							// Buscar font per nom (NomFont) a l'array Fonts
							// Retorna el número de font a l'array
							// Retorna -1 si no troba el font
							NumFont=BuscarFontPerNom(NomFont);
							if (NumFont<0)
							{
								sprintf(str, "ERROR: No es pot trobar el font \"%s\" (%i) per la impressora %s (liniain=\"%s\")", NomFont, nFont, getenv("REDMON_PRINTER"), liniain);
								Log(ERRORLOG, str);
							}
							else
							{
								Fonts[NumFont].num_macro=nFont;
								if ((Fonts[NumFont].tipus=='S')&&(Fonts[NumFont].enviar_macro<=0))
										Fonts[NumFont].enviar_macro=++NumMacros;
							}


							if (Fonts[NumFont].orientacio=='P')
							{
								deltax=-90;
								deltay=0;
							}
							else
							{
								deltax=10;
								deltay=0;
							}						
						}

						// Inicialitza les variables px i py segons l'orientació del font actual, Fonts[NumFont]
						px=-1; py=-1;
						CalcularOrigen();

						break;

			case 'c':	// Paper Tray (font de paper, número de safata)

						// XES: &c1 (safata superior)
						// XES: &c2 (safata inferior)
						// XES: &c5 (manual)
						
						// fprintf(fpout,"\033&l1H"); // OKIPAGE 20 Plus - Safata central
						// fprintf(fpout,"\033&l2H"); // OKIPAGE 20 Plus - Safata superior (manual, espera FORM FEED)
						// fprintf(fpout,"\033&l3H"); // OKIPAGE 20 Plus - Safata superior (manual, espera FORM FEED)
						// fprintf(fpout,"\033&l4H"); // OKIPAGE 20 Plus - Safata superior (manual, no espera FORM FEED)
						// fprintf(fpout,"\033&l5H"); // OKIPAGE 20 Plus - Safata inferior
						// fprintf(fpout,"\033&l6H"); // OKIPAGE 20 Plus - Safata central

						// fprintf(fpout,"\033&l1H"); // HP LaserJet 2200 DTN - Safata central (safata 2)
						// fprintf(fpout,"\033&l2H"); // HP LaserJet 2200 DTN - Alimentació manual, paper
						// fprintf(fpout,"\033&l3H"); // HP LaserJet 2200 DTN - Alimentació manual, sobres
						// fprintf(fpout,"\033&l4H"); // HP LaserJet 2200 DTN - Safata 1
						// fprintf(fpout,"\033&l5H"); // HP LaserJet 2200 DTN - Safata inferior (safata 3)
						// fprintf(fpout,"\033&l7H"); // HP LaserJet 2200 DTN - Selecció automàtica

						if      ((liniain[pos]=='1'))
							sprintf(str,"\033&l%sH", SafataSuperior);
						else if ((liniain[pos]=='2'))
							sprintf(str,"\033&l%sH", SafataInferior);
						else if ((liniain[pos]=='5'))
							sprintf(str,"\033&l%sH", SafataManual);

						strcat(liniaout, str);

						pos++;

						break;

			case 'd':	pos++; break;			// S'ignora: Escd

			case 'e':	pos++; break;			// S'ignora: Esce

			case 'f':	liniain[pos]=0; break;	// S'ignora: Escf

			case 'g':	liniain[pos]=0; break;	// S'ignora: Escg

			case 'h':	pos++; break;			// S'ignora: Esch

			case 'i':	if (liniain[pos]=='p') liniain[pos]=0;	// S'ignora: Esci
						else pos+=2;

						break;

			case 'j':	pos++; break;			// S'ignora: Escj

			case 'k':	pos++; break;			// S'ignora: Esck

			case 'l':	pos++; break;			// S'ignora: Escl

			case 'm':	liniain[pos]=0; break;	// S'ignora: Escm

			case 'n':	pos++; break;			// S'ignora: Escn

			case 'o':	pos++; break;			// S'ignora: Esco

			case 'q':	pos++; break;			// S'ignora: Escq

			case 's':	pos++; break;			// S'ignora: Escs

			case 't':	liniain[pos]=0; break;	// S'ignora: Esct

			case 'v':	liniain[pos]=0; break;	// S'ignora: Escv

			case 'z':	liniain[pos]=0; break;	// S'ignora: Escz

			default:	// Selecció font o comanda XES no esperada

						if ((c>='0')&&(c<='9')) // Text
						{
							nFont=c-'0';

							// Buscar font per número
							// Retorna el número de font a l'array havent-li passat el número de font XES
							NumFont=BuscarFontPerNum(nFont);

							if (NumFont<0)
							{
								sprintf(str, "ERROR: No es pot trobar el font número %i per la impressora %s", nFont, getenv("REDMON_PRINTER"));
								Log(ERRORLOG, str);
							}
							else
							{
								if (Fonts[NumFont].tipus=='F')
								{
									// Selecció de font

									// Enviar codis de selecció de font només si ha canviat

									if (LastFont!=NumFont)
									{
										//if (Fonts[NumFont].orientacio=='L') strcat(liniaout,"\033&a90P");
										//else								strcat(liniaout,"\033&a0P");

										sprintf(str,"\033%s",Fonts[NumFont].codis);
										strcat(liniaout, str);

										LastFont=NumFont;
									}

									SeleccionaFont=1;
								}
							}
						}
						else
						{
							if (c==' ')
							{
								sprintf(str, "& ");
								strcat(liniaout, str);

								NumCaractersALaLinia++;
							}
							else
							{
								// Qualsevol altre cas (comanda XES ignorada)
								// S'ignora fins el proper caràcter d'escape o final de línia
								while ((liniain[pos]!=EscapeCharacter)&&(liniain[pos]!=0)&&(liniain[pos]!=1)) pos++;
							}
						}
			}
		}
		else // Text (El caràcter actual no és EscapeCharacter)
		{
			// Si el tipus del font actual és signatura ('S'), i arriba la lletra que l'activa, imprimir-la
			// Si el tipus del font actual és signatura ('S'), i arriba una lletra que no l'activa, ignorar-la
			// Si el tipus del font actual és font      ('F'), escriure el caràcter

			if (Fonts[NumFont].tipus=='S')
			{
				HiHaTextALaPagina=1;

				if (Fonts[NumFont].lletra==liniain[pos])
					EscriuGrafic(liniaout);

				// Saltar-se el text (fins el proper caràcter d'escape o final de línia)
				while ((liniain[pos]!=EscapeCharacter)&&(liniain[pos]!=0)&&(liniain[pos]!=1)) pos++;
			}
			else
			{
				if ((HiHaTextALaLinia==0)&&(liniain[pos]==' '))
				{
					if (Fonts[NumFont].orientacio=='P')
						px+=Fonts[NumFont].size;
					else
						py-=Fonts[NumFont].size;
				}
				else
				{
					if (HiHaTextALaLinia==0)
					{
						x=px+deltax; if (x<0) x=0;
						y=py+deltay; if (y<0) y=0;

						// Posicionament a les coordenades x i y
						sprintf(str,"\033*p%05ix%05iY", x, y); // Move Positioning By Dot
						strcat(liniaout, str);

						if ((TipusDocument==TIPUS_AS400)&&(Bold==1)) strcat(liniaout,"\033(s3B");

						//sprintf(str, "NumFont=%i orientacio=%c", NumFont, Fonts[NumFont].orientacio);
						//Log(PRINTLOG, str);
						//LlistarFonts();

						// Si l'orientació és 'Landscape', enviem comanda d'orientació 90 graus
						if (Fonts[NumFont].orientacio=='L') strcat(liniaout,"\033&a90P");
						//else strcat(liniaout,"\033&a0P");

						if (UnderLine==1)
						{
							strcat(liniaout,"\033&d3D");
							UnderLine=0;
						}						
					}

					sprintf(str, "%c", liniain[pos]);
					strcat(liniaout, str);

					NumCaractersALaLinia++;
					if (NumCaractersALaLinia>=132)
					{
						// Salt de línia

						if ((Fonts[NumFont].tipus=='S')||(Fonts[NumFont].orientacio=='P'))
						{
							px=0;
							py+=Fonts[NumFont].linespacing;
						}
						else
						{
							px+=Fonts[NumFont].linespacing;
							py=PAGSIZEY_L;
						}

						NumCaractersALaLinia=0;

						// Si l'orientació és 'Landscape', enviem comanda d'orientació 0 graus
						if (Fonts[NumFont].orientacio=='L') strcat(liniaout,"\033&a0P");

						x=px+deltax; if (x<0) x=0;
						y=py+deltay; if (y<0) y=0;

						sprintf(str,"\033*p%05ix%05iY", x, y); // Move Positioning By Dot
						strcat(liniaout, str);

						if (Fonts[NumFont].orientacio=='L') strcat(liniaout,"\033&a90P");

						HiHaTextALaLinia=0;
					}

					HiHaTextALaLinia=1;
					HiHaTextALaPagina=1;
				}

				pos++;
			}
		}
	}

	// Inicialitza les variables px i py segons l'orientació del font actual, Fonts[NumFont]
	CalcularOrigen();

	// Generem nova línia en format PCL

	// Si l'orientació és 'Landscape', enviem comanda d'orientació 0 graus
	if (Fonts[NumFont].orientacio=='L') strcat(liniaout,"\033&a0P");

	// No saltar de línia si la línia acaba amb un posicionament absolut (Esc a)

	if (Posicionat==0)
	{
		// En saltar de línia, poso la coordenada x a 0 si és host o a 350 si és AS400 (marge esquerre per defecte)

		if ((Fonts[NumFont].tipus=='S')||(Fonts[NumFont].orientacio=='P'))
		{
			if (TipusDocument==TIPUS_AS400) px=350;
			else						    px=0;
			
			py+=Fonts[NumFont].linespacing;
		}
		else
		{
			px+=Fonts[NumFont].linespacing;
			py=PAGSIZEY_L;
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

// GenerarMacros: Enviar al fitxer FilePCL el contingut del fitxers de macros

void GenerarMacros(char *name)
{
	char buffer[512];
	int numFont, numMacro, numBytes;
	FILE *fpout, *fpmacro;

	// Obre el fitxer per escriptura

	fpout=fopen(name,"wb");

	if (fpout==NULL)
	{
		sprintf(buffer, "ERROR: No es pot obrir el fitxer %s de sortida", name);
		Log(ERRORLOG, buffer);
		exit(1); 
	}

	numMacro=0;

	// Només tractem les signatures (camp tipus == 'S') que es necessitin en aquesta impressió (camp enviar_macro>0)

	for (numFont=0; (numFont<NumFonts); numFont++)
	{
	if ((Fonts[numFont].tipus=='S')&&(Fonts[numFont].enviar_macro>0))
	{
		fprintf(fpout,"\033E");									// Reset
		//fprintf(fpout,"\033&f%iY", Fonts[numFont].num_macro);	// Macro ID
		fprintf(fpout,"\033&f%iY", Fonts[numFont].enviar_macro);	// Macro ID
		fprintf(fpout,"\033&f0X");								// Start macro definition (last ID specified)

		// Obrim la macro per lectura

		fpmacro=fopen(Fonts[numFont].codis, "rb");

		if (fpmacro==NULL)
		{
			sprintf(buffer, "ERROR: no es pot obrir el fitxer de macro PCL \"%s\"", Fonts[numFont].codis);
			Log(ERRORLOG, buffer);
			exit(1);
		}

		// Copiem el contingut de la macro al fitxer de sortida
		do
		{
			numBytes=fread(buffer, 1, 512, fpmacro);
			fwrite(buffer, 1, numBytes, fpout);
		}
		while (numBytes==512);

		fclose(fpmacro);

		fprintf(fpout,"\033&f1X");								// Stop macro definition (last ID specified)
		fprintf(fpout,"\033&f%iY", Fonts[numFont].enviar_macro);	// Macro ID
		fprintf(fpout,"\033&f10X");								// Make macro permanent (last ID specified)
		fprintf(fpout,"\033E");									// Reset

		numMacro++;
	}
	}

	fclose(fpout);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

// ComprovarDefinicioEscape: Comprovem si linia conté "=UDK=" (Host) o bé "&&??" (AS/400)
// Si conté "=UDK=", fem TipusDocument=TIPUS_HOST
// Si conté "&&??",  fem TipusDocument=TIPUS_AS400
// Guardem el caràcter d'escape a EscapeCharacter

void ComprovarDefinicioEscape(char *linia)
{
	char *p_host, *p_as400, substr[16384];

	PageFeed=0;

	p_host  = strstr(linia, "=UDK=");
	p_as400 = strstr(linia, "&&??");

	if (p_host!=NULL)
	{
		TipusDocument=TIPUS_HOST;
		EscapeCharacter=linia[p_host-linia+5];

		strcpy(substr, &(linia[p_host-linia+6])); substr[4]=0;
		if (strcmp(substr, " &+X")==0)
		{
			PageFeed=1;
			ResetFonts();
		}

		strcpy(substr, &(linia[p_host-linia])); substr[6]=0;
		StrReplace(linia, substr,  "");
	}

	if (p_as400!=NULL)
	{
		TipusDocument=TIPUS_AS400;
		EscapeCharacter=linia[p_as400-linia+4];
		strcpy(substr, &(linia[p_as400-linia])); substr[5]=0;
		StrReplace(linia, substr,  "");

 		//sprintf(substr, "TipusDocument=%i\tEscape='%c'", TipusDocument, EscapeCharacter);
		//Log(PRINTLOG, substr);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

// Homogeneitzar: Homogeneïtzem la linia a un únic format (el de host)
// Només s'aplica si TipusDocument=TIPUS_AS400
// Convertim les ocurrències "1B'" a EscapeCharacter, i eliminem "'0D0A" i també eliminem les ocurrències EscapeCharacter
// Si es troba 0A o 0a en notació hexadecimal, el converteix a un byte 01

void Homogeneitzar(char *linia)
{
	int i, n, lnova;
	char linianova[16384], c, str[16384];

	// En aquesta subrutina només tractem documents tipus AS/400
	if (TipusDocument==TIPUS_HOST) return;

	lnova=0;

	for (i=0; (i<(int)strlen(linia)); i++)
	{
		c=linia[i];

		if (c==EscapeCharacter)
		{
			if      (Notacio==STAT_COMMAND_HEX)      Notacio=STAT_PRINTABLE;
			else if (Notacio==STAT_COMMAND_APOSTROF) linianova[lnova++]=c;
			else if (Notacio==STAT_PRINTABLE)
			{
				if (linia[i+1]==EscapeCharacter) { i++; Notacio=STAT_COMMAND_HEX; }
				else linianova[lnova++]=c;
			}
		}
		else if (c=='\'')
		{
			if      (Notacio==STAT_COMMAND_HEX)      Notacio=STAT_COMMAND_APOSTROF;
			else if (Notacio==STAT_COMMAND_APOSTROF) Notacio=STAT_COMMAND_HEX;
			else if (Notacio==STAT_PRINTABLE)		 linianova[lnova++]=c;
		}
		else // Qualsevol altre caràcter que no sigui Escape ni apòstrof
		{
			if      (Notacio==STAT_COMMAND_HEX)
			{
				if (((c>='0')&&(c<='9'))||
					((c>='A')&&(c<='Z'))||
					((c>='a')&&(c<='z')))
				{
					str[0]=c; str[1]=linia[++i];
					sscanf(str, "%x", &n);

					if      (n==27) linianova[lnova++]=EscapeCharacter;
					else
					{
						if (TipusDocument==TIPUS_AS400)
						{
							if ((n!=13)&&(n!=10))	linianova[lnova++]=n;
						}
						if (TipusDocument==TIPUS_HOST)
						{
							if (n!=13)				linianova[lnova++]=n;
						}
					}
				}
			}
			else linianova[lnova++]=c;
		}
	}

	linianova[lnova]=0;

	strcpy(linia, linianova);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

// Convertir: Converteix el fitxer de nom FileXES (en format XES) al fitxer de nom FilePCL (en format PCL)
// El fitxer FileXES pot ser "stdin", mentre que el fitxer FilePCL pot ser "stdout"

void Convertir(char *FileXES, char *FilePCL)
{
	FILE *fpin, *fpout;
	#if defined(FILEIN)
	FILE *fp;
	#endif
	#if defined(LOGIN)
//char str[16384];
	#endif
	char liniain[16384], liniaout[16384];
	int ret;

	px=-1; py=-1;

	UnderLine=0;

	HiHaTextALaPagina=0;

	if (strcmp(FileXES,"stdin")==0)  fpin=stdin;
	else fpin =fopen(FileXES, "rb"); 
	
	if (fpin==NULL)
	{
		sprintf(liniaout, "ERROR: No es pot obrir el fitxer %s d'entrada", FileXES);
		Log(ERRORLOG, liniaout);
		exit(1); 
	}

	if (strcmp(FileXES,"stdout")==0) fpin=stdout;
	else fpout=fopen(FilePCL,"wb");

	if (fpout==NULL)
	{
		sprintf(liniaout, "ERROR: No es pot obrir el fitxer %s de sortida", FilePCL);
		Log(ERRORLOG, liniaout);
		exit(1); 
	}

	#if defined(FILEIN)
	fp=fopen(FILEIN,"wb");
	#endif

	//fprintf(fpout,"\033\001@EJL SET LO=-5\012"); // Sets Left Offset to -5 mm (entre -99.0 mm i +99.0 mm per la Epson EPL-N1600)

	fprintf(fpout,"\033E");         // Reset

	fprintf(fpout,"\033&l26A");     // Page size: A4
	fprintf(fpout,"\033&l6D");      // Line Spacing: 6 lines/inch

	fprintf(fpout,"\033&l0O");		// Orientation: Portrait
	fprintf(fpout,"\033&l1E");		// Top Margin: 1 line
	fprintf(fpout,"\033&l71F");		// Text Length: 71 lines

	fprintf(fpout,"\033(0N");       // Symbol Set: ISO 8859-1

	fprintf(fpout,"\033(s4148T");   // Primary Typeface Family: Univers

	fprintf(fpout,"\033&k12H");		// Horizontal column spacing: 12*0.0083=0.0996 inches=2.53 mm
	fprintf(fpout,"\033&a1L");		// Left Margin:  column  1
	fprintf(fpout,"\033&a80M");		// Right Margin: column 80

	fprintf(fpout,"\033(s0S");      // End Italics
	fprintf(fpout,"\033(s0B");      // End Boldface

	fprintf(fpout,"\033*t300R");	// Set Graphic Resolution to 300 dpi

	fprintf(fpout,"\033%s", Fonts[0].codis); // Font per defecte

	do
	{
		ret=LlegirLinia(fpin, liniain);

		#if defined(FILEIN)
		//fprintf(fp, "%s\015\012", liniain);
		#endif

		#if defined(LOGIN)
//sprintf(str, "IN =\"%s\"",liniain);
//Log(PRINTLOG, str);
		#endif

		// Comprovem si liniain conté "=UDK=" (Host) o bé "&&??" (AS/400)
		// Si conté "=UDK=", fem TipusDocument=TIPUS_HOST
		// Si conté "&&??",  fem TipusDocument=TIPUS_AS400
		// Guardem el caràcter d'escape a EscapeCharacter

		//sprintf(liniaout, "1 liniain=%s", liniain);
		//Log(PRINTLOG, liniaout);

		ComprovarDefinicioEscape(liniain);

		//sprintf(liniaout, "2 liniain=%s", liniain);
		//Log(PRINTLOG, liniaout);

		// Homogeneïtzem la linia a un únic format
		// Si TipusDocument=TIPUS_AS400, convertim les ocurrències "1B'" a EscapeCharacter, i 
		//								 eliminem "'0D0A" i les ocurrències EscapeCharacter

		Homogeneitzar(liniain);

		//sprintf(liniaout, "Homogeneitzat liniain=\"%s\"", liniain);
		//Log(PRINTLOG, liniaout);

		ConvertirLinia(liniain, liniaout);
		
		#if defined(LOGOUT)
//sprintf(str, "OUT=\"%s\"\n",liniaout);
//Log(PRINTLOG, str);
		#endif

		fputs(liniaout, fpout);

		if (py>PAGSIZEY) fprintf(fpout,"\033*p00100x00100Y");	// Move Positioning By Dot

		if (PageFeed)
		{
			if (HiHaTextALaPagina) fprintf(fpout,"\014");

			px=-1; py=-1;
			CalcularOrigen();

			HiHaTextALaPagina=0;
		}
	}
	while (ret>=0);

	fprintf(fpout,"\033*s0F");      // Flush All Complete Pages
	fprintf(fpout,"\033*s1F");      // Flush All Page Data

	fprintf(fpout,"\033E");         // Reset

	if (strcmp(FileXES,"stdin")!=0)  fclose(fpin); // Peta

	if (strcmp(FileXES,"stdout")!=0) fclose(fpout);

	#if defined(FILEIN)
	fclose(fp);
	#endif
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

// PrintFile: Envia un fitxer de nom File a la impressora indicada pel paràmetre PrinterName

long PrintFile(char *File, char *PrinterName)
{
	#if defined(PRINT)
	HANDLE hPrinter;
	DWORD jobid, written;
	#endif
	DWORD dwNumBytes;
	char buffer[16384];
	int c;
	FILE *fp;
	long numbytes;

	fp=fopen(File, "rb");
	if (fp==NULL)
	{
		sprintf(buffer, "ERROR: No es pot obrir el fitxer %s per imprimir", File);
		Log(ERRORLOG, buffer);
		exit(1);
	}

	numbytes=0;

	#if defined(PRINT)
	if (OpenPrinter(PrinterName, &hPrinter, NULL))
	{
		DOC_INFO_1 doc_info = {0};

		doc_info.pDocName="Document XES a PCL";
		doc_info.pOutputFile=NULL;
		doc_info.pDatatype="RAW";

		jobid=StartDocPrinter(hPrinter, 1, (LPBYTE)&doc_info);

		if (jobid!=0)
		{
			StartPagePrinter(hPrinter);
	#endif

			do
			{
				for (dwNumBytes=0; (dwNumBytes<sizeof(buffer))&&((c=fgetc(fp))!=EOF); dwNumBytes++)
					buffer[dwNumBytes]=(char)c;

				#if defined(PRINT)
				WritePrinter(hPrinter, (void*)buffer, dwNumBytes, &written);
				numbytes+=written;
				#else
				numbytes+=dwNumBytes;
				#endif
			}
			while (c!=EOF);

	#if defined(PRINT)
			EndPagePrinter(hPrinter);
		}

		EndDocPrinter(hPrinter);
		ClosePrinter(hPrinter);
	}
	#endif

	fclose(fp);

	return numbytes;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

// ConstruirNom: Genera un nom de fitxer únic que s'utilitzarà per a generar fitxers temporals
// Per tal de garantir-ne la unicitat, el construïm amb el nom de la impressora seguit del número de job,
// tot precedit pel camí indicat per la variable d'entorn TEMP

void ConstruirNom(char *nom)
{
	char str_zeros[16384], str[16384];
	int i, j;

	RedMon_Printer=getenv("REDMON_PRINTER");
	if (RedMon_Printer==NULL) { Log(ERRORLOG, "ERROR: Variable d'entorn REDMON_PRINTER no trobada"); exit(1); }

	RedMon_Job=getenv("REDMON_JOB");
	if (RedMon_Job==NULL) { Log(ERRORLOG, "ERROR: Variable d'entorn REDMON_JOB no trobada"); exit(1); }

	strcpy(str_zeros, "000000");
	if (strlen(RedMon_Job)<=6) str_zeros[6-strlen(RedMon_Job)]=0;
	else str_zeros[0]=0;

	strcpy(str, RedMon_Printer);
	strcat(str, "_");
	strcat(str, str_zeros);
	strcat(str, RedMon_Job);

	for (i=0, j=0; (i<(int)strlen(str)); i++)
		if ((str[i]=='_')||(IsCharAlphaNumeric(str[i]))) nom[j++]=str[i];

	nom[j]=0;

	strcpy(str, getenv("TEMP"));
	strcat(str, "\\");
	strcat(str, nom);

	strcpy(nom, str);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

// EliminarFitxer: Elimina un fitxer del sistema de fitxers
// Es crida la comanda del sistema DEL

void EliminarFitxer(char *nom)
{
	char str[256];

	sprintf(str, "DEL %s", nom);
	system(str);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

// LogVariables: Escriu al log les variables de RedMon

void LogVariables(char *FileXES, char *FilePCL)
{
	char *RedMon_Port, *RedMon_Job, *RedMon_Printer, *RedMon_Machine, *RedMon_User, *RedMon_DocName;
	char *Temp, str[16384];

	RedMon_Port=getenv("REDMON_PORT");
	if (RedMon_Port==NULL) Log(ERRORLOG, "ERROR: Variable REDMON_PORT no trobada");

	RedMon_Job=getenv("REDMON_JOB");
	if (RedMon_Job==NULL) Log(ERRORLOG, "ERROR: Variable REDMON_JOB no trobada");

	RedMon_Printer=getenv("REDMON_PRINTER");
	if (RedMon_Printer==NULL) Log(ERRORLOG, "ERROR: Variable REDMON_PRINTER no trobada");

	RedMon_Machine=getenv("REDMON_MACHINE");
	if (RedMon_Machine==NULL) Log(ERRORLOG, "ERROR: Variable REDMON_MACHINE no trobada");

	RedMon_User=getenv("REDMON_USER");
	if (RedMon_User==NULL) Log(ERRORLOG, "ERROR: Variable REDMON_USER no trobada");

	RedMon_DocName=getenv("REDMON_DOCNAME");
	if (RedMon_DocName==NULL) Log(ERRORLOG, "ERROR: Variable REDMON_DOCNAME no trobada");

	Temp=getenv("TEMP");
	if (Temp==NULL) Log(ERRORLOG, "ERROR: Variable TEMP no trobada");

	Temp=getenv("TMP");
	if (Temp==NULL) Log(ERRORLOG, "ERROR: Variable TMP no trobada");

	sprintf(str, "%s\t%s\t\"%s\"\t\"%s\"\t%s\t%s\t%s", RedMon_Printer, ImpressoraPCL, FileXES, FilePCL, RedMon_Port, RedMon_Machine, RedMon_User);
	Log(PRINTLOG, str);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

// BuscarImpressora: Busca al fitxer de configuració la impressora que té com a nom el paràmetre que especifiquem
// Retorna 0 si no es troba, 1 si es troba

int BuscarImpressora(char *nom, char *impressora)
{
	FILE *fp;
	char linia[16384], Impressora1[16384], FontsFile[16384], Estat;
	char strorientacio[16384], strsize[16384], strlinespacing[16384], strlletres[16384], strdescripcio[16384];
	char strsafatatipus[16384], strsafatacodi[16384];
	int ret;

	fp=fopen(nom, "rb");
	if (fp==NULL)
	{
		sprintf(linia, "ERROR: no es pot obrir el fitxer de configuració \"%s\"", nom);
		Log(ERRORLOG, linia);
		exit(1);
	}

	do
	{
		ret=LlegirLinia(fp, linia);  if (ret<0) break;

		if (linia[0]=='#') continue;

		Estat = strtok(linia, "\t")[0];
		strcpy(Impressora1,	  strtok(NULL, "\t"));
		strcpy(strdescripcio, strtok(NULL, "\t"));
		strcpy(ImpressoraPCL, strtok(NULL, "\t"));
		strcpy(FontsFile,	  strtok(NULL, "\t"));

		if (strcmp(Impressora1,impressora)==0) break;
	}
	while (ret>=0);

	fclose(fp);

	if (ret<0) return -1;

	if (Estat!='1') return 0;

	fp=fopen(FontsFile, "rb");
	if (fp==NULL)
	{
		sprintf(linia, "ERROR: no es pot obrir el fitxer de definició de fonts \"%s\"", FontsFile);
		Log(ERRORLOG, linia);
		exit(1);
	}

	NumFonts=0;

	strcpy(SafataSuperior, "");
	strcpy(SafataInferior, "");
	strcpy(SafataManual,   "");

	do
	{
		ret=LlegirLinia(fp, linia); if (ret<0) break;

		if (linia[0]=='#') continue;

		if (linia[0]=='F')		// Font
		{
			// F	Titan10iso-P	(s0p10h10V	P	30	50

			Fonts[NumFonts].tipus =		  strtok(linia, "\t")[0];

			strcpy(Fonts[NumFonts].nom,	  strtok(NULL,  "\t"));
			strcpy(Fonts[NumFonts].codis, strtok(NULL,  "\t"));
			strcpy(strorientacio,		  strtok(NULL,  "\t"));
			strcpy(strsize,				  strtok(NULL,  "\t"));
			strcpy(strlinespacing,		  strtok(NULL,  "\t"));

			Fonts[NumFonts].orientacio=strorientacio[0];
	
			sscanf(strsize, "%i", &(Fonts[NumFonts].size));
			sscanf(strlinespacing, "%i", &(Fonts[NumFonts].linespacing));

			NumFonts++;
			if (NumFonts>=MAX_FONTS)
			{
				sprintf(linia, "ERROR: Hi ha més de %i fonts (cal ampliar MAX_FONTS)", NumFonts);
				Log(ERRORLOG, linia);
				exit(1);
			}
		}
		else if (linia[0]=='S') // Signatura
		{
			// S	ESCUT-P	A	E:\XES2PCL\RECURSOS\IMPRESSORA1\ESCUT-P.prn	0	200

			Fonts[NumFonts].tipus = strtok(linia, "\t")[0];
			Fonts[NumFonts].enviar_macro=0;

			strcpy(Fonts[NumFonts].nom,	  strtok(NULL,  "\t"));
			strcpy(strlletres,			  strtok(NULL,  "\t")); Fonts[NumFonts].lletra=strlletres[0];
			strcpy(Fonts[NumFonts].codis, strtok(NULL,  "\t"));
			strcpy(strsize,				  strtok(NULL,  "\t"));
			strcpy(strlinespacing,		  strtok(NULL,  "\t"));

			sscanf(strsize, "%i", &(Fonts[NumFonts].size));
			sscanf(strlinespacing, "%i", &(Fonts[NumFonts].linespacing));

			Fonts[NumFonts].num_macro=-1;

			NumFonts++;
			if (NumFonts>=MAX_FONTS)
			{
				sprintf(linia, "ERROR: Hi ha més de %i fonts (cal ampliar MAX_FONTS)", NumFonts);
				Log(ERRORLOG, linia);
				exit(1);
			}
		}
		else if (linia[0]=='T') // Safates
		{
			//	T	Sup	1
			//	T	Inf	2
			//	T	Man	5

			strtok(linia, "\t")[0];
			strcpy(strsafatatipus, strtok(NULL,  "\t"));
			strcpy(strsafatacodi,  strtok(NULL,  "\t"));

			if      (strcmp(strsafatatipus, "Sup")==0)
				strcpy(SafataSuperior, strsafatacodi);
			else if (strcmp(strsafatatipus, "Inf")==0)
				strcpy(SafataInferior, strsafatacodi);
			else if (strcmp(strsafatatipus, "Man")==0)
				strcpy(SafataManual, strsafatacodi);
			else
			{
				sprintf(linia, "ERROR: Tipus de safata desconeguda: %s", strsafatatipus);
				Log(ERRORLOG, linia);
				exit(1);
			}
		}
	}
	while (ret>=0);

	fclose(fp);

	ResetFonts();

	return 1;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

void ResetFonts(void)
{
	int i;

	for (i=0; (i<NumFonts); i++) Fonts[i].num_macro=-1;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

// Monitor: Envia missatge al pipe de nom indicat per la variable d'entorn XES2PCL_PIPE
// Aquest pipe es fa servir per monitoritzar les impressions

void Monitor(char *msg)
{
	HANDLE hPipe;
	char str[16384], *Pipe;
	DWORD numwritten, ret, dwMode;

	Pipe=getenv("XES2PCL_PIPE");
	if (Pipe==NULL) { Log(ERRORLOG, "ERROR: Variable d'entorn XES2PCL_PIPE no trobada"); exit(1); }

	while (1)
	{
		hPipe=CreateFile(Pipe, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hPipe!=INVALID_HANDLE_VALUE) break;

		ret=GetLastError();

		if (ret==ERROR_FILE_NOT_FOUND) return; // Monitor no engegat

		if (ret!=ERROR_PIPE_BUSY)
		{
			sprintf(str, "ERROR: obrint pipe - CreateFile (error %i)", ret);
			Log(ERRORLOG, str);
			exit(1);
		}

		if (!WaitNamedPipe(Pipe, 10000)) // Timeout = 10 segons
		{
			Log(ERRORLOG, "ERROR: obrint pipe - timeout (WaitNamedPipe)");
			exit(1);
		}
	}

	dwMode=PIPE_READMODE_MESSAGE;
	ret=SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL);
	if (!ret)
	{
		Log(ERRORLOG, "ERROR: obrint pipe - SetNamedPipeHandleState");
		exit(1);
	}

	ret=WriteFile(hPipe, msg, strlen(msg), &numwritten, NULL);
	if (!ret)
	{
		Log(ERRORLOG, "ERROR: escrivint a pipe - WriteFile");
		exit(1);
	}

	ret=CloseHandle(hPipe);
	if (!ret)
	{
		Log(ERRORLOG, "ERROR: tancant pipe - CloseHandle");
		exit(1);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

// SumFile: Genera un fitxer de nom name que és la concatenació dels fitxers de noms name1 i name2

void SumFile(char *name1, char *name2, char *name)
{
	FILE *fp, *fp1;
	char buffer[4096];
	int num;

	fp=fopen(name, "wb");
	if (fp==NULL)
	{
		sprintf(buffer, "ERROR: No es pot obrir el fitxer %s de sortida", name);
		Log(ERRORLOG, buffer);
		exit(1); 
	}

	fp1=fopen(name1, "rb");
	if (fp1==NULL)
	{
		sprintf(buffer, "ERROR: No es pot obrir el fitxer %s d'entrada", name1);
		Log(ERRORLOG, buffer);
		exit(1); 
	}
	while (1)
	{
		num=fread(buffer, 1, 4096, fp1);
		fwrite(buffer, 1, num, fp);
		if (num<4096) break;
	}
	fclose(fp1);

	fp1=fopen(name2, "rb");
	if (fp1==NULL)
	{
		sprintf(buffer, "ERROR: No es pot obrir el fitxer %s d'entrada", name2);
		Log(ERRORLOG, buffer);
		exit(1); 
	}
	while (1)
	{
		num=fread(buffer, 1, 4096, fp1);
		fwrite(buffer, 1, num, fp);
		if (num<4096) break;
	}
	fclose(fp1);

	fclose(fp);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------

void main(void)
{
	char FileXES[16384], FileMacros[16384], FilePCL[16384], FilePCLFinal[16384], str[16384], *fconfig, *RedMon_Printer;
	char strdate[16384], strtime1[16384], strtime2[16384];
	int ret;
	#if defined(DELLOG)
	char *flog;
	#endif
	long numbytes;

    _strdate(strdate);
	_strtime(strtime1);

	TipusDocument=TIPUS_HOST;
	EscapeCharacter=-1;
	Notacio=STAT_PRINTABLE;
	LastFont=-1;
	NumFont=0; // Per defecte, el primer font
	
	deltax=-90; deltay=0;

	NumMacros=0;

	maxlen=0;

	// Si està definit DELLOG, el log es neteja inicialment, sinó, s'afegirà al final

	#if defined(DELLOG)
	flog=getenv("XES2PCL_LOG");
	if (flog!=NULL) EliminarFitxer(flog);
	#endif

	// Llegeix la variable d'entorn XES2PCL_CONFIG, la qual conté el camí del fitxer de configuració
	// Si no existeix, genera un error i surt del programa

	fconfig=getenv("XES2PCL_CONFIG");
	if (fconfig==NULL) { Log(ERRORLOG, "ERROR: variable d'entorn XES2PCL_CONFIG no trobada"); exit(1); }

	// Llegeix la variable d'entorn REDMON_PRINTER, la qual és retornada pel monitor de port RedMon
	// Aquesta variable conté el nom de la impressora que s'emula
	// Si no es pot llegir, genera un error i surt del programa

	RedMon_Printer=getenv("REDMON_PRINTER");
	if (RedMon_Printer==NULL) { Log(ERRORLOG, "ERROR: Variable d'entorn REDMON_PRINTER no trobada"); exit(1); }

	RedMon_Job=getenv("REDMON_JOB");
	if (RedMon_Job==NULL) { Log(ERRORLOG, "ERROR: Variable d'entorn REDMON_JOB no trobada"); exit(1); }

	// Busca el nom de la impressora XES emulada al fitxer de configuració
	// Si no s'hi troba, la funció BuscarImpressora retorna -1, genera un error i surt del programa
	// Si es troba però està inactiva (estat a 0), sortir

	ret=BuscarImpressora(fconfig, RedMon_Printer);
	if (ret==0)
	{
		sprintf(str, "ATENCIÓ: impressora %s inactiva", RedMon_Printer);
		Log(ERRORLOG, str);
		exit(1); // Inactiva
	}

	if (ret<0)
	{
		sprintf(str, "ERROR: impressora %s no trobada", RedMon_Printer);
		Log(ERRORLOG, str);
		exit(1);
	}

	// Si FileXES és "stdin",  es llegeix l'entrada en format XES de l'entrada estàndard (stdin)
	// Si FilePCL és "stdout", s'escriu la sortida en format PCL a la sortida estàndard (stdout)

	// Si FilePCL ha de ser un fitxer temporal, el nom ha de ser únic
	// Per tal de garantir-ne la unicitat, el construïm amb el nom de la impressora seguit del número de job

	ConstruirNom(FileMacros);
	strcpy(FilePCL,      FileMacros);
	strcpy(FilePCLFinal, FileMacros);
	
	strcat(FileMacros,   "-1.prn");
	strcat(FilePCL,      "-2.prn");
	strcat(FilePCLFinal, ".prn");

	strcpy(FileXES,		 "stdin");

	//*********************************************************
	//strcpy(FileXES,	"E:\\projectes\\XES2PCL\\captures\\SGIPRHC2.TXT"); // OK
	//strcpy(FileXES,	"E:\\projectes\\XES2PCL\\captures\\2SGI1000.TXT"); // ERR
	//strcpy(FileXES,	"E:\\projectes\\XES2PCL\\captures\\PRT16.TXT"); // ERR
	//strcpy(FileXES,	"E:\\projectes\\XES2PCL\\captures\\DIETES.TXT"); 
	//strcpy(FileXES,	"E:\\projectes\\XES2PCL\\captures\\DO12A.TXT"); 
	//strcpy(FileXES,	"E:\\projectes\\XES2PCL\\captures\\HOF1007D.TXT"); 
	//strcpy(FileXES,	"E:\\projectes\\XES2PCL\\captures\\tested\\HCE1010C-imma.TXT"); 
	//strcpy(FileXES,	"E:\\projectes\\XES2PCL\\captures\\tested\\taxat.TXT"); 
	//strcpy(FileXES,	"E:\\projectes\\XES2PCL\\captures\\SGIAS2.TXT"); // 21-1-2003
	//strcpy(FileXES,	"E:\\projectes\\XES2PCL\\captures\\SGI\\SGI1CA96.TXT"); 

	//*********************************************************

	// Converteix el fitxer en format XES (FileXES) a format PCL (FilePCL)

	Convertir(FileXES, FilePCL);

	// Enviar macros PCL

	GenerarMacros(FileMacros);

	// Concatenar FileMacros i FilePCL i generar FilePCLFinal

	SumFile(FileMacros, FilePCL, FilePCLFinal);
	// Borrar fitxers temporals
	EliminarFitxer(FileMacros);
	EliminarFitxer(FilePCL);

	//LogVariables(FileXES, FilePCL);

	// Enviar el fitxer temporal en format PCL a la impressora Windows corresponent

	numbytes=PrintFile(FilePCLFinal, ImpressoraPCL);

	// Borrar el fitxer temporal d'impressió

	#if defined(DELFILE)
	EliminarFitxer(FilePCLFinal);
	#endif

	_strtime(strtime2);

	RedMon_Printer=getenv("REDMON_PRINTER");
	if (RedMon_Printer==NULL) { Log(ERRORLOG, "ERROR: Variable d'entorn REDMON_PRINTER no trobada"); exit(1); }

	RedMon_Job=getenv("REDMON_JOB");
	if (RedMon_Job==NULL) { Log(ERRORLOG, "ERROR: Variable d'entorn REDMON_JOB no trobada"); exit(1); }

	sprintf(str, "%s\t%s\t%s\t%s\t%s\t%s\t%li", strdate, strtime1, strtime2, RedMon_Printer, RedMon_Job, ImpressoraPCL, numbytes);
	Monitor(str);

	sprintf(str, "%s\t%s\t%s\t%s\t%s\t%s\t%li\t%i", strdate, strtime1, strtime2, RedMon_Printer, RedMon_Job, ImpressoraPCL, numbytes, maxlen);
	Log(PRINTLOG, str);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------
