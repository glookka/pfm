#ifndef _main_
#define _main_

#pragma warning( disable : 4291 )		// we don't use exceptions, so who cares
#pragma warning( disable : 4127 )		// we redefine for
#pragma warning( disable : 4100 )		// stupid one about "unref formal parameter"

#define SafeDelete(p) { if(p) { delete (p); (p)=NULL; } }
#define SafeDeleteArray(p) { if(p) { delete[] (p); (p)=NULL; } }
#define GET_X_LPARAM(lParam)	((int)(short)LOWORD(lParam))
#define GET_Y_LPARAM(lParam)	((int)(short)HIWORD(lParam))

#include "pch.h"

// the proper for
#undef for
#define for if (0); else for

#include "defines.h"

//////////////////////////////////////////////////////////////////////////////////////////
// memory stuff
#if FM_MEMORY_TRACK

// shutdown
void Shutdown_Memory ();

// allocate managed memory
void * MemoryAllocate ( size_t iSize, char const * szFile, int iLine, bool bArray );

// free managed memory
void MemoryFree ( void * pMem, bool bArray );


inline void * operator new ( size_t iSize )
{
	return MemoryAllocate ( iSize, "unknown", -1, false );
}

inline void * operator new [] ( size_t iSize )
{
	return MemoryAllocate ( iSize, "unknown", -1, true );
}

inline void * operator new	( size_t iSize, char const * szFile, int iLine )
{
	return MemoryAllocate ( iSize, szFile, iLine, false );
}

inline void * operator new [] ( size_t iSize, char const * szFile, int iLine )
{
	return MemoryAllocate ( iSize, szFile, iLine, true );
}

inline void operator delete( void* pMem, char * szFilename, int nLine )
{
	MemoryFree ( pMem, false );
}

inline void operator delete [] ( void* pMem, char * szFilename, int nLine )
{
	MemoryFree ( pMem, true );
}

inline void operator delete ( void * pMem )
{
	MemoryFree ( pMem, false );
}

inline void operator delete [] ( void * pMem )
{
	MemoryFree ( pMem, true );
}

#define new new (__FILE__, __LINE__)

// for memory usage reporting
struct MemInfo_t
{
	unsigned long	m_iMaxMemUsage;			// max memory usage (bytes)
	unsigned long	m_iCurMemUsage;			// current memory usage (bytes)
	int				m_iCurAllocations;		// current allocations number
	int				m_iMaxAllocations;		// max allocations number
};

// returns memory usage info
const MemInfo_t & GetMemoryInfo ();

// for leak reporting
struct LeakInfo_t
{
	const char *	m_szFileName;			// the name of file
	int				m_iLine;				// line of file
	float			m_fSizeKb;				// leak size in kb
	int				m_nAllocation;			// allocation id
};

// resets memory leak iterator
void StartLeakIteration ();

// iterates next leak. returns false if fails
bool IterateNextLeak ( LeakInfo_t & tLeakInfo );

#else
	#define Shutdown_Memory __noop
#endif


//////////////////////////////////////////////////////////////////////////////////////////
// log stuff
enum
{
	 LOG_STRING_LENGTH	= 256
	,MAX_MESSAGE_LENGTH	= 256
};

#if FM_USE_ASSERT
	#define Assert(_expr) \
		{ \
			if ( !(_expr) ) \
			{ \
				wchar_t szFile [256];\
				wchar_t szExpr [256];\
				AnsiToUnicode(__FILE__, szFile);\
				AnsiToUnicode(#_expr,szExpr);\
				Log ( L"%s (%i) : Assert failed", szFile, __LINE__ ); \
				Warning ( L"Assert failed: %s\n%s, %i\n", szExpr, szFile, __LINE__ ); \
				DebugBreak (); \
			} \
		}\

#else
	#define Assert(_expr) (__assume(_expr))
#endif

#if FM_USE_VERIFY
	#define Verify(_expr) \
		{ \
			if ( !(_expr) ) \
			{ \
				wchar_t szFile [256];\
				wchar_t szExpr [256];\
				AnsiToUnicode(__FILE__, szFile);\
				AnsiToUnicode(#_expr,szExpr);\
				Log ( L"%s (%i) : Verify failed", szFile, __LINE__ ); \
				Warning ( L"Verify failed: %s\n%s, %i\n", szExpr, szFile, __LINE__ ); \
				DebugBreak (); \
			} \
		}\
		
#else
	#define Verify(_expr) _expr
#endif


#if FM_USE_LOG
class Log_c
{
public:	
					Log_c ();
					~Log_c ();

	void			SetLogFilePath ( const wchar_t * szPathName );	// full path + filename

	void			Add ( const wchar_t * szFormatStr, ... );
	void			SwitchTimeLogging ( bool bOn );

	void			LogTimeDate ();
	void			LogOSMemoryStatus ();	// OS memory status
	void			LogIntMemoryStatus ();	// internal memory status
	void			LogMemLeaks ( bool bLogUnknown = false );	
	void			LogWinError ();

	void			LogStartTiming ( int iTimer );
	void			LogTimeElapsed ( int iTimer, const wchar_t * szText );
	
private:
	enum
	{
		MAX_TIMERS = 16
	};

	bool			m_bInitialized;
	bool			m_bLogTime;
	wchar_t			m_szStr [LOG_STRING_LENGTH];
	wchar_t			m_szFileName[MAX_PATH];
	FILE *			m_pFile;
	double			m_dTimers [MAX_TIMERS];

	void			Clear ();
	bool			Open ();
	void			Close ();

	void			LogOSVersion ();
};


// common log
extern Log_c g_Log;
#endif

// log wrappers
#if ! FM_USE_LOG
	#define Log __noop
	#define LogWinError __noop
#else
	void Log ( const wchar_t * szFormatStr, ... );
	void LogWinError ();
#endif

#if ! FM_USE_WARNING
	#define Warning __noop
#else
	// display and log a warning
	void Warning ( const wchar_t * szFormat, ... );
#endif

#if ! FM_USE_ACHTUNG
	#define Achtung __noop
#else
	// Achtung execution, print a message
	void Achtung ( const wchar_t * szFormat, ... );
#endif


//////////////////////////////////////////////////////////////////////////////////////////
// timer stuff
class Timer_c
{
public:
					Timer_c ();

	double			GetTimeSec ();

private:
	__int64			m_iFreq;
	__int64			m_iStartTime;

	__int64			GetTime();
};

extern Timer_c g_Timer;

//////////////////////////////////////////////////////////////////////////////////////////
void Init_Core ();
void Shutdown_Core ();

#endif