/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "shmimpl.h"

#ifdef WITH_METHOD_SHM

/* prototypes */
void *shm_alloc(unsigned int);
void shm_free(void *);
void *shm_get_mem_sync(int, int, int);
void shm_release_mem(void);

/*@
   shm_alloc - allocate shared memory

   Parameters:
+  unsigned int size - size

   Notes:
@*/
void *shm_alloc(unsigned int size)
{
    MPIDI_STATE_DECL(MPID_STATE_SHM_ALLOC);
    MPIDI_FUNC_ENTER(MPID_STATE_SHM_ALLOC);
    MPIDI_FUNC_EXIT(MPID_STATE_SHM_ALLOC);
    return NULL;
}

/*@
   shm_free - free shared memory

   Parameters:
+  void *address - address

   Notes:
@*/
void shm_free(void *address)
{
    MPIDI_STATE_DECL(MPID_STATE_SHM_FREE);
    MPIDI_FUNC_ENTER(MPID_STATE_SHM_FREE);
    MPIDI_FUNC_EXIT(MPID_STATE_SHM_FREE);
}

/*@
   shm_get_mem_sync - allocate and get address and size of memory shared by all processes. 

   Parameters:
+  int nTotalSize - total size
.  int nRank - rank
-  int nNproc - number of processes

   Notes:
    Set the global variables MPIDI_Shm_addr, MPIDI_Shm_size, MPIDI_Shm_id
    Ensure that MPIDI_Shm_addr is the same across all processes that share memory.
@*/
void *shm_get_mem_sync(int nTotalSize, int nRank, int nNproc)
{
    MPIDI_STATE_DECL(MPID_STATE_SHM_GET_MEM_SYNC);
    MPIDI_FUNC_ENTER(MPID_STATE_SHM_GET_MEM_SYNC);
    MPIDI_FUNC_EXIT(MPID_STATE_SHM_GET_MEM_SYNC);
    return NULL;
}

/*@
   shm_release_mem - release shared memory pool

   Notes:
@*/
void shm_release_mem(void)
{
    MPIDI_STATE_DECL(MPID_STATE_SHM_RELEASE_MEM);
    MPIDI_FUNC_ENTER(MPID_STATE_SHM_RELEASE_MEM);
    MPIDI_FUNC_EXIT(MPID_STATE_SHM_RELEASE_MEM);
}

#endif
