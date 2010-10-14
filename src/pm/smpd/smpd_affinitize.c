#include "smpd.h"
#include <stdlib.h>

#define MAX_PROCESSORS (sizeof(DWORD_PTR) * 8)

/*
    The relative priorities of system resources.  Small non-negative values
    are considered more important (i.e. higher priority).
*/
typedef int priority_t;
static const priority_t prioUnused    =-1;
static const priority_t prioCacheBase = 1;  /* L1 cache will have priority Base+1, etc. */
static const priority_t prioPackage   = 1000;
static const priority_t prioNuma      = 1001;

/*
    An entry in the processor information priority table.  Each entry contains 
    the priority of a given system resource type (e.g. L1 cache, L2 cache, etc.)
    along with a list of all resources of that type.  For each resource, the set
    of processes sharing that resouce is stored in the list.
*/

typedef struct
{
    priority_t priority;
    int nResources;
    struct
    {
        DWORD_PTR ProcessorMask;
        int UsageCount;
    } resources[MAX_PROCESSORS];
} ResourceTableEntry;


static unsigned int s_affinity_idx = 0;
static unsigned int s_affinity_max = 0;
static DWORD_PTR s_affinity_table[MAX_PROCESSORS];

DWORD_PTR smpd_get_next_process_affinity_mask()
{
    DWORD_PTR Mask;
    if (s_affinity_max == 0){
        smpd_dbg_printf("Affinity table not initialized. Returning invalid mask\n");
        return 0;
    }

    Mask = s_affinity_table[s_affinity_idx % s_affinity_max];
    s_affinity_idx++;
    return Mask;
}


static inline LONG_PTR lowest_bit(LONG_PTR x)
{
    return (x & -x);
}

static inline BOOL one_bit_set(LONG_PTR x)
{
    return ( x == lowest_bit(x) );
}

static inline void dbg_print_relationship(SYSTEM_LOGICAL_PROCESSOR_INFORMATION *pInfo)
{
    switch(pInfo->Relationship){
        case RelationProcessorCore:
            smpd_dbg_printf("Processor core [%x] info\n", pInfo->ProcessorCore.Flags);
            break;
        /* case RelationProcessorPackage: */
        case RelationCache:
            switch (pInfo->Cache.Type)
            {
                case CacheUnified:
                    smpd_dbg_printf("Unified cache info\n");
                    break;
                case CacheData:
                    smpd_dbg_printf("Data cache info\n");
                    break;
                case CacheInstruction:
                    smpd_dbg_printf("Instruction cache info\n");
                    break;
                case CacheTrace:
                    smpd_dbg_printf("Trace cache info\n");
                    break;
                default:
                    smpd_dbg_printf("Unknown cache info [type=%d]\n", pInfo->Cache.Type);
                    break;
            }
            smpd_dbg_printf("level=%d, assoc=%d, lnsz=%d, sz=%d\n", pInfo->Cache.Level, pInfo->Cache.Associativity, pInfo->Cache.LineSize, pInfo->Cache.Size);
            break;
        case RelationNumaNode:
            smpd_dbg_printf("Numa node [%d] Info\n", pInfo->NumaNode.NodeNumber);
            break;
        default:
            smpd_dbg_printf("Unknown Relationship [rel=%d]\n", pInfo->Relationship);
    }
}

static inline priority_t get_relationship_priority(SYSTEM_LOGICAL_PROCESSOR_INFORMATION* pInfo)
{
    switch (pInfo->Relationship)
    {
        case RelationProcessorCore :
            return prioUnused;

        /* FIXME: Is this defined for win32 ?
    	case RelationProcessorPackage :
            return prioPackage;
        */

    	case RelationCache :
    	{
            switch (pInfo->Cache.Type)
            {
                case CacheUnified:
                case CacheData:
                    return prioCacheBase + pInfo->Cache.Level;
                    
                case CacheInstruction:
                case CacheTrace:
                    return prioUnused;

                default:
                    return prioUnused;
            }
    	}

    	case RelationNumaNode :
            return prioNuma;
                	
    	default:
            return prioUnused;
    }
}


/*
    This function is used to sort the array of SYSTEM_LOGICAL_PROCESSOR_INFORMATION
    by relationship priority (from highest to lowest importance).
*/
static int __cdecl compare_proc_info(const void *p1, const void *p2)
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION* pInfo1;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION* pInfo2;
    int prio1, prio2;
    DWORD_PTR mask1, mask2;

    pInfo1 = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*) p1;
    pInfo2 = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*) p2;
    
    prio1 = get_relationship_priority(pInfo1);
    prio2 = get_relationship_priority(pInfo2);

    if (prio1 < prio2)
        return -1;
    if (prio1 > prio2)
        return 1;

    mask1 = pInfo1->ProcessorMask;
    mask2 = pInfo2->ProcessorMask;

    return mask1 < mask2 ? -1 : (mask1 > mask2 ? 1 : 0);
}


