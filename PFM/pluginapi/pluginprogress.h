#ifndef _pluginprogress_
#define _pluginprogress_

#include "pluginstd.h"

enum
{
	 FILESIZE_BUFFER_SIZE	= 64
	,PROGRESS_BUFFER_SIZE	= 128
};

class FileProgress_c
{
public:
	enum
	{
		NUM_PROGRESS_POINTS = 100
	};

						FileProgress_c ()				{ Reset ();	}

	ULARGE_INTEGER		GetTotalSize () const			{ return m_uTotalSize; }
	ULARGE_INTEGER		GetTotalProcessedSize () const	{ return m_uProcessedTotal; }

	ULARGE_INTEGER		GetFileProcessedSize () const	{ return m_uProcessedFile; }
	ULARGE_INTEGER		GetFileTotalSize () const		{ return m_uSourceSize; }

	int					GetFileProgress () const		{ return m_iFileProgress; }
	int					GetTotalProgress () const		{ return m_iTotalProgress; }

	bool				GetNameFlag () const			{ return m_bNameFlag; }
	void				ResetChangeFlags ()				{ m_bNameFlag = false; }

protected:
	bool				m_bNameFlag;

	ULARGE_INTEGER		m_uSourceSize;
	ULARGE_INTEGER		m_uProcessedFile;
	ULARGE_INTEGER		m_uProcessedTotal;
	ULARGE_INTEGER		m_uTotalSize;

	void Reset ()
	{
		m_iFileProgress				= 0;
		m_iTotalProgress			= 0;
		m_uSourceSize.QuadPart		= 0;
		m_uProcessedFile.QuadPart	= 0;
		m_uProcessedTotal.QuadPart	= 0;
		m_uTotalSize.QuadPart 		= 0;

		m_bNameFlag				= true;
	}

	void MoveToNext ( ULARGE_INTEGER & uDoneSize, ULARGE_INTEGER & uSourceSize )
	{
		m_uProcessedTotal.QuadPart -= m_uProcessedFile.QuadPart;
		m_uProcessedTotal.QuadPart += m_uSourceSize.QuadPart;
		m_uProcessedFile.QuadPart = 0;
		m_uSourceSize.QuadPart = 0;
	}

	void UpdateProgress ()
	{
		m_iFileProgress = int ( double ( LONGLONG ( m_uProcessedFile.QuadPart ) ) / double ( LONGLONG ( m_uSourceSize.QuadPart ) ) * NUM_PROGRESS_POINTS );
		m_iTotalProgress = int ( double ( LONGLONG ( m_uProcessedTotal.QuadPart ) ) / double ( LONGLONG ( m_uTotalSize.QuadPart ) ) * NUM_PROGRESS_POINTS );
	}

private:
	int					m_iFileProgress;
	int					m_iTotalProgress;
};


class WindowProgress_c
{
public:
				WindowProgress_c ( const wchar_t * szSizeText );
			
	void 		Init ( HWND hDlg, const wchar_t * szHeader, const wchar_t * szProgressText, FileProgress_c * pProgress  );
	void		SetItems ( HWND hHeader, HWND hTotal, HWND hFile, HWND hSourceTxt, HWND hDestTxt, HWND hTotalPerc, HWND hFilePerc, HWND hTotalSize, HWND hFileSize );
	bool  		UpdateProgress ( FileProgress_c * pProgress );
	void 		Setup ( FileProgress_c * pProgress );

protected:
	HWND 	m_hSourceText;
	HWND 	m_hDestText;

private:
	HWND	m_hHeader;
	HWND 	m_hTotalProgressBar;
	HWND 	m_hFileProgressBar;
	HWND 	m_hTotalProgressText;
	HWND 	m_hFileProgressText;
	HWND 	m_hTotalText;
	HWND 	m_hFileText;

	double	m_fLastTime;
	wchar_t	m_szTotalSize [FILESIZE_BUFFER_SIZE];
	wchar_t	m_szProgressText [PROGRESS_BUFFER_SIZE];
	const wchar_t * m_szSizeText;
};

#endif
