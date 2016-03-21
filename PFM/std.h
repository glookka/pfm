#ifndef _std_
#define _std_

#include "main.h"

//////////////////////////////////////////////////////////////////////////////////////////
// common stuff
template < typename T >
inline T Clamp ( T value, T minValue, T maxValue )
{
	if ( value < minValue )
		return minValue;

	if ( value > maxValue )
		return maxValue;

	return value;
}


template < typename T, typename U >
inline void Swap ( T& v1, U& v2 )
{
	T temp;
	temp = v1;
	v1 = T (v2);
	v2 = U (temp);
}


template < typename T >
inline bool Odd ( T value )
{
	return value & 1 ? true : false;
}


template < typename T1, typename T2 >
inline T2 Round ( T1 value )
{
	return value > T1 ( 0.0 ) ? T2 ( value + static_cast < T1 >( 0.5 ) ) : T2 ( value - static_cast < T1 > ( 0.5 ) );
}


template < typename T >
inline T Sqr ( T value )
{
	return value * value;
}


template < typename T1, typename T2 >
inline T1 Min ( const T1 & v1, const T2 & v2 )
{
	return ( v1 < v2 ) ? v1 : v2;
}


template < typename T1, typename T2, typename T3 >
inline T1 Min ( const T1 & v1, const T2 & v2, const T3 & v3 )
{
	return Min ( Min ( v1, v2 ), v3 );
}


template < typename T1, typename T2, typename T3, typename T4 >
inline T1 Min ( const T1 & v1, const T2 & v2, const T3 & v3, const T4 & v4 )
{
	return Min ( Min ( v1, v2 ), Min ( v3, v4 ) );
}


template < typename T1, typename T2, typename T3, typename T4, typename T5 >
inline T1 Min ( const T1 & v1, const T2 & v2, const T3 & v3, const T4 & v4, const T5 & v5 )
{
	return Min ( Min ( v1, v2 ), Min ( v3, v4 ), v5 );
}


template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 >
inline T2 Min ( const T1 & v1, const T2 & v2, const T3 & v3, const T4 & v4, const T5 & v5, const T6 & v6 )
{
	return Min ( Min ( v1, v2 ), Min ( v3, v4 ), Min ( v5, v6 ) );
}


template < typename T1, typename T2 >
inline T1 Max ( const T1 & v1, const T2 & v2 )
{
	return ( v1 > v2 ) ? v1 : v2;
}


template < typename T1, typename T2, typename T3 >
inline T1 Max ( const T1 & v1, const T2 & v2, const T3 & v3 )
{
	return Max ( Max (v1, v2), v3 );
}


template < typename T1, typename T2, typename T3, typename T4 >
inline T1 Max ( const T1 & v1, const T2 & v2, const T3 & v3, const T4 & v4 )
{
	return Max ( Max ( v1, v2 ), Max ( v3, v4 ) );
}


template < typename T1, typename T2, typename T3, typename T4, typename T5 >
inline T1 Max ( const T1 & v1, const T2 & v2, const T3 & v3, const T4 & v4, const T5 & v5 )
{/
return Max ( Max ( v1, v2 ), Max ( v3, v4 ), v5 );
}


template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 >
inline T1 Max ( const T1 & v1, const T2 & v2, const T3 & v3, const T4 & v4, const T5 & v5, const T6 & v6 )
{
	return Max ( Max ( v1, v2 ), Max ( v3, v4 ), Max ( v5, v6 ) );
}


inline bool IsInRect ( int iX, int iY, const RECT & tRect )
{
	return iX >= tRect.left && iX <= tRect.right && iY >= tRect.top && iY <= tRect.bottom;
}


inline void CollapseRect ( RECT & tRect, int iCollapse )
{
	tRect.top += iCollapse;
	tRect.bottom -= iCollapse;
	tRect.left += iCollapse;
	tRect.right -= iCollapse;
}


//////////////////////////////////////////////////////////////////////////////////////////
// dynamic wchar_t string
class StringDynamic_c
{
public:
						StringDynamic_c ();
						StringDynamic_c ( const StringDynamic_c & sStr );
						StringDynamic_c ( const wchar_t * szTxt );
						StringDynamic_c ( float fValue, const wchar_t * szFormat = NULL );
						StringDynamic_c ( int iValue );
						StringDynamic_c ( wchar_t cChar );
						~StringDynamic_c ();

	const wchar_t *		c_str () const;

	wchar_t &			operator [] ( int iIndex );
	StringDynamic_c &	operator = ( const wchar_t * szTxt );
	StringDynamic_c &	operator = ( const StringDynamic_c & sStr );
	bool				operator < ( const StringDynamic_c & sStr ) const;
	StringDynamic_c &	operator += ( const wchar_t * szTxt );
	StringDynamic_c		operator + ( const wchar_t * szTxt ) const;
	bool				operator == ( const wchar_t * szTxt ) const;
	bool				operator != ( const wchar_t * szTxt ) const;
						operator const wchar_t * () const;

