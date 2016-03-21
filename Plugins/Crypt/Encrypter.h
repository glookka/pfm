#ifndef _crypt_encrypter_
#define _crypt_encrypter_

#include "resource.h"
#include "../../pfm/pluginapi/pluginstd.h"
#include "../../pfm/pluginapi/pluginprogress.h"
#include "Crypt.h"

class Encrypter_c : public FileProgress_c
{ 
public:
						Encrypter_c ( const wchar_t * szRoot, PanelItem_t ** dItems, int nItems, bool bEncrypt, bool bDelete, const AlgInfo_t & tAlg, const wchar_t * szPassword, SlowOperationCallback_t SlowOp, MarkCallback_t MarkFile );
						~Encrypter_c ();

	bool				IsInDialog () const;
	bool				DoWork ();
	void				Cancel ();

	void				SetWindow ( HWND hWnd )		{ m_hWnd = hWnd; }

	bool				IsEncryption () const		{ return m_bEncrypt; }

	int					GetNumTotal () const		{ return m_nTotalFiles; }
	int					GetNumProcessed () const	{ return m_nProcessedFiles; }
	int					GetNumEncrypted () const	{ return m_nEncryptedFiles; }

	int					GetNumSkipped () const		{ return m_nSkippedFiles; }
	int					GetNumErrors () const		{ return m_nErrorFiles; }

	const wchar_t *		GetSourceFileName () const	{ return m_szSource; }

private:
	enum CryptState_e
	{
		 STATE_PREPARE
		,STATE_ENCRYPT
		,STATE_CANCELLED
	};

	enum
	{
		 PLAIN_CHUNK_SIZE	= 65536
		,ENC_CHUNK_SIZE		= 65536
		,MAX_KEY_SIZE		= 64
		,MAX_BLOCK_LENGTH	= 32
	};

	CryptState_e		m_eState;

	unsigned char *		m_pPlainBuffer;
	unsigned char *		m_pEncBuffer;
	bool 				m_bEncrypt;
	bool				m_bDelete;
	bool				m_bDestExists;

	PanelColumn_t **	m_pItems;
	int					m_nItems;
	const AlgInfo_t *	m_pAlg;

	HANDLE				m_hIterator;

	MarkCallback_t		m_fnMarkCallback;

	HWND				m_hWnd;

	wchar_t 			m_szKey [MAX_PWD_CHARS+2];
	unsigned char		m_dKey [MAX_KEY_SIZE];

	wchar_t 			m_szRootDir [MAX_PATH];
	wchar_t 			m_szSource [MAX_PATH];
	wchar_t 			m_szDest [MAX_PATH];
	wchar_t 			m_szTempDest [MAX_PATH];
	wchar_t 			m_szSourceName [MAX_PATH];		// name w/o ext
	wchar_t 			m_szSourceFileName [MAX_PATH];	// name w/o path
	wchar_t 			m_szDir [MAX_PATH];
	wchar_t 			m_szExt [MAX_PATH];

	wchar_t 			m_szDecryptedFilename [MAX_PATH];
	wchar_t 			m_szResultDir [MAX_PATH];
	DWORD				m_dwSourceAttrib;
	DWORD				m_uSignature;

	int					m_nProcessedFiles;
	int					m_nTotalFiles;
	int					m_nEncryptedFiles;
	int					m_nSkippedFiles;
	int					m_nErrorFiles;

	HANDLE				m_hSource;
	HANDLE				m_hDest;

	symmetric_CTR		m_CTR;
	unsigned char		m_IV [MAX_BLOCK_LENGTH];

	void				PrepareKey ();
	bool				PrepareCryptLib (); 
	void				PrepareIV ();

	bool 				GenerateNextFileName ();
	const wchar_t *		GetDecryptedFileName ();
	OperationResult_e	PrepareSource ();
	OperationResult_e	PrepareDest ();
	void				MoveToNextFile ( OperationResult_e eResult );

	OperationResult_e	EncryptFirstChunk ();
	OperationResult_e 	DecryptFirstChunk ();
	OperationResult_e	EncryptDecryptNextChunk ();

	OperationResult_e	FinishFile ();
	void 				Cleanup ();
	void 				MarkSourceOk ( const wchar_t * szName );
};

#endif
