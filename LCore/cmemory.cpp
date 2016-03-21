#include "pch.h"

#include "LCore/cmemory.h"
#include "LCore/clog.h"

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
	pMemChunk->m_iLine		= iLine;
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
