#ifndef _clog_
#define _clog_

#include "LCore/cmain.h"
#include "LCore/cstr.h"

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
	Str_c	m_sFileName;
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
	#define LogMemLeaks __noop
	#define LogWinError __noop
#else
	void Log ( const wchar_t * szFormatStr, ... );
	void LogMemLeaks ( bool bLogUnknown = false );
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

#endif