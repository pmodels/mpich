/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*#define ZERO_RANK 0x10101010*/
#define ZERO_RANK 0x12345678

#undef USE_SYNCHRONIZE_SHMAPPING

#ifdef HAVE_SHARED_PROCESS_READ
#undef FUNCNAME
#define FUNCNAME InitSharedProcesses
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int InitSharedProcesses(MPIDI_PG_t *pg, int nRank)
{
    int mpi_errno;
#ifndef HAVE_WINDOWS_H
    char filename[256];
#endif
    int i;
    MPIDI_CH3I_Shared_process_t *pSharedProcess;
    MPIDI_CH3I_PG *pgch = (MPIDI_CH3I_PG *)pg->channel_private;
    int nProc;

    nProc = MPIDI_PG_Get_size(pg);

    /* initialize arrays */
#ifdef HAVE_WINDOWS_H
    pgch->pSharedProcessHandles = (HANDLE*)MPIU_Malloc(sizeof(HANDLE) * nProc);
#else
    pgch->pSharedProcessIDs = (int*)MPIU_Malloc(sizeof(int) * nProc);
    pgch->pSharedProcessFileDescriptors = (int*)MPIU_Malloc(sizeof(int) * nProc);
#endif

    pSharedProcess = pgch->pSHP;

#ifdef HAVE_WINDOWS_H
    pSharedProcess[nRank].nPid = GetCurrentProcessId();
#else
    pSharedProcess[nRank].nPid = getpid();
#endif
    pSharedProcess[nRank].bFinished = FALSE;
    if (nRank == 0)
	pSharedProcess[nRank].nRank = ZERO_RANK;
    else
	pSharedProcess[nRank].nRank = nRank;

    for (i=0; i<nProc; i++)
    {
        if (i != nRank)
        {
	    if (i == 0)
	    {
		while (pSharedProcess[i].nRank != ZERO_RANK)
		    MPIU_Yield();
	    }
	    else
	    {
		while (pSharedProcess[i].nRank != i)
		    MPIU_Yield();
	    }
#ifdef HAVE_WINDOWS_H
            /*MPIU_DBG_PRINTF(("Opening process[%d]: %d\n", i, pSharedProcess[i].nPid));*/
            pgch->pSharedProcessHandles[i] =
                OpenProcess(STANDARD_RIGHTS_REQUIRED | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, 
                            FALSE, pSharedProcess[i].nPid);
            if (pgch->pSharedProcessHandles[i] == NULL)
            {
                int err = GetLastError();
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**OpenProcess", "**OpenProcess %d %d", i, err); /*"unable to open process %d, error %d\n", i, err);*/
		return mpi_errno;
            }
#else
            MPIU_Snprintf(filename, 256, "/proc/%d/mem", pSharedProcess[i].nPid);
            pgch->pSharedProcessIDs[i] = pSharedProcess[i].nPid;
            pgch->pSharedProcessFileDescriptors[i] = open(filename, O_RDWR/*O_RDONLY*/);
            if (pgch->pSharedProcessFileDescriptors[i] == -1)
	    {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**open", "**open %s %d %d", filename, pSharedProcess[i].nPid, errno);
		return mpi_errno;
	    }
#endif
        }
        else
        {
#ifdef HAVE_WINDOWS_H
            pgch->pSharedProcessHandles[i] = NULL;
#else
            pgch->pSharedProcessIDs[i] = 0;
            pgch->pSharedProcessFileDescriptors[i] = 0;
#endif
        }
    }
    if (nRank == 0)
    {
        for (i=1; i<nProc; i++)
        {
            while (pSharedProcess[i].bFinished != TRUE)
                MPIU_Yield();
	}
	/* Why are the fields erased here? */
	for (i=1; i<nProc; i++)
	{
            pSharedProcess[i].nPid = -1;
            pSharedProcess[i].bFinished = -1;
            pSharedProcess[i].nRank = -1;
        }
        pSharedProcess[0].nPid = -1;
        pSharedProcess[0].nRank = -1;
        pSharedProcess[0].bFinished = TRUE;
    }
    else
    {
        pSharedProcess[nRank].bFinished = TRUE;
        while (pSharedProcess[0].bFinished == FALSE)
            MPIU_Yield();
    }
    return MPI_SUCCESS;
}
#endif

