#include "pch.h"

#include "LCore/clog.h"
#include "LCore/ctimer.h"
#include "LCore/cos.h"

const float F_MB = 1048576.0f;

#if FM_USE_LOG
Log_c g_Log;

///////////////////////////////////////////////////////////////////////////////////////////
// the log!
Log_c::Log_c ()
	: m_bLogTime 		( false )
	, m_bInitialized	( false )
{
	ZeroMemory ( &m_dTimers, sizeof ( m_dTimers ) );
}


void Log_c::SetLogFilePath ( const wchar_t * szPathName )
{
	m_sFileName = szPathName;
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

	DWORD uAttr = GetFileAttributes ( m_sFileName );

	if ( uAttr == 0xFFFFFFFF )
		return false;

	if ( uAttr & FILE_ATTRIBUTE_HIDDEN || uAttr & FILE_ATTRIBUTE_HIDDEN )
	{
		int iRes = SetFileAttributes ( m_sFileName, FILE_ATTRIBUTE_NORMAL );
		if ( ! iRes )
		{
			OutputDebugString ( L"ERROR: Unable to set log file attributes\n" );
			return false;
		}
	}
		
	if ( uAttr & FILE_ATTRIBUTE_DIRECTORY )
			return false;

	m_pFile = _wfopen ( m_sFileName, L"a+" );

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

	m_pFile = _wfopen ( m_sFileName, L"w+" );
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
#if MEMORY_TRACK
	const MemInfo_t & tInfo = GetMemoryInfo ();
	Add ( L"Current allocations: %d",		tInfo.m_iCurAllocations );
	Add ( L"Max allocations: %d",			tInfo.m_iMaxAllocations );
	Add ( L"Current memory usage: %.02f Mb",float (tInfo.m_iCurMemUsage) / F_MB );
	Add ( L"Max memory usage: %.02f Mb",	float (tInfo.m_iMaxMemUsage) / F_MB );
#endif
}


void Log_c::LogMemLeaks ( bool bLogUnknown )
{
#if MEMORY_TRACK
	bool bReport = false;

	StartLeakIteration ();
	LeakInfo_t tLeak;

	while ( IterateNextLeak ( tLeak ) )
		if ( bLogUnknown || tLeak.m_iLine != -1 )
		{
			bReport = true;
			break;
		}

	if ( bReport )
	{
		StartLeakIteration ();

		Add ( L"\n---- Memory leaks detected!! ---" );

		while ( IterateNextLeak ( tLeak ) )
			if ( bLogUnknown || tLeak.m_iLine != -1 )
			{
				if ( tLeak.m_szFileName )
					Add ( L"Memory leak: %s(%i), %.3f kb in allocation %d", AnsiToUnicode ( tLeak.m_szFileName ).c_str (), tLeak.m_iLine, tLeak.m_fSizeKb, tLeak.m_nAllocation );
				else
					Add ( L"Memory leak: %.3f kb in allocation %d", tLeak.m_fSizeKb, tLeak.m_nAllocation );
			}

		Add ( L"" );
	}
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

void LogMemLeaks ( bool bLogUnknown )
{
	g_Log.LogMemLeaks ( bLogUnknown );
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