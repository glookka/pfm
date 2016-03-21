#ifndef _fmkdir_
#define _fmkdir_

#include "LCore/cmain.h"
#include "LFile/ferrors.h"

class Panel_c;

bool MakeDirectory ( const Str_c & sDir );
bool MakeDirectory ( const Str_c & sDir, const Str_c & sRootDir );

#endif
