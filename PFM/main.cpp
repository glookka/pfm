#include "pch.h"
#include "main.h"

#include "std.h"
#include "system.h"

//////////////////////////////////////////////////////////////////////////////////////////
// memory stuff
#if FM_MEMORY_TRACK

// memory critical section
CRITICAL_SECTION g_MemoryCritical;

// memory usage info
MemInfo_t g_tMemInfo;

// the memory info chunk. 32 bytes
#pragma pack(8)
struct MemChunk_t
{
	const char *	m_szFileName;
	int				m_iLine;
	bool			m_bArray;
	unsigned int	m_iLength;
	void *			m_pAddr;
	MemChunk_t *	m_pNext;
	MemChunk_t *	m_pPrev;
	int				m_nAllocation;
};
#pragma pack()


MemChunk_t * g_pMemChunkHead = NULL;		// mem chunk list head
int			 g_nAllocation = 0;				// next allocation id

MemChunk_t * g_pMemChunkIterator = NULL;	// leak report helper

extern bool g_bLogInitialized;


void Shutdown_Memory ()
{
	DeleteCriticalSection ( &g_MemoryCritical );
}


const MemInfo_t & GetMemoryInfo ()
{
	return g_tMemInfo;
}


void StartLeakIteration ()
{
	g_pMemChunkIterator = g_pMemChunkHead;
}


bool IterateNextLeak ( LeakInfo_t & tLeakInfo )
{
	if ( ! g_pMemChunkIterator )
		return false;

	tLeakInfo.m_szFileName	= g_pMemChunkIterator->m_szFileName;
	tLeakInfo.m_iLine		= g_pMemChunkIterator->m_iLine;
	tLeakInfo.m_fSizeKb		= float ( g_pMemChunkIterator->m_iLength ) / 1024.0f;
	tLeakInfo.m_nAllocation	= g_pMemChunkIterator->m_nAllocation;

	g_pMemChunkIterator = g_pMemChunkIterator->m_pNext;

	return true;
}


// common memory allocation
void * MemoryAllocate ( size_t iSize, char const * szFile, int iLine, bool bArray )
{
	static bool bFirstTime = true;
	if ( bFirstTime )
	{
		InitializeCriticalSection ( &g_MemoryCritical );
		bFirstTime = false;

		ZeroMemory ( & g_tMemInfo, sizeof ( g_tMemInfo ) );
	}

	EnterCriticalSection ( &g_MemoryCritical );

	if ( !iSize )
		return NULL;

	void * pPtr = malloc ( sizeof (MemChunk_t) + iSize );

	if ( ! pPtr )
		Achtung ( L"MemoryAllocate : could not allocate %d bytes", iSize );

	MemChunk_t * pMemChunk	= (MemChunk_t*) pPtr;
	pMemChunk->m_iLine		= g_bLogInitialized ? iLine : -1;
	pMemChunk->m_iLength	= iSize;
	pMemChunk->m_pAddr		= pPtr;
	pMemChunk->m_pPrev		= NULL;
	pMemChunk->m_pNext		= NULL;
	pMemChunk->m_bArray		= bArray;
	pMemChunk->m_szFileName	= szFile;
	pMemChunk->m_nAllocation= g_nAllocation++;
		
	// chain it!
	if ( ! g_pMemChunkHead )
		g_pMemChunkHead	= pMemChunk;
	else
	{
		pMemChunk->m_pNext = g_pMemChunkHead;
		g_pMemChunkHead->m_pPrev = pMemChunk;
		g_pMemChunkHead = pMemChunk;
	}

		// info...
	g_tMemInfo.m_iCurMemUsage += (unsigned long) iSize;
	g_tMemInfo.m_iMaxMemUsage = Max (  g_tMemInfo.m_iCurMemUsage, g_tMemInfo.m_iMaxMemUsage );

	g_tMemInfo.m_iCurAllocations++;
	g_tMemInfo.m_iMaxAllocations = Max ( g_tMemInfo.m_iCurAllocations, g_tMemInfo.m_iMaxAllocations );

	LeaveCriticalSection ( &g_MemoryCritical );

	return (void *)(pMemChunk + 1);
}


