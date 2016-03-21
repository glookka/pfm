#ifndef _ffilter_
#define _ffilter_

#include "LCore/cmain.h"
#include "LCore/cstr.h"
#include "LCore/carray.h"

// supports range [a-b], set [abcd], negation -[a-b], any-symbol '?'
class BaseFilter_c
{
public:
	virtual void	Set ( const wchar_t * szFilter );
	bool			Fits ( const wchar_t * szFile ) const;

protected:
	virtual const wchar_t *	GetStringToCompare ( const wchar_t * szFile ) const = 0;
	virtual bool	FitsMask ( const wchar_t * szFile, const wchar_t * szMask ) const = 0;

	bool			CheckRangeSet ( const wchar_t * & szFileStart, const wchar_t * & szFilterStart ) const;

private:
	struct Mask_t
	{
		bool			m_bInvert;
		Str_c			m_sMask;
	};

	Array_T <Mask_t> m_dMasks;
};


// extension filter
// doesnt support '*' for it is sloooow
class ExtFilter_c : public BaseFilter_c
{
protected:
	virtual const wchar_t *	GetStringToCompare ( const wchar_t * szFile ) const;
	virtual bool	FitsMask ( const wchar_t * szExt, const wchar_t * szMask ) const;
};


// supports '*'
// slower than the extfilter
class Filter_c : public BaseFilter_c
{
public:
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