/*
    Group resources types by priority in the resource table.

    PRECONDITION: pInfo must be sorted according to the compare_proc_info function above
*/
static int init_resource_table(SYSTEM_LOGICAL_PROCESSOR_INFORMATION* pInfo, int count, ResourceTableEntry *pTable)
{
    int nTableEntries = 0, i=0, j=0, k=0;
    priority_t prevPrio = prioUnused;
    DWORD_PTR EntryMask = 0;

    for (i=0; i<count; i++)
    {
        priority_t prio = get_relationship_priority(&pInfo[i]);
        if (prio == prioUnused)
            continue;

        SMPDU_Assert(prio >= prevPrio);
        if (prio != prevPrio)
        {
            pTable[nTableEntries].priority = prio;
            pTable[nTableEntries].nResources = 0;
            EntryMask = 0;
            nTableEntries++;
        }

        if( ( EntryMask & pInfo[i].ProcessorMask ) != 0){
            /* Ignore duplicate resource infos */
            smpd_dbg_printf("WARNING: Duplicate resource Info [for core=%x], Ignoring resource : \n", pInfo[i].ProcessorMask);
            smpd_dbg_printf("==========================\n");
            dbg_print_relationship(&pInfo[i]);
            smpd_dbg_printf("==========================\n");
            continue;
        }

        EntryMask |= pInfo[i].ProcessorMask;

        j = nTableEntries - 1;
        k = pTable[j].nResources;
        pTable[j].resources[k].ProcessorMask = pInfo[i].ProcessorMask;
        pTable[j].resources[k].UsageCount = 0;
        pTable[j].nResources++;

        prevPrio = prio;
    }

    return nTableEntries;
}


static DWORD_PTR get_min_usage_mask(DWORD_PTR CandidateMask, ResourceTableEntry *pEntry)
{
    /*
        From among the currently remaining set of candidates, find all processors that map 
        to one of the least heavily used resources of the current type.  If no candidate 
        processor maps onto a resource of the current type, zero is returned.
    */
    int minUsageCount = INT_MAX, j=0;
    DWORD_PTR minUsageMask = 0;
    for (j=0; j<pEntry->nResources; j++)
    {
        if ( (pEntry->resources[j].ProcessorMask & CandidateMask) != 0)
        {
            if (pEntry->resources[j].UsageCount < minUsageCount)
            {
                minUsageMask  = pEntry->resources[j].ProcessorMask;
                minUsageCount = pEntry->resources[j].UsageCount;
            }
            else if (pEntry->resources[j].UsageCount == minUsageCount)
            {
                minUsageMask |= pEntry->resources[j].ProcessorMask;
            }
        }
    }
    return minUsageMask;
}


static DWORD_PTR get_least_used_processor_bit(DWORD_PTR CandidateMask, ResourceTableEntry *pTable, int nTableEntries)
{
    int i=0;
    for (i=0; i<nTableEntries && !one_bit_set(CandidateMask); i++)
    {
        DWORD_PTR minUsageMask = get_min_usage_mask(CandidateMask, &pTable[i]);

        /*
            Information about this resource type is unavailable for the remaining candidate processors.
            However, it may be available for the next, lower-priority resource type, so move on.
        */
        if (minUsageMask == 0)
            continue;

        /*
            Update the candidate set only to include the processors mapping to the least-used resources.
            Move on to the next, lower-priority resource type.
        */
        CandidateMask &= minUsageMask;
        SMPDU_Assert(CandidateMask != 0);
    }

    /*
        Return the mask for only one processor, in case more than one candidate remains.
    */
    return lowest_bit(CandidateMask);
}


static void update_usage_counts(DWORD_PTR AssignMask, ResourceTableEntry *pTable, int nTableEntries)
{
    int i=0, j=0;
    for (i=0; i<nTableEntries; i++)
    {
        for (j=0; j<pTable[i].nResources; j++)
        {
            if ( (pTable[i].resources[j].ProcessorMask & AssignMask) != 0 )
            {
                pTable[i].resources[j].UsageCount++;
                break;
            }
        }
    }
}