void MemoryFree ( void * pMem, bool bArray )
{
	EnterCriticalSection ( &g_MemoryCritical );

	if ( pMem )
	{	
		MemChunk_t * pMemChunk = (MemChunk_t *) pMem - 1;

		Assert ( pMemChunk->m_bArray == bArray );

		if ( pMemChunk->m_pNext )
			pMemChunk->m_pNext->m_pPrev = pMemChunk->m_pPrev;
	
		if ( pMemChunk->m_pPrev )
			pMemChunk->m_pPrev->m_pNext = pMemChunk->m_pNext;
		else
			g_pMemChunkHead = pMemChunk->m_pNext;

		g_tMemInfo.m_iCurAllocations--;
		g_tMemInfo.m_iCurMemUsage -= pMemChunk->m_iLength;

		if ( g_tMemInfo.m_iCurAllocations < 0 )
		{
			Assert ( g_tMemInfo.m_iCurAllocations >= 0 );
		}

		Assert ( g_tMemInfo.m_iCurMemUsage >= 0 );
	
		free ( (void * ) pMemChunk );
	}

	LeaveCriticalSection ( &g_MemoryCritical );
} 
#endif


//////////////////////////////////////////////////////////////////////////////////////////
// log stuff
const float F_MB = 1048576.0f;
bool g_bLogInitialized = false;

#if FM_USE_LOG
Log_c g_Log;

Log_c::Log_c ()
	: m_bLogTime 		( false )
	, m_bInitialized	( false )
{
	ZeroMemory ( &m_dTimers, sizeof ( m_dTimers ) );
	g_bLogInitialized = true;
}


Log_c::~Log_c ()
{
	LogMemLeaks ( false );
}


void Log_c::SetLogFilePath ( const wchar_t * szPathName )
{
	wcscpy ( m_szFileName, szPathName );
	m_bInitialized = true;

	Clear ();
	Add ( L"Log file: [%s]", szPathName );
	LogTimeDate ();
	LogOSVersion ();
	LogOSMemoryStatus ();
	Add ( L"" );
}


void Log_c::SwitchTimeLogging ( bool bOn )
{
	m_bLogTime = bOn;
}


bool Log_c::Open ()
{
	if ( ! m_bInitialized )
	{
		OutputDebugString ( L"WARNING: Set log file path and name first!\n" );
		return false;
	}

	DWORD uAttr = GetFileAttributes ( m_szFileName );

	if ( uAttr == 0xFFFFFFFF )
		return false;

	if ( uAttr & FILE_ATTRIBUTE_HIDDEN || uAttr & FILE_ATTRIBUTE_HIDDEN )
	{
		int iRes = SetFileAttributes ( m_szFileName, FILE_ATTRIBUTE_NORMAL );
		if ( ! iRes )
		{
			OutputDebugString ( L"ERROR: Unable to set log file attributes\n" );
			return false;
		}
	}
		
	if ( uAttr & FILE_ATTRIBUTE_DIRECTORY )
			return false;

	m_pFile = _wfopen ( m_szFileName, L"a+" );

	if ( ! m_pFile )
		OutputDebugString ( L"ERROR: Can't open log file for appending\n" );

	return !!m_pFile;
}


void Log_c::Close ()
{
	if ( m_pFile )
	{
		fclose ( m_pFile );
        m_pFile = NULL;
	}
}



void Log_c::Clear ()
{
	if ( ! m_bInitialized )
		return;

	m_pFile = _wfopen ( m_szFileName, L"w+" );
	Close ();
}


