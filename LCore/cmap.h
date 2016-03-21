#ifndef _cmap_
#define _cmap_

#include "LCore/cmain.h"
#include "LCore/carray.h"

// lets pretend a sorted array is a map. search time is comparable anyway
// TODO: implement a real map
template < typename T1, typename T2 >
class Map_T
{
public:
	bool Add ( const T1 & Key, const T2 & Value )
	{
		MapEntry_t tEntry ( Key, Value );

		int iIndex = m_dArray.FindInSorted ( tEntry, CompareFunc );
		if ( iIndex != -1 )
			return false;

		Verify ( m_dArray.AddToSorted ( tEntry, CompareFunc ) );
		return true;
	}

	T2 * Find ( const T1 & Key )
	{
		MapEntry_t tEntry;
		tEntry.m_Key = Key;

		int iIndex = m_dArray.FindInSorted ( tEntry, CompareFunc );
		return iIndex == -1 ? NULL : & ( m_dArray [iIndex].m_Value );
	}

	bool Delete ( const T1 & Key )
	{
		MapEntry_t tEntry;
		tEntry.m_Key = Key;

		int iIndex = m_dArray.FindInSorted ( tEntry, CompareFunc );
		if ( iIndex != -1 )
		{
			m_dArray.Delete ( iIndex );
			return true;
		}

		return false;
	}

	T1 & Key ( int iIndex )
	{
		return m_dArray [iIndex].m_Key;
	}

	T2 & operator [] ( int iIndex )
	{
		return m_dArray [iIndex].m_Value;
	}

	void Resize ( int nItems )
	{
		m_dArray.Resize ( nItems );
	}

	int Length () const
	{
		return m_dArray.Length ();
	}

	void Clear ()
	{
		m_dArray.Clear ();
	}

private:
	struct MapEntry_t
	{
		MapEntry_t () {}
		MapEntry_t ( const T1 & Key, const T2 & Value )
			: m_Key 	( Key )
			, m_Value	( Value )
		{
		}

		T1	m_Key;
		T2	m_Value;
	};

	Array_T < MapEntry_t > m_dArray;

	static int CompareFunc ( const MapEntry_t & tEntry1, const MapEntry_t & tEntry2 )
	{
		return tEntry1.m_Key < tEntry2.m_Key ? -1 : ( tEntry1.m_Key == tEntry2.m_Key ? 0 : 1 );
	}
};

#endif