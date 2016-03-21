#include "pch.h"

#include "LFile/fcopy.h"
#include "LCore/clog.h"
#include "LCore/ctimer.h"
#include "LCore/cfile.h"
#include "LFile/fmkdir.h"
#include "LSettings/sconfig.h"
#include "LSettings/slocal.h"
#include "LPanel/pbase.h"
#include "LDialogs/derrors.h"

#include "Resources/resource.h"


//////////////////////////////////////////////////////////////////////////
// helper
static bool SetupOperation ( HWND hWnd, const FileList_t & tList, Str_c & sDestDir, DestType_e & eDestType, bool & bCreateDir )
{
	// check if we have any dirs as source
	bool bHaveSourceDirs = false;
	for ( int i = 0; i < tList.m_dFiles.Length () && !bHaveSourceDirs; ++i )
		if ( tList.m_dFiles [i]->m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			bHaveSourceDirs = true;

	bool bEndInSlash = sDestDir.Ends ( L"\\" );
	RemoveSlash ( sDestDir );

	WIN32_FIND_DATA tFindData;
	HANDLE hFound = FindFirstFile ( sDestDir, &tFindData );

	if ( hFound != INVALID_HANDLE_VALUE )
	{
		FindClose ( hFound );

		if ( tFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			AppendSlash ( sDestDir );

			// renaming one directory
			if ( tList.m_dFiles.Length () == 1 )
			{
				Str_c sSourceFile = tList.m_sRootDir + tList.m_dFiles [0]->m_tData.cFileName;
				AppendSlash ( sSourceFile );

				Str_c sTestDir = sDestDir;

				eDestType = sSourceFile.ToLower () == sTestDir.ToLower () ? DEST_OVER : DEST_INTO;
			}
			else
				eDestType = DEST_INTO;
		}
		else
		{
			if ( bHaveSourceDirs )
			{
				ShowErrorDialog ( hWnd, Txt ( T_MSG_ERROR ), Txt ( T_MSG_WRONG_DEST ), IDD_ERROR_OK );
				return false;
			}
			else
				eDestType = DEST_OVER;
		}
	}
	else
	{
		if ( bEndInSlash )
		{
			AppendSlash ( sDestDir );
			eDestType = DEST_INTO;
			bCreateDir = true;
		}
		else if ( sDestDir.Empty () )
		{
			AppendSlash ( sDestDir );
			eDestType = DEST_INTO;
		}
		else if ( bHaveSourceDirs )
		{
			AppendSlash ( sDestDir );
			eDestType = DEST_INTO;
			bCreateDir = true;
		}
		else
			eDestType = DEST_OVER;
	}

	return true;
}


///////////////////////////////////////////////////////////////////////////////////////////
// move files within one volume
FileMover_c::FileMover_c ( const Str_c & sDest, FileList_t & tList, MarkCallback_t fnMarkCallback )
	: m_pList 			( &tList )
	, m_sDestDir		( sDest )
	, m_hWnd 			( NULL )
	, m_eDestType		( DEST_OVER )
	, m_bFirstPass		( true )
	, m_fnMarkCallback	( fnMarkCallback )
{
}

bool FileMover_c::Setup ()
{
	Assert ( m_pList );

	bool bCreateDir = false;

	if ( ! SetupOperation ( m_hWnd, *m_pList, m_sDestDir, m_eDestType, bCreateDir ) )
		return false;

	if ( m_eDestType == DEST_OVER )
		g_tErrorList.Reset ( OM_RENAME );
	else
		g_tErrorList.Reset ( OM_MOVE );

	m_tFileIterator.IterateStart ( *m_pList, false );

	bool bRes = true;

	if ( bCreateDir )
	{
		if ( m_pList->m_dFiles.Length () == 1 && ( m_pList->m_dFiles [0]->m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
		{
			RemoveSlash ( m_sDestDir );
			m_eDestType = DEST_OVER;
		}
		else
			bRes = MakeDirectory ( m_sDestDir );
	}

	if ( m_eDestType == DEST_OVER )
		g_tErrorList.Reset ( OM_RENAME );
	else
		g_tErrorList.Reset ( OM_MOVE );

	return bRes;
}

bool FileMover_c::MoveNext ()
{
	Assert ( m_pList );

	if ( m_bFirstPass )
	{
		if ( !Setup () )
			return false;

		m_bFirstPass = false;
	}

	Str_c sDest, sSource;

	bool bFound = true;
	for ( int i = 0; i < FILES_IN_PASS && bFound; ++i )
	{
		while ( ( bFound = m_tFileIterator.IterateNext () ) == true )
		{
			if ( ! m_tFileIterator.Is2ndPassDir () )
			{
				sSource	= m_pList->m_sRootDir + m_tFileIterator.GetFileName ();

				Str_c sDest = GenerateDestName ( m_tFileIterator.GetFileNameSkipped () );

				Str_c sTmp1 = RemoveSlashes ( sSource );
				Str_c sTmp2 = RemoveSlashes ( sDest );

				if ( sTmp1 == sTmp2 )
				{
					g_tErrorList.ShowErrorDialog ( m_hWnd, EC_MOVE_ONTO_ITSELF, false, sSource, sDest );
					return false;
				}

				OperationResult_e eRes = TryToMoveFile ( sSource, sDest, sTmp1.ToLower () == sTmp2.ToLower () );
				if ( eRes == RES_CANCEL )
					return false;

				if ( m_fnMarkCallback && m_tFileIterator.IsRootDir () )
					m_fnMarkCallback ( sSource, false );
			}
		}
	}

	return bFound;
}

void FileMover_c::SetWindow ( HWND hWnd )
{
	m_hWnd = hWnd;
}

bool FileMover_c::IsInDialog () const
{
	return g_tErrorList.IsInDialog ();
}

Str_c FileMover_c::GenerateDestName ( const Str_c & sSourceName ) const
{
	switch ( m_eDestType )
	{
	case DEST_INTO:
		return m_sDestDir + sSourceName;
	case DEST_OVER:
		return m_sDestDir;
	}
	return L"";
}

OperationResult_e FileMover_c::TryToMoveFile ( const Str_c & sSource, const Str_c & sDest, bool bCaseRename )
{
	BOOL bRes;
	
	if ( sSource == sDest )
	{
		g_tErrorList.ShowErrorDialog ( m_hWnd, EC_MOVE_ONTO_ITSELF, false, sSource, sDest );
		return RES_ERROR;
	}

	// check if dest exists
	bool bDestExists = GetFileAttributes ( sDest ) != 0xFFFFFFFF;
	if ( bDestExists && ! bCaseRename )
	{
		bool bAsk = g_tConfig.conf_move_over;
		if ( bAsk )
		{
			int iErrAction = g_tErrorList.ShowErrorDialog ( m_hWnd, EC_DEST_EXISTS, false, sSource, sDest );

			if ( iErrAction != IDC_OVER )
				return iErrAction == IDCANCEL ? RES_CANCEL : RES_ERROR;
		}

		DWORD dwDestAttrib;

		// get dest attributes
		TRY_FUNC ( dwDestAttrib, GetFileAttributes ( sDest ), 0xFFFFFFFF, sSource, sDest );

		DWORD dwAttribToSet = dwDestAttrib;

		// test for read-only-ness
		dwAttribToSet &= ~ ( FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM );
		TRY_FUNC ( bRes, SetFileAttributes ( sDest, dwAttribToSet ), FALSE, sSource, sDest );
		TRY_FUNC ( bRes, DeleteFile ( sDest ), FALSE, sSource, sDest );
	}

	TRY_FUNC ( bRes, MoveFile ( sSource, sDest ), FALSE, sSource, sDest );

	return RES_OK;
}



///////////////////////////////////////////////////////////////////////////////////////////
// the file copier class
FileCopier_c::FileCopier_c ( const Str_c & sDest, FileList_t & tList, bool bMove,
		SlowOperationCallback_t fnPrepareCallback, MarkCallback_t fnMarkCallback )
	: m_fnMarkCallback		( fnMarkCallback )
	, m_pCopyBuffer			( NULL )
	, m_eDestType			( DEST_OVER )
{
	Reset ();

	if ( bMove )
	{
		Assert ( AreFilesOnDifferentVolumes ( tList.m_sRootDir, sDest ) );
		m_eMode = MODE_MOVE_VOLUMES;
	}
	else
		m_eMode = MODE_COPY;

	m_pCopyBuffer = new char [COPY_CHUNK_SIZE];
	Assert ( m_pCopyBuffer );

	m_pList	= &tList;
	m_sDestDir = sDest;

	g_tErrorList.Reset ( bMove ? OM_MOVE : OM_COPY );

	int nFiles, nFolders;
	DWORD dwA1, dwA2;
	CalcTotalSize ( tList, m_uTotalSize, nFiles, nFolders, dwA1, dwA2, fnPrepareCallback );
}


FileCopier_c::~FileCopier_c ()
{
	SafeDeleteArray ( m_pCopyBuffer );
}


bool FileCopier_c::IsInDialog () const
{
	return g_tErrorList.IsInDialog ();
}


FileCopier_c::CopyMode_e FileCopier_c::GetCopyMode () const
{
	return m_eMode;
}

bool FileCopier_c::Setup ()
{
	Assert ( m_pList );

	bool bCreateDir = false;

	if ( ! SetupOperation ( m_hWnd, *m_pList, m_sDestDir, m_eDestType, bCreateDir ) )
		return false;

	if ( bCreateDir )
	{
		OperationMode_e eMode = g_tErrorList.GetMode ();
		bool bMkDirRes = MakeDirectory ( m_sDestDir );
		g_tErrorList.SetOperationMode ( eMode );

		if ( ! bMkDirRes )
			return false;
	}
	
	m_tFileIterator.IterateStart ( *m_pList );

	// skip source root directory in case we're copying just one dir
	if ( bCreateDir && m_pList->m_dFiles.Length () == 1 && ( m_pList->m_dFiles [0]->m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
		m_tFileIterator.SkipRoot ( true );

	return true;
}

bool FileCopier_c::DoCopyWork ()
{
	if ( IsInDialog () )
		return true;

	OperationResult_e eResult = RES_OK;
	OperationResult_e eResultFin = RES_OK;

	switch ( m_eState )
	{
	case STATE_INITIAL:
		if ( ! Setup () )
		{
			Reset ();
			return false;
		}

		m_eState = STATE_START_FILE;
		break;

	case STATE_START_FILE:
		{
			if ( ! GenerateNextFileName () )
			{
				Reset ();
				return false;
			}

			eResult = PrepareFile ();
			switch ( eResult )
			{
			case RES_ERROR:
				Cleanup ( false );
				MoveToNextFile ();
				break;
			case RES_CANCEL:
				Cleanup ( false );
				break;
			default:
				m_eState = STATE_COPY_FILE;
				break;
			}
		}
		break;

	case STATE_COPY_FILE:
		eResult = CopyNextChunk ();
		switch ( eResult )
		{
		case RES_ERROR:
			Cleanup ( true );
			MoveToNextFile ();
			break;
		case RES_FINISHED:
			eResultFin = FinishFile ();
			switch ( eResultFin )
			{
			case RES_ERROR:
				Cleanup ( true );
				MoveToNextFile ();
				break;
			case RES_CANCEL:
				Cleanup ( true );
				break;
			default:
				Cleanup ( false );
				MarkSourceOk ();
				MoveToNextFile ();
				break;
			}
			break;
		case RES_CANCEL:
			Cleanup ( true );
			break;
		}
		break;
	}

	return eResult != RES_CANCEL && eResultFin != RES_CANCEL;
}


void FileCopier_c::Cancel ()
{
	switch ( m_eState )
	{
	case STATE_START_FILE:
		Cleanup ( false );
		break;
	case STATE_COPY_FILE:
		Cleanup ( true );
		break;
	}

	m_eState = STATE_CANCELLED;
}


void FileCopier_c::SetWindow ( HWND hWnd )
{
	m_hWnd = hWnd;
}


const wchar_t *	FileCopier_c::GetSourceFileName () const
{
	return m_sSource;
}


const wchar_t *	FileCopier_c::GetDestFileName () const
{
	return m_sDest;
}

void FileCopier_c::Reset ()
{
	FileProgress_c::Reset ();

	g_tErrorList.Reset ( OM_COPY );

	m_pList				= NULL;
	m_eState			= STATE_INITIAL;
	m_eMode				= MODE_COPY;
	m_sSourceFileName	= L"";
	m_sSource			= L"";
	m_sDest				= L"";
	m_sDestDir			= L"";
	m_hSource			= NULL;
	m_hDest				= NULL;
	m_dwSourceAttrib	= 0;
	m_hWnd				= NULL;

	SafeDeleteArray ( m_pCopyBuffer );
}


void FileCopier_c::Cleanup ( bool bError )
{
	if ( m_hSource && m_hSource != INVALID_HANDLE_VALUE )
		CloseHandle ( m_hSource );

	if ( m_hDest && m_hDest != INVALID_HANDLE_VALUE )
		CloseHandle ( m_hDest );

	if ( bError )
	{
		if ( m_dwSourceAttrib & FILE_ATTRIBUTE_DIRECTORY )
			RemoveDirectory ( m_sDest );
		else
			DeleteFile ( m_sDest );
	}
}


bool FileCopier_c::GenerateNextFileName ()
{
	Assert ( m_eState == STATE_START_FILE );

	bool bRes = false;

	while ( m_tFileIterator.IterateNext () )
	{
		if ( m_tFileIterator.Is2ndPassDir () )
		{
			if ( m_eMode != MODE_COPY )
			{
				Str_c sLastDir = m_pList->m_sRootDir + m_tFileIterator.GetFileName ();
				if ( ! sLastDir.Empty () )
					RemoveDirectory ( sLastDir );
			}
		}
		else
		{
			m_sSourceFileName = m_tFileIterator.GetFileName ();
			m_sSource	= m_pList->m_sRootDir + m_sSourceFileName;

			m_bNameFlag = true;
			m_sDest		= GenerateDestName ( m_tFileIterator.GetFileNameSkipped () );

			Str_c sTmpS = RemoveSlashes ( m_sSource );
			Str_c sTmpD = RemoveSlashes ( m_sDest );
			Str_c sTmpDD = RemoveSlashes ( m_sDestDir );

			sTmpS.ToLower ();
			sTmpD.ToLower ();
			sTmpDD.ToLower ();

			if ( sTmpS == sTmpD || sTmpS == sTmpDD )
			{
				g_tErrorList.ShowErrorDialog ( m_hWnd, EC_COPY_ONTO_ITSELF, false, m_sSource, m_sDest );
				return false;
			}

			bRes = true;
			break;
		}
	}

	return bRes;
}

void FileCopier_c::MoveToNextFile ()
{
	MoveToNext ( m_uProcessedFile, m_uSourceSize );
	m_eState = STATE_START_FILE;
}

OperationResult_e FileCopier_c::PrepareSimpleFile ()
{
	BOOL bResult = FALSE;
	ULARGE_INTEGER uAvailable, uTotal;

	// open source file
	TRY_FUNC ( m_hSource, CreateFile ( m_sSource, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL ), INVALID_HANDLE_VALUE, m_sSource, m_sDest );

	// get file size
	TRY_FUNC ( m_uSourceSize.LowPart, GetFileSize ( m_hSource, &m_uSourceSize.HighPart ), 0xFFFFFFFF, m_sSource, m_sDest );

	// get free space
	TRY_FUNC ( bResult, GetDiskFreeSpaceEx ( GetPath ( m_sDest ), &uAvailable, &uTotal, NULL ), FALSE, m_sSource, m_sDest );

	// get source file time
	TRY_FUNC ( bResult, GetFileTime ( m_hSource, NULL, NULL, &m_tModifyTime ), FALSE, m_sSource, m_sDest );

	// check available space
	int iErrAction = IDC_RETRY;
	while ( m_uSourceSize.QuadPart > uAvailable.QuadPart && iErrAction == IDC_RETRY  )
		iErrAction = g_tErrorList.ShowErrorDialog ( m_hWnd, EC_NOT_ENOUGH_SPACE, false, m_sSource, m_sDest );

	if ( m_uSourceSize.QuadPart > uAvailable.QuadPart )
		return iErrAction == IDCANCEL ? RES_CANCEL : RES_ERROR;

	return RES_OK;
}


OperationResult_e FileCopier_c::PrepareFile ()
{
	Assert ( m_eState == STATE_START_FILE );

	BOOL bResult = FALSE;
	DWORD dwDestAttrib;

	// check if source and dest are different
	if ( m_sSource == m_sDest )
	{
		int iErrAction = g_tErrorList.ShowErrorDialog ( m_hWnd, EC_COPY_ONTO_ITSELF, false, m_sSource, m_sDest );
		return iErrAction == IDCANCEL ? RES_CANCEL : RES_ERROR;
	}

	// get file attributes
	TRY_FUNC ( m_dwSourceAttrib, GetFileAttributes ( m_sSource ), 0xFFFFFFFF, m_sSource, m_sDest );

	bool bDirectory = !! ( m_dwSourceAttrib & FILE_ATTRIBUTE_DIRECTORY );
	if ( ! bDirectory )
	{
		OperationResult_e eRes = PrepareSimpleFile ();
		if ( eRes != RES_OK )
			return eRes;
	}

	// check if dest exists
	WIN32_FIND_DATA tFindData;
	HANDLE hFound = FindFirstFile ( m_sDest, &tFindData );
	bool bDestExists = hFound != INVALID_HANDLE_VALUE;
	if ( bDestExists )
		FindClose ( hFound );

	if ( bDestExists && ! bDirectory )
	{
		bool bAsk = GetCopyMode () == MODE_COPY ? g_tConfig.conf_copy_over : g_tConfig.conf_move_over;
		if ( bAsk )
		{
			int iErrAction = g_tErrorList.ShowErrorDialog ( m_hWnd, EC_DEST_EXISTS, false, m_sSource, m_sDest );

            if ( iErrAction != IDC_OVER )
				return iErrAction == IDCANCEL ? RES_CANCEL : RES_ERROR;
		}

		// get dest attributes
		TRY_FUNC ( dwDestAttrib, GetFileAttributes ( m_sDest ), 0xFFFFFFFF, m_sSource, m_sDest );

		DWORD dwAttribToSet = dwDestAttrib;
		dwAttribToSet &= ~ ( FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM );
		TRY_FUNC ( bResult, SetFileAttributes ( m_sDest, dwAttribToSet ), FALSE, m_sSource, m_sDest );
	}

	if ( bDirectory )
	{
		// create destination directory
		if ( ! bDestExists )
			TRY_FUNC ( bResult, CreateDirectory ( m_sDest, NULL ), FALSE, m_sSource, m_sDest );
	}
	else
	{
		// open destination file
		TRY_FUNC ( m_hDest, CreateFile ( m_sDest, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, m_dwSourceAttrib, NULL ), INVALID_HANDLE_VALUE, m_sSource, m_sDest );
	}

	return RES_OK;
}


OperationResult_e FileCopier_c::CopyNextChunk ()
{
	Assert ( m_eState == STATE_COPY_FILE );
	
	BOOL bResult;
	DWORD uBytesRead, uBytesWritten;

	if ( m_dwSourceAttrib & FILE_ATTRIBUTE_DIRECTORY )
		return RES_FINISHED;

	Assert ( m_pCopyBuffer );

	TRY_FUNC ( bResult, ReadFile ( m_hSource, (void *) m_pCopyBuffer, COPY_CHUNK_SIZE, &uBytesRead, NULL ), FALSE, m_sSource, m_sDest  );
	if ( uBytesRead )
	{
		TRY_FUNC ( bResult, WriteFile ( m_hDest, (void *) m_pCopyBuffer, uBytesRead, &uBytesWritten, NULL ), FALSE, m_sSource, m_sDest  );
		m_uProcessedFile.QuadPart += uBytesWritten;
		m_uProcessedTotal.QuadPart += uBytesWritten;
		UpdateProgress ();
	}

	if ( uBytesRead < COPY_CHUNK_SIZE )
		return RES_FINISHED;

	return RES_OK;
}


OperationResult_e FileCopier_c::FinishFile ()
{
	if ( ! ( m_dwSourceAttrib & FILE_ATTRIBUTE_DIRECTORY ) )
	{
		BOOL bResult;
		TRY_FUNC ( bResult, SetFileTime ( m_hDest, NULL, NULL, &m_tModifyTime ), FALSE, m_sSource, m_sDest );

		if ( m_eMode == MODE_MOVE_VOLUMES )
		{
			CloseHandle ( m_hSource );
			m_hSource = NULL;

			DWORD dwAttribToSet = m_dwSourceAttrib & ~ ( FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM );
			TRY_FUNC ( bResult, SetFileAttributes ( m_sSource, dwAttribToSet ), FALSE, m_sSource, m_sDest );
			TRY_FUNC ( bResult, DeleteFile ( m_sSource ), FALSE, m_sSource, m_sDest );
		}
	}

	return RES_OK;
}


void FileCopier_c::MarkSourceOk ()
{
	if ( m_fnMarkCallback && m_tFileIterator.IsRootDir () )
		m_fnMarkCallback ( m_sSourceFileName, false );
}


Str_c FileCopier_c::GenerateDestName ( const Str_c & sSourceName ) const
{
	switch ( m_eDestType )
	{
	case DEST_INTO:
		return m_sDestDir + sSourceName;
	case DEST_OVER:
		return m_sDestDir;
	}
	return L"";
}