/*@
   MPIDI_CH3I_SHM_Get_mem - allocate and get address and size of memory shared by all processes. 

   Parameters:
+  int nTotalSize
.  int nRank
-  int nNproc

   Notes:
    Set the global variables pg->ch.addr, pg->ch.size, pg->ch.id
@*/
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Get_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_Get_mem(MPIDI_PG_t *pg, int nTotalSize, int nRank, int nNproc, BOOL bUseShm)
{
    int mpi_errno;
    MPIDI_CH3I_PG *pgch = (MPIDI_CH3I_PG *)pg->channel_private;
#ifdef HAVE_SHARED_PROCESS_READ
    int shp_offset;
#endif
#if defined(HAVE_WINDOWS_H) && defined(USE_SYNCHRONIZE_SHMAPPING)
    HANDLE hSyncEvent1, hSyncEvent2;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);

#ifdef HAVE_SHARED_PROCESS_READ
    /* add room at the end of the shard memory region for the shared process information */
    shp_offset = nTotalSize;
    nTotalSize += nNproc * sizeof(MPIDI_CH3I_Shared_process_t);
#endif

#if defined(HAVE_WINDOWS_H) && defined(USE_SYNCHRONIZE_SHMAPPING)
    hSyncEvent1 = CreateEvent(NULL, TRUE, FALSE, "mpich2shmsyncevent1");
    hSyncEvent2 = CreateEvent(NULL, TRUE, FALSE, "mpich2shmsyncevent2");
#endif

    if (nTotalSize < 1)
    {
	/*MPIDI_err_printf("MPIDI_CH3I_SHM_Get_mem", "unable to allocate %d bytes of shared memory: must be greater than zero.\n", nTotalSize);*/
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	pgch->addr = NULL;
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	return mpi_errno;
    }

    if (bUseShm)
    {
	/* Create the shared memory object */
#ifdef USE_POSIX_SHM
	pgch->id = shm_open(pgch->key, O_RDWR | O_CREAT, 0600);
	if (pgch->id == -1)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shm_open", "**shm_open %s %d", pgch->key, errno);
	    pgch->addr = NULL;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	    return mpi_errno;
	}
	ftruncate(pgch->id, nTotalSize);
#elif defined (USE_SYSV_SHM)
	pgch->id = shmget(pgch->key, nTotalSize, IPC_CREAT | SHM_R | SHM_W);
	if (pgch->id == -1) 
	{
	    if (errno == EINVAL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmsize", "**shmsize %d", nTotalSize);
	    }
	    else
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmget", "**shmget %d", errno);
	    }
	    pgch->addr = NULL;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	    return mpi_errno;
	}
#elif defined (USE_WINDOWS_SHM)
	pgch->id = CreateFileMapping(
	    INVALID_HANDLE_VALUE,
	    NULL,
	    PAGE_READWRITE,
	    0, 
	    nTotalSize,
	    pgch->key);
	if (pgch->id == NULL) 
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**CreateFileMapping", "**CreateFileMapping %d", GetLastError()); /*"Error in CreateFileMapping, %d\n", GetLastError());*/
	    pgch->addr = NULL;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	    return mpi_errno;
	}
#else
#error No shared memory subsystem defined
#endif

	/* Get the shmem pointer */
#if defined(HAVE_WINDOWS_H) && defined(USE_SYNCHRONIZE_SHMAPPING)
	if (nRank == 0)
	{
	    ResetEvent(hSyncEvent2);
	    SetEvent(hSyncEvent1);
	    WaitForSingleObject(hSyncEvent2, INFINITE);
	}
	else
	{
	    WaitForSingleObject(hSyncEvent1, INFINITE);
	    ResetEvent(hSyncEvent1);
	    SetEvent(hSyncEvent2);
	}
