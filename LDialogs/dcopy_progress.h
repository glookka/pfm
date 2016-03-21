#ifndef _dcopy_progress_
#define _dcopy_progress_

#include "LCore/cmain.h"
#include "LPanel/pbase.h"

// copy files
void DlgCopyFiles ( Panel_c * pSource, const wchar_t * szDest, FileList_t & tList );
// move files
void DlgMoveFiles ( Panel_c * pSource, const wchar_t * szDest, FileList_t & tList );
// rename files
void DlgRenameFiles ( Panel_c * pSource, const wchar_t * szDest, FileList_t & tList );

#endif