	int					Length () const;
	bool				Empty () const;
	StringDynamic_c		SubStr ( int iStart ) const;
	StringDynamic_c		SubStr ( int iStart, int iLength ) const;
	StringDynamic_c &	ToUpper ();
	StringDynamic_c &	ToLower();

	int					Find ( const wchar_t * szPattern, int iOffset = -1 ) const;
	int					Find ( wchar_t cPattern, int iOffset = -1 ) const;
	int					RFind ( const wchar_t * szPattern, int iOffset = -1 ) const;
	int					RFind ( wchar_t cPattern, int iOffset = -1 ) const;
	bool				Begins ( wchar_t cPrefix ) const;
	bool				Ends ( wchar_t cSuffix ) const;
	bool				Begins ( const wchar_t * szPrefix ) const;
	bool				Ends ( const wchar_t * szSuffix ) const;

	void				LTrim ();
	void				RTrim ();
	void				Trim ();
	void				Insert ( const wchar_t * szText, int iPos );
	void				Erase ( int iPos, int iLen );
	void				Replace ( wchar_t cFind, wchar_t cReplace );
	StringDynamic_c &	Chop ( int iCount );
	void				Strip ( wchar_t cChar );

private:
	static const int	MAX_FLOAT_LENGTH = 32;
	static const int	MAX_INT_LENGTH = 32;

	wchar_t *			m_szData;
	int					m_iDataSize;

	static wchar_t		m_cEmpty;

	void				Grow ( int nChars, bool bDiscard = true );
};

typedef StringDynamic_c Str_c;

// string construction helper
Str_c NewString ( const wchar_t * szFormat, ...  );

// unicode to ansi conversion
void UnicodeToAnsi ( const wchar_t * szStr, char * szResult );

// ansi to unicode conversion
void AnsiToUnicode ( const char * szStr, wchar_t * szResult );

// string guid to GUID
GUID StrToGUID ( const wchar_t * szGUID );


//////////////////////////////////////////////////////////////////////////////////////////
// dynamic array

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

	void Grow ( int nItems )
	{
		if ( nItems <= m_iDataLength )
			m_iLength = nItems;
		else
		{
			int iNewLen = Max ( m_iDataLength, 1 );
			while ( nItems > iNewLen )
				iNewLen *= 2;

			T * pNewData = new T [iNewLen];
			if ( m_pData )
			{
				for ( int i = 0; i < m_iLength; i++ )
					pNewData [i] = m_pData [i];

				delete [] m_pData;
			}

			m_pData = pNewData;
			m_iDataLength = iNewLen;
			m_iLength = nItems;
		}
	}

	void SetLength ( int nItems )
	{
		Reserve ( nItems );
		m_iDataLength = nItems;
		m_iLength = nItems;
	}

	void Reserve ( int nItems )
	{
		if ( nItems <= m_iDataLength )
			return;

		delete [] m_pData;
		m_pData = new T [nItems];
		m_iDataLength = nItems;
		m_iLength = 0;
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

	T * Data () const
	{
		return m_pData;
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
				return ( b < m_iLength && comp ( tItem, m_pData [ b ] ) == 0 ) ? b : -1;

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
void Sort ( Array_T <T> & dData, int (*Comp) ( const void *, const void * ), int iStartItem = 0, int iEndItem = 0 )
{
	if ( dData.Length () <= iStartItem )
		return;

	if ( iEndItem == 0 )
		iEndItem = dData.Length ();

	qsort ( &(dData [iStartItem]), iEndItem - iStartItem, sizeof ( dData [0] ), Comp );
}


//////////////////////////////////////////////////////////////////////////////////////////
// array-based map
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

//////////////////////////////////////////////////////////////////////////////////////////
// string helpers

// extension
Str_c GetExt ( const wchar_t * szFileName );

// path (w/o filename)
Str_c GetPath ( const wchar_t * szFileName );

// filename w/o path
Str_c GetName ( const wchar_t * szPath );

// fast name/ext getter. unsafe.
const wchar_t * GetNameExtFast ( const wchar_t * szNameExt, wchar_t * szName = NULL );

// split path into path, file name and extension
void SplitPath ( const Str_c & sFileName, Str_c & sDir, Str_c & sName, Str_c & sExt );

// the same path splitter, wchar_t version. some params can be null
void SplitPathW ( const wchar_t * szFileName, wchar_t * szDir, wchar_t * szName, wchar_t * szExt );

// append a slash
Str_c & AppendSlash ( Str_c & sStr );

// prepend a slash
Str_c & PrependSlash ( Str_c & sStr );

// remove a slash
Str_c & RemoveSlash ( Str_c & sStr );

// ends in slash?
bool EndsInSlash ( const Str_c & sStr );

// cut string to first slash
void CutToSlash ( Str_c & sStr );

// remove trailing and heading slashes
Str_c RemoveSlashes ( const Str_c & sStr );

// replace all backslashes with slashes
void ReplaceSlashes ( Str_c & sPath );

#endif