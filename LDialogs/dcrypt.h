#ifndef _dcrypt_
#define _dcrypt_

#include "LCore/cmain.h"
#include "LFile/fiterator.h"

int ShowCryptDialog ( Panel_c * pSource, bool bEncrypt, FileList_t & tList, Str_c & sPassword );
void ShowCryptProgressDlg ( Panel_c * pSource, bool bEncrypt, FileList_t & tList, const Str_c & sPassword );

#endif