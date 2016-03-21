#include "pch.h"

#include "LFile/fcommon.h"

FileProgress_c::FileProgress_c ()
{
	Reset ();
}

ULARGE_INTEGER FileProgress_c::GetTotalSize () const
{
	return m_uTotalSize;
}


ULARGE_INTEGER FileProgress_c::GetTotalProcessedSize () const
{
	return m_uProcessedTotal;
}

ULARGE_INTEGER FileProgress_c::GetFileProcessedSize () const
{
	return m_uProcessedFile;
}

ULARGE_INTEGER FileProgress_c::GetFileTotalSize () const
{
	return m_uSourceSize;
}

int	FileProgress_c::GetFileProgress () const
{
	return m_iFileProgress;
}


int FileProgress_c::GetTotalProgress () const
{
	return m_iTotalProgress;
}


bool FileProgress_c::GetFileProgressFlag () const
{
	return m_bFileProgressFlag;
}


bool FileProgress_c::GetTotalProgressFlag () const
{
	return m_bTotalProgressFlag;
}


bool FileProgress_c::GetNameFlag () const
{
	return m_bNameFlag;
}


void FileProgress_c::ResetChangeFlags ()
{
	m_bFileProgressFlag = false;
	m_bTotalProgressFlag = false;
	m_bNameFlag = false;
}


void FileProgress_c::Reset ()
{
	m_iFileProgress			= 0;
	m_iTotalProgress		= 0;
	m_uSourceSize.QuadPart	= 0;
	m_uProcessedFile.QuadPart	= 0;
	m_uProcessedTotal.QuadPart	= 0;
	m_uTotalSize.QuadPart 		= 0;

	m_bFileProgressFlag		= true;
	m_bTotalProgressFlag	= true;
	m_bNameFlag				= true;
}


void FileProgress_c::MoveToNext ( ULARGE_INTEGER & uDoneSize, ULARGE_INTEGER & uSourceSize )
{
	m_uProcessedTotal.QuadPart -= m_uProcessedFile.QuadPart;
	m_uProcessedTotal.QuadPart += m_uSourceSize.QuadPart;
	m_uProcessedFile.QuadPart = 0;
	m_uSourceSize.QuadPart = 0;

	UpdateProgress ();
}

void FileProgress_c::UpdateProgress ()
{
	int iFileProgress = int ( double ( LONGLONG ( m_uProcessedFile.QuadPart ) ) / double ( LONGLONG ( m_uSourceSize.QuadPart ) ) * NUM_PROGRESS_POINTS );
	int iTotalProgress = int ( double ( LONGLONG ( m_uProcessedTotal.QuadPart ) ) / double ( LONGLONG ( m_uTotalSize.QuadPart ) ) * NUM_PROGRESS_POINTS );

	if ( iFileProgress != m_iFileProgress )
		m_bFileProgressFlag = true;

	if ( iTotalProgress != m_iTotalProgress )
		m_bTotalProgressFlag = true;

	m_iFileProgress = iFileProgress;
	m_iTotalProgress = iTotalProgress;
}