#endif
	pgch->addr = NULL;
	MPIU_DBG_PRINTF(("[%d] mapping shared memory\n", nRank));
#ifdef USE_POSIX_SHM
	/* style: allow:mmap:1 sig:0 */
	pgch->addr = mmap(NULL, nTotalSize, PROT_READ | PROT_WRITE, MAP_SHARED /* | MAP_NORESERVE*/, pgch->id, 0);
	if (pgch->addr == MAP_FAILED)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**mmap", "**mmap %d", errno);
	    pgch->addr = NULL;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	    return mpi_errno;
	}
#elif defined (USE_SYSV_SHM)
	pgch->addr = shmat(pgch->id, NULL, SHM_RND);
	if (pgch->addr == (void*)-1)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmat", "**shmat %d", errno); /*"Error from shmat %d\n", errno);*/
	    pgch->addr = NULL;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	    return mpi_errno;
	}
#elif defined(USE_WINDOWS_SHM)
	pgch->addr = MapViewOfFileEx(
	    pgch->id,
	    FILE_MAP_WRITE,
	    0, 0,
	    nTotalSize,
	    NULL
	    );
	MPIU_DBG_PRINTF(("."));
	if (pgch->addr == NULL)
	{
	    /*MPIDI_err_printf("MPIDI_CH3I_SHM_Get_mem", "Error in MapViewOfFileEx, %d\n", GetLastError());*/
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**MapViewOfFileEx", 0);
	    pgch->addr = NULL;
	    return mpi_errno;
	}
#else
#error No shared memory subsystem defined
#endif
	MPIU_DBG_PRINTF(("\n[%d] finished mapping shared memory: addr:%x\n", nRank, pgch->addr));

#ifdef HAVE_SHARED_PROCESS_READ

	pgch->pSHP = (MPIDI_CH3I_Shared_process_t*)((char*)pgch->addr + shp_offset);
	InitSharedProcesses(pg, nRank);
#endif
    }
    else
    {
	pgch->addr = MPIU_Malloc(nTotalSize);
    }

    MPIU_DBG_PRINTF(("[%d] made it: shm address: %x\n", nRank, pgch->addr));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Release_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/*@
   MPIDI_CH3I_SHM_Release_mem - 

   Notes:
@*/
int MPIDI_CH3I_SHM_Release_mem(MPIDI_PG_t *pg, BOOL bUseShm)
{
    MPIDI_CH3I_PG *pgch = (MPIDI_CH3I_PG *)pg->channel_private;
#ifdef HAVE_SHARED_PROCESS_READ
    int i;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);
    
    if (bUseShm)
    {
#ifdef USE_POSIX_SHM
	close(pgch->id);
#elif defined (USE_SYSV_SHM)
        shmdt(pgch->addr);
#elif defined (USE_WINDOWS_SHM)
        UnmapViewOfFile(pgch->addr);
        pgch->addr = NULL;
        CloseHandle(pgch->id);
        pgch->id = NULL;
#else
#error No shared memory subsystem defined
#endif
#ifdef HAVE_SHARED_PROCESS_READ
#ifdef USE_WINDOWS_SHM
	for (i=0; i<MPIDI_PG_Get_size(pg); i++)
	    CloseHandle(pgch->pSharedProcessHandles[i]);
	MPIU_Free(pgch->pSharedProcessHandles);
	pgch->pSharedProcessHandles = NULL;
#else
	for (i=0; i<MPIDI_PG_Get_size(pg); i++)
	    close(pgch->pSharedProcessFileDescriptors[i]);
	MPIU_Free(pgch->pSharedProcessFileDescriptors);
	MPIU_Free(pgch->pSharedProcessIDs);
	pgch->pSharedProcessFileDescriptors = NULL;
	pgch->pSharedProcessIDs = NULL;
#endif
#endif
    }
    else
    {
        MPIU_Free(pgch->addr);
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);
    return MPI_SUCCESS;
}
