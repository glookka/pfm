#ifndef _fcommon_
#define _fcommon_

#include "LCore/cmain.h"

class FileProgress_c
{
public:
	enum
	{
		NUM_PROGRESS_POINTS = 100
	};

						FileProgress_c ();

	ULARGE_INTEGER		GetTotalSize () const;		// returns total copy size
	ULARGE_INTEGER		GetTotalProcessedSize () const;

	ULARGE_INTEGER		GetFileProcessedSize () const;
	ULARGE_INTEGER		GetFileTotalSize () const;

	int					GetFileProgress () const;
	int					GetTotalProgress () const;

	bool				GetFileProgressFlag () const;
	bool				GetTotalProgressFlag () const;
	bool				GetNameFlag () const;
	void				ResetChangeFlags ();

protected:
	bool				m_bFileProgressFlag;
	bool				m_bTotalProgressFlag;
	bool				m_bNameFlag;

	ULARGE_INTEGER		m_uSourceSize;
	ULARGE_INTEGER		m_uProcessedFile;
	ULARGE_INTEGER		m_uProcessedTotal;
	ULARGE_INTEGER		m_uTotalSize;

	void				Reset ();
	void				MoveToNext ( ULARGE_INTEGER & uDoneSize, ULARGE_INTEGER & uSourceSize );
	void				UpdateProgress ();

private:
	int					m_iFileProgress;
	int					m_iTotalProgress;
};

#endif