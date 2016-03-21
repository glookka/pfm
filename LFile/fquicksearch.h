#ifndef _fquicksearch_
#define _fquicksearch_

#include "pfm/main.h"

const int READ_BUFFER_SIZE = 32768;

class StringSearch_c
{
public:
					StringSearch_c ( const wchar_t * szPattern, bool bMatchCase, bool bWholeWord );
					~StringSearch_c ();

	bool 			StartFile ( const wchar_t * szFile );
	bool 			DoSearch ();
	bool 			IsFileFinished () const;

protected:
	bool			m_bMatchCase;
	bool			m_bWholeWord;
	int				m_iPatLen;
	int 			m_iMin;
	int				m_iMax;
	FILE *			m_pFile;
	bool			m_bEndReached;
	int				m_iNextOffset;
	unsigned short*	m_dBadChar;
	wchar_t *		m_szPattern;
	wchar_t			m_dBuffer [READ_BUFFER_SIZE];

	bool 			Test ( wchar_t * pBuffer, int nSymbolsToTest, int & iNextOffset );
	void			EndOfFile ();
	bool 			CheckWholeWord ( wchar_t * pBuffer, int iIndex, int nSymbolsToTest ) const;
};

#endif