// Filename:-	oddbits.h
//

#ifndef ODDBITS_H
#define ODDBITS_H


char *va(char *format, ...);
bool FileExists (LPCSTR psFilename);

void ErrorBox(const char *sString);
void InfoBox(const char *sString);
void WarningBox(const char *sString);
//
// (Afx message boxes appear to be MB_TASKMODAL anyway, so no need to specify)
//
#define GetYesNo(psQuery) (!!(AfxMessageBox(psQuery,MB_YESNO|MB_ICONWARNING)==IDYES))


char *scGetTempPath(void);
char *InputLoadFileName(char *psInitialLoadName, char *psCaption, const char *psInitialDir, char *psFilter);
long filesize(FILE *handle);
int  scLoadFile (LPCSTR psPathedFilename, void **bufferptr, bool bBinaryMode = true );
void Filename_RemoveBASEQ(CString &string);
void Filename_RemoveFilename(CString &string);
void Filename_BaseOnly(CString &string);
void Filename_AccountForLOD(CString &string, int iLODLevel);

#define StartWait() HCURSOR hcurSave = SetCursor(::LoadCursor(NULL, IDC_WAIT))
#define EndWait()   SetCursor(hcurSave)


#endif	// #ifndef ODDBITS_H

/////////////////// eof ////////////////////


