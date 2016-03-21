#ifndef _dcopy_progress_
#define _dcopy_progress_

#include "pfm/main.h"
#include "pfm/panel.h"

// copy files
void DlgCopyFiles ( Panel_c * pSource, const wchar_t * szDest, SelectedFileList_t & tList );
// move files
void DlgMoveFiles ( Panel_c * pSource, const wchar_t * szDest, SelectedFileList_t & tList );
// rename files
void DlgRenameFiles ( Panel_c * pSource, const wchar_t * szDest, SelectedFileList_t & tList );

#endif