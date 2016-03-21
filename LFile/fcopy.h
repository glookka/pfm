#ifndef _fcopy_
#define _fcopy_

#include "pfm/main.h"
#include "pfm/progress.h"
#include "pfm/dialogs/errors.h"

enum DestType_e
{
	 DEST_INTO
	,DEST_OVER
};

// move files within one volume
class FileMover_c
{
public:
						FileMover_c ( const Str_c & sDest, SelectedFileList_t & tList, MarkCallback_t fnMarkCallback );

	bool				MoveNext ();
	void				SetWindow ( HWND hWnd );
	bool 				IsInDialog () const;

private:
	static const int 	FILES_IN_PASS = 20;

	MarkCallback_t		m_fnMarkCallback;

	HWND				m_hWnd;
	SelectedFileList_t *		m_pList;
	Str_c		m_sDestDir;
	DestType_e			m_eDestType;
	FileIteratorPanel_c m_tFileIterator;
	bool				m_bFirstPass;

	bool				Setup ();
	Str_c 				GenerateDestName ( const Str_c & sSourceName ) const;
	OperationResult_e 	TryToMoveFile ( const Str_c & sSource, const Str_c & sDest, bool bCaseRename );
};


// file step-by-step copy class
class FileCopier_c : public FileProgress_c
{
public:
	enum CopyMode_e
	{
		 MODE_COPY
		,MODE_MOVE_VOLUMES
	};


						FileCopier_c ( const Str_c & sDest, SelectedFileList_t & tList, bool bMove,
								SlowOperationCallback_t fnPrepareCallback, MarkCallback_t fnMarkCallback );
						~FileCopier_c ();

	bool				IsInDialog () const;
	CopyMode_e			GetCopyMode () const;
	bool				DoCopyWork ();				// returns false when copy is finished
	void				Cancel ();					// cancels the copy

	void				SetWindow ( HWND hWnd );

	const wchar_t *		GetSourceFileName () const;
	const wchar_t *		GetDestFileName () const;

private:
	enum CopyState_e
	{
		 STATE_INITIAL
		,STATE_START_FILE
		,STATE_COPY_FILE
		,STATE_CANCELLED
	};

	enum
	{
		COPY_CHUNK_SIZE	= 131072
	};

	char *				m_pCopyBuffer;

	DestType_e			m_eDestType;

	SelectedFileList_t *		m_pList;
	FileIteratorPanel_c	m_tFileIterator;

	CopyMode_e			m_eMode;
	CopyState_e			m_eState;
	Str_c				m_sSourceFileName;
	Str_c				m_sSource;
	Str_c				m_sDest;
	Str_c				m_sDestDir;
	HANDLE				m_hSource;
	HANDLE				m_hDest;
	DWORD				m_dwSourceAttrib;
	FILETIME			m_tModifyTime;

	bool				m_bMove;
	HWND				m_hWnd;
	MarkCallback_t		m_fnMarkCallback;

	bool				Setup ();
	void				Reset ();
	void				Cleanup ( bool bError );

	bool				GenerateNextFileName ();
	void				MoveToNextFile ();

	OperationResult_e	PrepareSimpleFile ();		// prepares a simple file (not a dir)
	OperationResult_e	PrepareFile ();
	OperationResult_e	CopyNextChunk ();
	OperationResult_e	FinishFile ();

	void				MarkSourceOk ();

	Str_c				GenerateDestName ( const Str_c & sSourceName ) const;
};

#endif
