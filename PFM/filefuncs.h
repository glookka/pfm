#ifndef _filefuncs_
#define _filefuncs_

#include "main.h"
#include "std.h"

void FMDeleteFiles ();
void FMCreateShortcut ( const Str_c & sDest, bool bShowDialog );
void FMCopyMoveFiles ();
void FMRenameFiles ();
void FMOpenInOpposite ();
void FMOpenFolder ();
void FMFileProperties ();
void FMOpenWith ();
void FMRunParams ();
void FMViewFile ();
void FMExecuteFile ( const Str_c & sFile );

void FMSendIR ();
void FMSendBT ();

void FMExit ();
void FMSelectFilter ( bool bMark );
void FMMkDir ();
void FMFindFiles ();
void FMAddBookmark ();

#endif