void Log_c::Add ( const wchar_t * szFormatStr, ... )
{
	m_szStr [0] = L'\0';

	if ( m_bLogTime )
	{
		SYSTEMTIME tSystemTime;
		GetLocalTime ( &tSystemTime );
		swprintf ( m_szStr, L"%d:%d:%d ", tSystemTime.wHour, tSystemTime.wMinute, tSystemTime.wSecond );
	}

	int iLen = wcslen ( m_szStr );
	wchar_t * szTmp = & ( m_szStr [iLen] );
		
	va_list argptr;

	va_start ( argptr, szFormatStr );
	vswprintf ( szTmp, szFormatStr, argptr );
	va_end ( argptr );

	iLen = wcslen ( m_szStr );
	m_szStr [iLen]	 = L'\n';
	m_szStr [iLen+1] = L'\0';

	OutputDebugString ( m_szStr );

	if ( ! Open () )
		return;

	fwprintf ( m_pFile, L"%s", m_szStr );

	Close ();
}


void Log_c::LogTimeDate ()
{
	SYSTEMTIME tSysTime;
	GetLocalTime ( &tSysTime );
	Add ( L"Current date/time: %d-%d-%d, %d:%d:%d",
		tSysTime.wDay, tSysTime.wMonth, tSysTime.wYear,
		tSysTime.wHour, tSysTime.wMinute, tSysTime.wSecond );
}


void Log_c::LogOSVersion()
{
	Add ( L"--- OS Info ---" );
	OSVERSIONINFO tOSInfo;
	tOSInfo.dwOSVersionInfoSize = sizeof ( OSVERSIONINFO );
	GetVersionEx ( &tOSInfo );
	
	switch ( tOSInfo.dwPlatformId )
	{
		case VER_PLATFORM_WIN32_CE:
			Add ( L"OS: Windows CE" );
			break;

		default:
			Add ( L"OS: Unknown" );
			break;
	}

	Add ( L"Version: %u.%u.%u", tOSInfo.dwMajorVersion, tOSInfo.dwMinorVersion, tOSInfo.dwBuildNumber & 0xFFFF );
}


void Log_c::LogOSMemoryStatus ()
{
	MEMORYSTATUS tMemStat;
	tMemStat.dwLength = sizeof ( MEMORYSTATUS );

	GlobalMemoryStatus ( &tMemStat );

	Add ( L"--- Memory Status ---" );
	Add ( L"Memory load: %d%%", 				tMemStat.dwMemoryLoad );
	Add ( L"Total physical mem: %.02f Mb", 		float ( tMemStat.dwTotalPhys ) / F_MB );
	Add ( L"Available physical mem:  %.02f Mb", float ( tMemStat.dwAvailPhys ) / F_MB );
	Add ( L"Total virtual mem: %.02f Mb", 		float ( tMemStat.dwTotalVirtual ) / F_MB );
	Add ( L"Available virtual mem: %.02f Mb", 	float ( tMemStat.dwAvailVirtual ) / F_MB );
	Add ( L"Total page file: %.02f Mb", 		float ( tMemStat.dwTotalPageFile ) / F_MB );
	Add ( L"Available page file: %.02f Mb", 	float ( tMemStat.dwAvailPageFile ) / F_MB );
}


void Log_c::LogIntMemoryStatus ()
{
#if FM_MEMORY_TRACK
	const MemInfo_t & tInfo = GetMemoryInfo ();
	Add ( L"Current allocations: %d",		tInfo.m_iCurAllocations );
	Add ( L"Max allocations: %d",			tInfo.m_iMaxAllocations );
	Add ( L"Current memory usage: %.02f Mb",float (tInfo.m_iCurMemUsage) / F_MB );
	Add ( L"Max memory usage: %.02f Mb",	float (tInfo.m_iMaxMemUsage) / F_MB );
#endif
}


