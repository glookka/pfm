#ifndef _carray_
#define _carray_

#include "LCore/cmain.h"
#include "LCore/clog.h"

// todo: custom grow param? 
// todo: add sorting by operator <, not by func
template < typename T >
class Array_T
{
public:
	typedef int (*Compare_fn) ( const T &, const T & );

	Array_T () 
		: m_iDataLength	( 0 )
		, m_iLength		( 0 )
		, m_pData		( NULL )
	{
	}

	Array_T ( const Array_T & Value )
		: m_iDataLength	( 0 )
		, m_iLength		( 0 )
		, m_pData		( NULL )
	{
		operator = ( Value );
	}

	~Array_T () 
	{
		Reset ();
	}

	Array_T & operator = ( const Array_T & Value )
	{
		int l = Value.Length ();
		Grow ( l );
		for ( int i = 0; i < l; ++i )
			m_pData [ i ] = Value [ i ];

		return *this;
	}


	void Grow ( int nItems )
	{
		if ( nItems <= m_iDataLength )
			m_iLength = nItems;
		else
		{
			int iNewLen = Max ( m_iDataLength, 1 );
			while ( nItems > iNewLen )
				iNewLen *= 2;

			Resize ( iNewLen );
			m_iLength = nItems;
		}
	}

	void Clear ( int nItemsLeft = 0 )
	{
		Assert ( nItemsLeft >= 0 );
	
		if ( nItemsLeft < m_iLength )
			m_iLength = nItemsLeft;
	}

	void Reset ()
	{
		SafeDeleteArray ( m_pData );
		m_iDataLength = 0;
		m_iLength = 0;
	}

	void Resize ( int nItems )
	{
		if ( nItems <= m_iDataLength )
			return;

		T * pNewData = new T [nItems];
		if ( m_pData )
		{
			for ( int i = 0; i < m_iLength; i++ )
				pNewData [i] = m_pData [i];

			delete [] m_pData;
		}

		m_pData = pNewData;
		m_iDataLength = nItems;
	}

	T & operator [] ( int iIndex ) 
	{
		Assert ( iIndex >= 0 && iIndex < m_iLength );
		return m_pData [iIndex];
	}

	const T & operator [] ( int iIndex ) const
	{
		Assert ( iIndex >= 0 && iIndex < m_iLength );
		return m_pData [iIndex];
	}

	int Add ( const T & Value )
	{
		Grow ( m_iLength + 1 );
		m_pData [m_iLength-1] = Value;
		return m_iLength - 1;
	}

	void Delete ( int iIndex )
	{
		Assert ( iIndex >= 0 && iIndex < m_iLength );
		for ( int i = iIndex; i < m_iLength - 1; ++i )
			m_pData [i] = m_pData [i+1];

		--m_iLength;
	}

	int DeleteValue ( const T & Value )
	{
		int iId = FindIndex ( Value );

		if ( iId >= 0 )
			Delete ( iId );

		return iId;
	}

	int Length () const
	{
		return m_iLength;
	}

	int FindInSorted ( const T & tItem, Compare_fn comp )
	{
		int low = 0; 
		int high = m_iLength;

		while ( true )
		{
			int b = low + ( high - low ) / 2;

			if ( b == high )
				return ( b < m_iLength && ! comp ( tItem, m_pData [ b ] ) ) ? b : -1;

			int iCompRes = comp ( tItem, m_pData [ b ] );

			if ( iCompRes > 0 )
				low = b + 1;
			else
				if ( iCompRes < 0 )
					high = b;
				else
					high = low = b;
		}
	}

	bool AddToSorted ( const T & tItem, Compare_fn comp )
	{
		bool bNoDupes = true;
		int low = 0; 
		int high = m_iLength;

		while ( true )
		{
			int b = low + ( high - low ) / 2;

			if ( b == high )
			{
				Insert ( b, tItem );
				return bNoDupes;
			}

			int iCompRes = comp ( tItem, m_pData [ b ] );
			if ( iCompRes > 0 )
				low = b + 1;
			else
				if ( iCompRes < 0 )
					high = b;
				else
				{
					high = low = b;
					bNoDupes = false;
				}
		}
	}

	T & Last () const
	{
		Assert( m_iLength > 0 );
		return m_pData [m_iLength - 1];
	}

	T * Find ( const T & Value )
	{
		for ( int i = 0; i < m_iLength; ++i )
			if ( m_pData [i] == Value )
				return &m_pData [i];

		return NULL;
	}

	int FindIndex ( const T & Value ) const
	{
		for ( int i = 0; i < m_iLength; ++i )
			if ( m_pData [i] == Value )
				return i;

		return -1;
	}

	T & Pop ()
	{
		Assert ( m_iLength > 0 );
		--m_iLength;

		return m_pData [m_iLength];
	}

	bool Empty () const
	{
		return m_iLength == 0;
	}

	void Insert ( int iIndex, const T & Value )
	{
		Assert ( iIndex >= 0 && iIndex <= m_iLength );

		Grow ( m_iLength + 1 );

		for ( int i = m_iLength - 1; i > iIndex; --i )
			m_pData [i] = m_pData [i-1];

		m_pData [iIndex] = Value;
	}
	
protected:
	int			m_iDataLength;
	T *			m_pData;
	int			m_iLength;
};


template < typename T >
void Sort ( Array_T <T> & dData, int (*Comp) ( const T &, const T & ), int iStart = 0, int iEnd = -1 )
{
	int iLength = dData.Length ();
	if ( iLength <= 1 )
		return;

	if ( iEnd < 0 )
		iEnd = iEnd + iLength;

	if ( iStart < 0 || iEnd < 0 )
		return;

	if ( iEnd - iStart < 1 )
		return;

	int st0[128], st1[128];
	int a, b, k, i, j;
	T x;

	k = 1;
	st0[0] = iStart;
	st1[0] = iEnd;
	while (k != 0)
	{
		k--;
		i = a = st0[k];
		j = b = st1[k];
		x = dData[(a + b) / 2];
		while (a < b)
		{
			while (i <= j)
			{
				while ( Comp ( dData[i], x) < 0 ) i++;
				while ( Comp ( x, dData[j]) < 0 ) j--;
				if (i <= j)
				{
					Swap(dData[i], dData[j]);
					i++;
					j--;
				}
			}
			if (j-a >= b-i)
			{
				if (a < j)
				{
					st0[k] = a;
					st1[k] = j;
					k++;
				}
				a = i;
			} else
			{
				if (i < b)
				{
					st0[k] = i;
					st1[k] = b;
					k++;
				}
				b = j;
			}
		}
	}
}

#endif