/*
    This function initilizes the affinity table.  The processor assignments to processes 
    are distributed so as to balance the use of system resources.
*/
typedef BOOL (WINAPI *LPFN_GetLogicalProcessorInformation)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);
BOOL smpd_init_affinity_table()
{
    /*
        Get this process affinity mask (the system mask is unused)
    */
    int count, nTableEntries;
    BOOL fSucc, retval = TRUE;
    DWORD Length;
    DWORD_PTR SystemMask, ReserveMask, PrimaryMask;
    DWORD_PTR UsableProcessorMask;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION* pInfo = NULL;
    ResourceTableEntry* pTable = NULL;
    LPFN_GetLogicalProcessorInformation lpfn_get_logical_proc_info;

    fSucc = GetProcessAffinityMask(GetCurrentProcess(), &UsableProcessorMask, &SystemMask);
    if(!fSucc){
        return FALSE;
    }

    /* Check if GetLogicalProcessorInformation() is available */
    lpfn_get_logical_proc_info =
        (LPFN_GetLogicalProcessorInformation ) GetProcAddress(GetModuleHandle(TEXT("kernel32")),
                                                    "GetLogicalProcessorInformation");
    if(lpfn_get_logical_proc_info == NULL){
        smpd_dbg_printf("GetLogicalProcessorInformation() not available\n"); 
        return FALSE;
    }
    /*
        Retrieve the length required to store the processor information data
    */
    Length = 0;
    /* This call will fail - however it will return the length reqd to store proc info*/
    fSucc = lpfn_get_logical_proc_info(NULL, &Length);
    if(Length == 0){
        smpd_dbg_printf("GetLogicalProcessorInformation() returned invalid (0 bytes) length to store proc info\n");
        return FALSE;
    }

    /*
        Allocate the processor information buffer
    */
    pInfo = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*) malloc(Length);
    if(pInfo == NULL){
        smpd_err_printf("Allocating memory for system logical proc info failed\n");
        return FALSE;
    }

    /*
        Query for the processors information
    */
    fSucc = lpfn_get_logical_proc_info(pInfo, &Length);
    if(!fSucc){
        smpd_err_printf("GetLogicalProcessorInformation() failed (error = %d)\n", GetLastError());
        goto fn_fail;
    }
    
    count = Length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);

    /*
        Sort the processor data in order of priority, from most to least important.
    */
    qsort(pInfo, count, sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION), compare_proc_info);

    pTable = (ResourceTableEntry*) malloc(count * sizeof(ResourceTableEntry));
    if (pTable == NULL){
        smpd_err_printf("Allocating memory for Resourcetableentry failed\n");
        goto fn_fail;
    }

    /*
        Initialize resource table from the sorted PROCESSOR_INFORMATION array
    */
    nTableEntries = init_resource_table(pInfo, count, pTable);

    /*
        Reserve CPU zero.  We will not assigned a reserved processor to a process unless all other
        processors are already busy.  We can generalize this code to reserve other processors.
    */
    ReserveMask = 0x1;
    ReserveMask &= UsableProcessorMask;

    /*
        First assign all primary processors, then all the reserve processors.
    */
    PrimaryMask = ( UsableProcessorMask & ~ReserveMask );
    while ( (PrimaryMask | ReserveMask) != 0)
    {
        DWORD_PTR AssignMask;
        DWORD_PTR* pRemainMask = ( PrimaryMask != 0 ? &PrimaryMask : &ReserveMask );
        AssignMask = get_least_used_processor_bit(*pRemainMask, pTable, nTableEntries);
        update_usage_counts(AssignMask, pTable, nTableEntries);

        s_affinity_table[s_affinity_max] = AssignMask;
        s_affinity_max++;
        *pRemainMask &= ~AssignMask;
    }

    free(pTable);
 fn_exit:
    free(pInfo);
    return retval;
fn_fail:
    retval = FALSE;
    goto fn_exit;
}

/* This function returns an affinity mask corresponding to the logical processor
 * id, proc_num, specified. In the case of an error a NULL mask, 0x0, is returned.
 */
DWORD_PTR smpd_get_processor_affinity_mask(int proc_num)
{
    BOOL fSucc;
    DWORD_PTR system_mask, usable_processor_mask, mask;

    if(proc_num < 0){
        smpd_err_printf("Invalid processor num specified\n");
        return NULL;
    }

    /* Get the proc affinity mask for the process/system */
    fSucc = GetProcessAffinityMask(GetCurrentProcess(), &usable_processor_mask, &system_mask);
    if(!fSucc){
        smpd_err_printf("Unable to get proc affinity mask\n");
        return NULL;
    }

    mask = 0x1 << (proc_num % ( 8 * sizeof(DWORD_PTR)));
    mask &= system_mask;

    return mask;
}

