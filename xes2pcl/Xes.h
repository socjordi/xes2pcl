//-------------------------------------------------------------------------------------------------------------------------------------------------
//
//	xes.h
//
//	Conversió de format XES (Xerox Escape Sequence) a format PCL (HP Printer Command Language)
//
//	Abril 2001
//
//-------------------------------------------------------------------------------------------------------------------------------------------------

void Log(int LogType, char *str);

void CalcularOrigen(void);

void StrReplace(char *str, char *substr, char *rep);

int LlegirLinia(FILE *fp, char *linia);

int GetParam(char *linia, int *pos, int *param);

int BuscarFontPerNom(char *NomFont);

int BuscarFontPerNum(int NumFont);

void EscriuGrafic(char *liniaout);

void ConvertirLinia(char *liniain, char *liniaout);

void GenerarMacros(char *name);

void ComprovarDefinicioEscape(char *linia);

void Homogeneitzar(char *linia);

void Convertir(char *FileXES, char *FilePCL);

long PrintFile(char *File, char *PrinterName);

void ConstruirNom(char *nom);

void EliminarFitxer(char *nom);

void LogVariables(char *FileXES, char *FilePCL);

int BuscarImpressora(char *nom, char *impressora);

void Monitor(char *msg);

void SumFile(char *name1, char *name2, char *name);

void ResetFonts(void);
