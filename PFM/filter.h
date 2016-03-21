#ifndef _filter_
#define _filter_

#include "pfm/main.h"
#include "pfm/std.h"

// supports range [a-b], set [abcd], any-symbol '?'
class BaseFilter_c
{
public:
					BaseFilter_c ( bool bCaseSensitive );

	virtual void	Set ( const wchar_t * szFilter );
	bool			Fits ( const wchar_t * szFile ) const;
	bool			HasMasks () const;

protected:
	bool			m_bCaseSensitive;

	virtual const wchar_t *	GetStringToCompare ( const wchar_t * szFile ) const = 0;
	virtual bool	FitsMask ( const wchar_t * szFile, const wchar_t * szMask ) const = 0;

	bool			CheckRangeSet ( const wchar_t * & szFileStart, const wchar_t * & szFilterStart ) const;

private:
	Array_T <Str_c> m_dMasks;
};


// extension filter
// doesnt support '*' for it is sloooow
class ExtFilter_c : public BaseFilter_c
{
public:
					ExtFilter_c ();

protected:
	virtual const wchar_t *	GetStringToCompare ( const wchar_t * szFile ) const;
	virtual bool	FitsMask ( const wchar_t * szExt, const wchar_t * szMask ) const;
};


// supports '*'
// slower than the extfilter
class Filter_c : public BaseFilter_c
{
public:
					Filter_c ( bool bCaseSensitive );
	virtual void	Set ( const wchar_t * szFilter );

protected:
	virtual const wchar_t *	GetStringToCompare ( const wchar_t * szFile ) const;
	virtual bool	FitsMask ( const wchar_t * szFile, const wchar_t * szMask ) const;

private:
	bool			CheckMask ( const wchar_t * szFile, const wchar_t * szMask, bool bAcceptEmpty ) const;
	const wchar_t *	TryToMatch ( const wchar_t * szPattern, int iPatternLen, const wchar_t * szFile ) const;
	bool			CheckAnySequence ( const wchar_t * szFileStart, const wchar_t * szFilterStart ) const;
};


#endif