#ifndef _pvar_loader_
#define _pvar_loader_

#include "LCore/cmain.h"
#include "LCore/cstr.h"
#include "LCore/carray.h"
#include "LCore/cmap.h"

// a base load-save var
class SavedVar_c
{
public:
					SavedVar_c ( const wchar_t * szName );

	virtual bool	LoadVar ();
	virtual void	SaveVar ( FILE * pFile ) = 0;

	const Str_c &	GetName () const;

protected:
	Str_c	m_sName;
};


// comment
class SavedVar_Comment_c : public SavedVar_c
{
public:
					SavedVar_Comment_c ( const wchar_t * szComment );

	virtual void	SaveVar ( FILE * pFile );
};

// int type
class SavedVar_Int_c : public SavedVar_c
{
public:
					SavedVar_Int_c ( const wchar_t * szName, int * pField );

	virtual bool	LoadVar ();
	virtual void	SaveVar ( FILE * pFile );

	int				GetValue () const;
	void			SetValue ( int iValue ) const;

private:
	int *			m_pField;
};


// float type
class SavedVar_Float_c : public SavedVar_c
{
public:
					SavedVar_Float_c ( const wchar_t * szName, float * pField );

	virtual bool	LoadVar ();
	virtual void	SaveVar ( FILE * pFile );

	float			GetValue () const;

protected:
	float *			m_pField;
};


// string type
class SavedVar_String_c : public SavedVar_c
{
public:
					SavedVar_String_c ( const wchar_t * szName, Str_c * pField );

	virtual bool	LoadVar ();
	virtual void	SaveVar ( FILE * pFile );

	const Str_c & GetValue () const;

protected:
	Str_c *	m_pField;
};


// bool type
class SavedVar_Bool_c : public SavedVar_c
{
public:
					SavedVar_Bool_c ( const wchar_t * szName, bool * pField );

	virtual bool	LoadVar ();
	virtual void	SaveVar ( FILE * pFile );

	bool			 GetValue () const;

protected:
	bool *			m_pField;
};


// dword type
class SavedVar_Dword_c : public SavedVar_c
{
public:
					SavedVar_Dword_c ( const wchar_t * szName, DWORD * pField );

	virtual bool	LoadVar ();
	virtual void	SaveVar ( FILE * pFile );

	DWORD			GetValue () const;

protected:
	DWORD *			m_pField;
};


// variable loader-saver
class VarLoader_c
{
public:
					~VarLoader_c ();

	void			AddComment ( wchar_t * szComment );

	void			RegisterVar ( wchar_t * szName, int *			pField );
	void			RegisterVar ( wchar_t * szName, float *			pField );
	void			RegisterVar ( wchar_t * szName, Str_c * 	pField );
	void			RegisterVar ( wchar_t * szName, bool *			pField );
	void			RegisterVar ( wchar_t * szName, DWORD *			pField );

	bool			LoadVars ( const wchar_t * szFileName );
	void			SaveVars ();

	void			ResetRegistered ();

private:
	typedef Map_T < Str_c, SavedVar_c * >	VarMap_t;
	typedef Array_T < SavedVar_c * >		VarVec_t;

	Str_c			m_sFileName;
	VarVec_t		m_dVars;
	VarMap_t		m_tVarMap;

	void			AddVar ( const wchar_t * szName, SavedVar_c * pVar );
};


#endif