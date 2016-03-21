#ifndef _cfile_
#define _cfile_

#include "LCore/cmain.h"
#include "LCore/cstr.h"

// extension
Str_c GetExt ( const wchar_t * szFileName );

// path (w/o filename)
Str_c GetPath ( const wchar_t * szFileName );

// filename w/o path
Str_c GetName ( const wchar_t * szPath );

// fast name/ext getter. unsafe.
const wchar_t * GetNameExtFast ( const wchar_t * szNameExt, wchar_t * szName = NULL );

// split path into path, file name and extension
void SplitPath ( const Str_c & sFileName, Str_c & sDir, Str_c & sName, Str_c & sExt );

// are files on different drives/cards/volumes?
bool AreFilesOnDifferentVolumes ( const Str_c & sFile1, const Str_c & sFile2 );

// is this a storage card?
bool IsStorageCard ( const Str_c & sFile );

// append a slash
Str_c & AppendSlash ( Str_c & sStr );

// remove a slash
void RemoveSlash ( Str_c & sStr );

// ends in slash?
bool EndsInSlash ( const Str_c & sStr );

// cut string to first slash
void CutToSlash ( Str_c & sStr );

// remove trailing and heading slashes
Str_c RemoveSlashes ( const Str_c & sStr );

// replace all backslashes with slashes
void ReplaceSlashes ( Str_c & sPath );

// check if .lnk is a folder or a file
bool DecomposeLnk ( Str_c & sPath, Str_c & sParams, bool & bDir );

// current executable path
Str_c GetExecutablePath ();

#endif