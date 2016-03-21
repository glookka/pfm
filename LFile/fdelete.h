#ifndef _fdelete_
#define _fdelete_

#include "LCore/cmain.h"
#include "LFile/fiterator.h"
#include "LFile/ferrors.h"

//
// deletes all marked/selected files from a panel
//
class FileDelete_c
{
public:
	static const int 	FILES_IN_PASS = 20;

						FileDelete_c ( FileList_t & tList );
				
	bool				PrepareFileName ();
	bool				DeleteNext ();
		
	bool				IsInDialog  () const;
	void				SetWindow ( HWND hWnd );
	int					GetNumDeletedFiles () const;
	Str_c				GetFileToDelete () const;

private:
	HWND				m_hWnd;
	int					m_nFiles;
	FileList_t &		m_tList;

	FileIteratorPanel_c	m_tIterator;

	OperationResult_e	TryToDeleteDir ( const Str_c & sFileName );
	OperationResult_e	TryToDeleteFile ( const Str_c & sFileName );
};

#endif
