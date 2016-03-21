#ifndef _dialogs_find_
#define _dialogs_find_

#include "pfm/main.h"
#include "LFile/ffind.h"

bool FindRequestDlg ( FileSearchSettings_t & tSettings );
bool FindProgressDlg ( const SelectedFileList_t & tList, const FileSearchSettings_t & tSettings, Str_c & sDir, Str_c & sFile );

#endif