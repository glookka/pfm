#ifndef _cmemory_
#define _cmemory_

#include "LCore/cmain.h"

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

#endif