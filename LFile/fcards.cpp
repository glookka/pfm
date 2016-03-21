#include "pch.h"

#include "LFile/fcards.h"
#include "LCore/carray.h"

#include "projects.h"

static Array_T <WIN32_FIND_DATA> g_dCards;

void RefreshStorageCards ()
{
	g_dCards.Clear ();

	WIN32_FIND_DATA tData;
	HANDLE hCard = FindFirstFlashCard ( &tData );

	if ( hCard != INVALID_HANDLE_VALUE )
	{
		do 
		{
			if ( tData.cFileName [0] != L'\0' )
				g_dCards.Add ( tData );
		}
		while ( FindNextFlashCard ( hCard, &tData ) );
	}
}

int GetNumStorageCards ()
{
	return g_dCards.Length ();
}

void GetStorageCardData ( int iCard, WIN32_FIND_DATA & tData )
{
	tData = g_dCards [iCard];
}
