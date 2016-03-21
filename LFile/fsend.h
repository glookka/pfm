#ifndef _fsend_
#define _fsend_

#include "pfm/main.h"
#include "LComm/cirda.h"
#include "pfm/progress.h"
#include "pfm/dialogs/errors.h"

///////////////////////////////////////////////////////////////////////////////////////////
// common sender
class FileSend_c : public FileProgress_c
{
public:
	enum Error_e
	{
		 ERR_OK
		,ERR_SEND
		,ERR_READ
	};

						// the obex client should be already connected
						FileSend_c ( unsigned int uMaxChunk, SelectedFileList_t & tList, SlowOperationCallback_t fnPrepareCallback );

	void				SetWindow ( HWND hWnd );
	const wchar_t *		GetSourceFileName () const;
	Error_e				GetError () const;

	bool				StartFile ();
	bool 				OnPut ( int iResponse );

	virtual bool 		SendFileName () = 0;
	virtual bool 		SendChunk ( unsigned char * pBuffer, unsigned int uBytes, bool bEOF ) = 0;
	virtual void 		SendAbort () = 0;

	void				Cancel ();

protected:
	enum State_e
	{
		 STATE_SEND_HEADER
		,STATE_SEND_BODY
		,STATE_ERROR
	};

	Error_e					m_eError;
	State_e					m_eState;
	DWORD					m_uMaxChunk;
	DWORD					m_uBytesRead;
	SelectedFileList_t *	m_pList;
	FileIteratorPanel_c		m_tFileIterator;

	unsigned char *		m_pBuffer;

	Str_c				m_sSourceFileName;
	HANDLE				m_hSource;

	HWND				m_hWnd;

	virtual int			GetRspOk () const = 0;
	virtual int			GetRspContinue () const = 0;

	void				Reset ();
	void				Cleanup ();
	bool				GenerateNextFileName ();

	OperationResult_e	PrepareFile ();
	bool				SendNextChunk ();
};

///////////////////////////////////////////////////////////////////////////////////////////
class FileSendIrda_c : public FileSend_c
{
public:
						FileSendIrda_c ( IrdaObexClient_c * pOBEX, unsigned int uMaxChunk, SelectedFileList_t & tList, SlowOperationCallback_t fnPrepareCallback );

protected:
	virtual bool 		SendFileName ();
	virtual bool 		SendChunk ( unsigned char * pBuffer, unsigned int uBytes, bool bEOF );
	virtual void 		SendAbort ();

	virtual int			GetRspOk () const;
	virtual int			GetRspContinue () const;

private:
	IrdaObexClient_c * m_pOBEX;
};

///////////////////////////////////////////////////////////////////////////////////////////
class FileSendWidcomm_c : public FileSend_c
{
public:
						FileSendWidcomm_c ( unsigned int uMaxChunk, SelectedFileList_t & tList, SlowOperationCallback_t fnPrepareCallback );

protected:
	virtual bool 		SendFileName ();
	virtual bool 		SendChunk ( unsigned char * pBuffer, unsigned int uBytes, bool bEOF );
	virtual void 		SendAbort ();

	virtual int			GetRspOk () const;
	virtual int			GetRspContinue () const;
};

///////////////////////////////////////////////////////////////////////////////////////////
class MsBTObexClient_c;

class FileSendMsBT_c : public FileSend_c
{
public:
						FileSendMsBT_c ( MsBTObexClient_c * pObex, unsigned int uMaxChunk, SelectedFileList_t & tList, SlowOperationCallback_t fnPrepareCallback );

protected:
	virtual bool 		SendFileName ();
	virtual bool 		SendChunk ( unsigned char * pBuffer, unsigned int uBytes, bool bEOF );
	virtual void 		SendAbort ();

	virtual int			GetRspOk () const;
	virtual int			GetRspContinue () const;

private:
	MsBTObexClient_c *	m_pOBEX;
};

#endif