void Log_c::LogMemLeaks ( bool bLogUnknown )
{
#if FM_MEMORY_TRACK
	StartLeakIteration ();

	wchar_t szBuf [MAX_PATH];

	LeakInfo_t tLeak;
	while ( IterateNextLeak ( tLeak ) )
		if ( bLogUnknown || tLeak.m_iLine != -1 )
		{
			if ( tLeak.m_szFileName )
			{
				bool bStatics = ( strstr ( tLeak.m_szFileName, "std.cpp" ) && tLeak.m_iLine == 429 ) ||
								( strstr ( tLeak.m_szFileName, "std.h" ) && tLeak.m_iLine == 280 );

				if ( !bStatics )
				{
					AnsiToUnicode ( tLeak.m_szFileName, szBuf );
					Add ( L"Memory leak: %s(%i), %.3f kb in allocation %d", szBuf, tLeak.m_iLine, tLeak.m_fSizeKb, tLeak.m_nAllocation );
				}
			}
			else
				Add ( L"Memory leak: %.3f kb in allocation %d", tLeak.m_fSizeKb, tLeak.m_nAllocation );
		}

	Add ( L"" );
#endif
}


void Log_c::LogWinError ()
{
	Add ( L"Windows error: %s", GetWinErrorText ( GetLastError () ).c_str () );
}


void Log_c::LogStartTiming ( int iTimer )
{
	Assert ( iTimer >= 0 && iTimer < MAX_TIMERS );
	m_dTimers [iTimer] = g_Timer.GetTimeSec ();
}


void Log_c::LogTimeElapsed ( int iTimer, const wchar_t * szText )
{
	Assert ( iTimer >= 0 && iTimer < MAX_TIMERS );
	Add ( L"%s: %.4f", szText, g_Timer.GetTimeSec () - m_dTimers [iTimer] );
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////
// helper stuff

#if FM_USE_LOG
void Log ( const wchar_t * szFormatStr, ... )
{
	static wchar_t szStr [MAX_MESSAGE_LENGTH];
	va_list argptr;

	va_start ( argptr, szFormatStr );
	vswprintf ( szStr, szFormatStr, argptr );
	va_end	 ( argptr );

	g_Log.Add ( szStr );
}

void LogWinError ()
{
	g_Log.LogWinError ();
}
#endif


#if FM_USE_WARNING
void Warning ( const wchar_t * szFormat, ... )
{
	static wchar_t szStr [MAX_MESSAGE_LENGTH];
	va_list argptr;

	va_start ( argptr, szFormat );
	vswprintf ( szStr, szFormat, argptr );
	va_end	 ( argptr );

	Log ( szStr );
	MessageBox ( NULL, szStr, L"Warning!", MB_OK );
}
#endif


#if FM_USE_ACHTUNG
void Achtung ( const wchar_t * szFormat, ... )
{
	static wchar_t szStr [MAX_MESSAGE_LENGTH];
	va_list argptr;

	va_start ( argptr, szFormat );
	vswprintf ( szStr, szFormat, argptr );
	va_end	 ( argptr );

	Log ( szStr );
	int iRes = MessageBox ( NULL, szStr, L"Application Aborted!", MB_OK );
	if ( iRes == IDOK )
		DebugBreak ();
}
#endif


//////////////////////////////////////////////////////////////////////////////////////////
// timer stuff
Timer_c g_Timer;

Timer_c::Timer_c ()
{
	LARGE_INTEGER iTmpTime;
	QueryPerformanceFrequency ( &iTmpTime );
	m_iFreq = iTmpTime.QuadPart;
}


__int64 Timer_c::GetTime ()
{
	LARGE_INTEGER iTmpTime;
	QueryPerformanceCounter ( &iTmpTime );
	return iTmpTime.QuadPart;
}


double Timer_c::GetTimeSec ()
{
	LARGE_INTEGER iTmpTime;
	QueryPerformanceCounter ( &iTmpTime );
	return double ( iTmpTime.QuadPart ) / double ( m_iFreq );
}

//////////////////////////////////////////////////////////////////////////////////////////
void Init_Core ()
{
	// randomization
	LARGE_INTEGER uLarge;
	QueryPerformanceCounter ( &uLarge);
	DWORD uSeed = uLarge.LowPart ^ uLarge.HighPart;
	srand ( uSeed );
}


void Shutdown_Core ()
{
	Shutdown_Memory ();
}