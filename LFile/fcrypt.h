#ifndef _fcrypt_
#define _fcrypt_

#include "LCore/cmain.h"
#include "LFile/fmisc.h"
#include "LFile/fcommon.h"
#include "LFile/ferrors.h"
#include "LCrypt/cproviders.h"

class Encrypter_c : public FileProgress_c
{
public:
	enum ErrorCode_e
	{
		 ERROR_NONE
		,ERROR_COMMON
		,ERROR_NOT_ENCRYPTED
		,ERROR_CORRUPT_HEADER
		,ERROR_DECRYPT
		,ERROR_INVALID_PWD
		,ERROR_TOTAL
	};


						Encrypter_c ( FileList_t & tList, bool bEncrypt, bool bDelete, const crypt::AlgInfo_t & tAlg , const Str_c & sKey,
									SlowOperationCallback_t fnPrepareCallback, MarkCallback_t fnMarkCallback );
						~Encrypter_c ();

	bool				IsInDialog () const;
	bool				DoWork ();
	void				Cancel ();

	void				SetWindow ( HWND hWnd );

	int					GetNumFiles ();
	int					GetNumProcessed ();

	int					GetNumResults () const;
	ErrorCode_e			GetResult ( int iId ) const;
	const wchar_t *		GetResultString ( int iId ) const;
	const Str_c	GetResultDir ( int iId  ) const;
	const WIN32_FIND_DATA & GetResultData ( int iId ) const;

	const wchar_t *		GetSourceFileName () const;

private:
	enum CryptState_e
	{
		 STATE_INITIAL
		,STATE_PREPARE_SOURCE
		,STATE_PREPARE_DEST
		,STATE_CRYPT_FIRST
		,STATE_ENCRYPT
		,STATE_CANCELLED
	};

	enum
	{
		 PLAIN_CHUNK_SIZE	= 32768
		,ENC_CHUNK_SIZE		= 32768
		,MAX_KEY_SIZE		= 64
		,MAX_BLOCK_LENGTH	= 32
	};

	CryptState_e		m_eState;

	unsigned char *		m_pPlainBuffer;
	unsigned char *		m_pEncBuffer;
	bool 				m_bEncrypt;
	bool				m_bDelete;

	const crypt::AlgInfo_t * m_pAlg;

	FileList_t *		m_pList;
	FileIteratorPanelCached_c m_tFileIterator;

	struct Error_t
	{
		ErrorCode_e	m_eError;
		int			m_iIndex;
	};

	ErrorCode_e			m_eErrorCode;
	Array_T <Error_t> m_dErrors;

	MarkCallback_t		m_fnMarkCallback;

	HWND				m_hWnd;

	Str_c				m_sKey;
	unsigned char		m_dKey [MAX_KEY_SIZE];

	Str_c				m_sSource;
	Str_c				m_sDest;
	Str_c				m_sSourceName;			// name w/o ext
	Str_c				m_sSourceFileName;		// name w/o path
	Str_c				m_sDir;
	Str_c				m_sExt;

	int					m_nFiles;
	int					m_nProcessed;

	HANDLE				m_hSource;
	HANDLE				m_hDest;
	DWORD				m_dwSourceAttrib;

	symmetric_CTR		m_CTR;
	unsigned char		m_IV [MAX_BLOCK_LENGTH];

	void				PrepareKey ();
	bool				PrepareCryptLib (); 
	void				PrepareIV ();

	bool 				GenerateNextFileName ();
	Str_c				GetDecryptedFileName () const;
	OperationResult_e	PrepareSource ();
	OperationResult_e	PrepareDest ();
	void				MoveToNextFile ();

	OperationResult_e	EncryptFirstChunk ();
	OperationResult_e 	DecryptFirstChunk ();
	OperationResult_e	EncryptNextChunk ();

	OperationResult_e	FinishFile ();
	void 				Cleanup ( bool bError );
	void 				MarkSourceOk ( const Str_c & sName );

	void				RememberError ( OperationResult_e eResult );
};

#endif
