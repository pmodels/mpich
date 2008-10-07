/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#if defined (HAVE_SYSV_SHARED_MEM)
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

static int check_alloc(int num_processes);

typedef struct alloc_elem
{
    struct alloc_elem *next;
    void **ptr_p;
    size_t len;
} alloc_elem_t;

static struct { alloc_elem_t *head, *tail; } allocq = {0};

#define ALLOCQ_HEAD() GENERIC_Q_HEAD(allocq)
#define ALLOCQ_EMPTY() GENERIC_Q_EMPTY(allocq)
#define ALLOCQ_ENQUEUE(ep) GENERIC_Q_ENQUEUE(&allocq, ep, next)
#define ALLOCQ_DEQUEUE(epp) GENERIC_Q_DEQUEUE(&allocq, epp, next)

static size_t segment_len = 0;

/* MPIDI_CH3I_Seg_alloc(len, ptr_p)

   This function is used to allow the caller to reserve a len sized
   region in the shared memory segment.  Once the shared memory
   segment is actually allocated, when MPIDI_CH3I_Seg_commit() is
   called, the pointer *ptr_p will be set to point to the reserved
   region in the shared memory segment.

   Note that no shared memory is actually allocated by this function,
   and the *ptr_p pointer will be valid only after
   MPIDI_CH3I_Seg_commit() is called.
*/
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Seg_alloc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Seg_alloc(size_t len, void **ptr_p)
{
    int mpi_errno = MPI_SUCCESS;
    alloc_elem_t *ep;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEG_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SEG_ALLOC);

    MPIU_Assert(len);
    MPIU_Assert(ptr_p);

    MPIU_CHKPMEM_MALLOC(ep, alloc_elem_t *, sizeof(alloc_elem_t), mpi_errno, "el");
    
    ep->ptr_p = ptr_p;
    ep->len = len;

    ALLOCQ_ENQUEUE(ep);

    segment_len += len;
    
 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEG_ALLOC);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

/* MPIDI_CH3I_Seg_commit(memory, num_local, local_rank)

   This function allocates a shared memory segment large enough to
   hold all of the regions previously requested by calls to
   MPIDI_CH3I_Seg_alloc().  For each request, this function sets the
   associated pointer to point to the reserved region in the allocated
   shared memory segment.

   If there is only one process local to this node, then a shared
   memory region is not allocated.  Instead, memory is allocated from
   the heap.

   At least one call to MPIDI_CH3I_Seg_alloc() must be made before
   calling MPIDI_CH3I_Seg_commit().
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Seg_commit
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Seg_commit(MPID_nem_seg_ptr_t memory, int num_local, int local_rank)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int key_max_sz;
    int val_max_sz;
    char *key;
    char *val;
    char *kvs_name;
    char *handle = 0;
    void *current_addr;
    void *start_addr;
    size_t size_left;
    MPIU_CHKPMEM_DECL (1);
    MPIU_CHKLMEM_DECL (2);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEG_COMMIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SEG_COMMIT);

    /* MPIDI_CH3I_Seg_alloc() needs to have been called before this function */
    MPIU_Assert(!ALLOCQ_EMPTY());
    MPIU_Assert(segment_len > 0);
    
    memory->segment_len = segment_len;

    /* if there is only one process on this processor, don't use shared memory */
    if (num_local == 1)
    {
        char *addr;

        MPIU_CHKPMEM_MALLOC (addr, char *, segment_len + MPID_NEM_CACHE_LINE_LEN, mpi_errno, "segment");

        memory->base_addr = addr;
        current_addr = (char *)(((MPIR_Upint)addr + (MPIR_Upint)MPID_NEM_CACHE_LINE_LEN-1) & (~((MPIR_Upint)MPID_NEM_CACHE_LINE_LEN-1)));
        memory->symmetrical = 0 ;

        /* we still need two calls to barrier */
	pmi_errno = PMI_Barrier();
        MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
	pmi_errno = PMI_Barrier();
        MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);

        MPIU_CHKPMEM_COMMIT();
    }
    else
    {
        /* Allocate space for pmi key and val */
        pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
        MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
        MPIU_CHKLMEM_MALLOC(key, char *, key_max_sz, mpi_errno, "key");

        pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
        MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
        MPIU_CHKLMEM_MALLOC(val, char *, val_max_sz, mpi_errno, "val");

        mpi_errno = MPIDI_PG_GetConnKVSname (&kvs_name);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);

        if (local_rank == 0)
        {
            mpi_errno = MPID_nem_allocate_shared_memory (&memory->base_addr, segment_len, &handle);
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);

            /* post name of shared file */
            MPIU_Assert (MPID_nem_mem_region.local_procs[0] == MPID_nem_mem_region.rank);
            MPIU_Snprintf (key, key_max_sz, "sharedFilename[%i]", MPID_nem_mem_region.rank);

            pmi_errno = PMI_KVS_Put (kvs_name, key, handle);
            MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);

            pmi_errno = PMI_KVS_Commit (kvs_name);
            MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);

            pmi_errno = PMI_Barrier();
            MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
        }
        else
        {
            pmi_errno = PMI_Barrier();
            MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);

            /* get name of shared file */
            MPIU_Snprintf (key, key_max_sz, "sharedFilename[%i]", MPID_nem_mem_region.local_procs[0]);
            pmi_errno = PMI_KVS_Get (kvs_name, key, val, val_max_sz);
            MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", pmi_errno);

            handle = val;

            mpi_errno = MPID_nem_attach_shared_memory (&memory->base_addr, segment_len, handle);
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);
        }

        pmi_errno = PMI_Barrier();
        MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);

        if (local_rank == 0)
        {
            mpi_errno = MPID_nem_remove_shared_memory (handle);
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);
            MPIU_Free (handle);
        }

        current_addr = memory->base_addr;
        memory->symmetrical = 0 ;

    }
    
    /* assign sections of the shared memory segment to their pointers */

    start_addr = current_addr;
    size_left = segment_len;

    do
    {
        alloc_elem_t *ep;

        ALLOCQ_DEQUEUE(&ep);

        *(ep->ptr_p) = current_addr;
        size_left -= ep->len;
        current_addr = (char *)current_addr + ep->len;

        MPIU_Free(ep);

        MPIU_Assert(size_left >= 0);
        MPIU_Assert((char *)current_addr <= (char *)start_addr + segment_len);
    }
    while (!ALLOCQ_EMPTY());

    mpi_errno = check_alloc(num_local);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEG_COMMIT);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (handle)
        MPID_nem_remove_shared_memory (handle);
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* MPIDI_CH3I_Seg_destroy() free the shared memory segment */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Seg_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Seg_destroy()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEG_DESTROY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SEG_DESTROY);

    if (MPID_nem_mem_region.num_local == 1)
        MPIU_Free(MPID_nem_mem_region.memory.base_addr);
    else
    {
        mpi_errno = MPID_nem_detach_shared_memory (MPID_nem_mem_region.memory.base_addr, MPID_nem_mem_region.memory.segment_len);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEG_DESTROY);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* check_alloc() checks to see whether the shared memory segment is
   allocated at the same virtual memory address at each process.
*/
#undef FUNCNAME
#define FUNCNAME check_alloc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int check_alloc(int num_local)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int rank = MPID_nem_mem_region.local_rank;
    MPIR_Upint address = 0;
    int asym, index;
    MPIDI_STATE_DECL(MPID_STATE_CHECK_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_CHECK_ALLOC);

    pmi_errno = PMI_Barrier();
    MPIU_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);

    address = (MPIR_Upint)MPID_nem_mem_region.memory.base_addr;
    ((MPIR_Upint *)MPID_nem_mem_region.memory.base_addr)[rank] = address;
    
    pmi_errno = PMI_Barrier();
    MPIU_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);

    asym = 0;
    for (index = 0 ; index < num_local ; index ++)
        if (((MPIR_Upint *)MPID_nem_mem_region.memory.base_addr)[index] != address)
        {
            asym = 1;
            break;
        }
          
    pmi_errno = PMI_Barrier();
    MPIU_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);

    if (asym)
    {
	MPID_nem_mem_region.memory.symmetrical = 0;
	MPID_nem_asymm_base_addr = MPID_nem_mem_region.memory.base_addr;
#ifdef MPID_NEM_SYMMETRIC_QUEUES
        MPIU_ERR_SETFATALANDJUMP1(mpi_errno, MPI_ERR_INTERN, "**intern", "**intern %s", "queues are not symmetrically allocated as expected");
#endif
    }
    else
    {
	MPID_nem_mem_region.memory.symmetrical = 1;
	MPID_nem_asymm_base_addr = NULL;
    }	
      

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_CHECK_ALLOC);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#if defined (HAVE_SYSV_SHARED_MEM)
/* SYSV shared memory */

/* FIXME: for sysv, when we have more than 8 procs, we exceed SHMMAX
   when allocating a shared-memory region.  We need a way to handle
   this, e.g., break it up into smaller chunks, but make them
   contiguous. */
/* MPID_nem_allocate_shared_memory allocates a shared mem region of size "length" and attaches to it.  "handle" points to a string
   descriptor for the region to be passed in to MPID_nem_attach_shared_memory.  "handle" is dynamically allocated and should be
   freed by the caller.*/
#define MAX_INT_STR_LEN 12
#undef FUNCNAME
#define FUNCNAME MPID_nem_allocate_shared_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_allocate_shared_memory (char **buf_p, const int length, char *handle[])
{
    int mpi_errno = MPI_SUCCESS;
    int shmid;
    static int key = 0;
    void *buf;
    struct shmid_ds ds;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ALLOCATE_SHARED_MEMORY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ALLOCATE_SHARED_MEMORY);

    do
    {
        ++key;
        shmid = shmget (key, length, IPC_CREAT | IPC_EXCL | S_IRWXU);
    }
    while (shmid == -1 && errno == EEXIST);
    MPIU_ERR_CHKANDJUMP2 (shmid == -1, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem", "**alloc_shar_mem %s %s", "shmget", strerror (errno));

    buf = 0;
    buf = shmat (shmid, buf, 0);
    MPIU_ERR_CHKANDJUMP2 ((MPI_Aint)buf == -1, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem", "**alloc_shar_mem %s %s", "shmat", strerror (errno));

    *buf_p = buf;

    MPIU_CHKPMEM_MALLOC (*handle, char *, MAX_INT_STR_LEN, mpi_errno, "shared memory handle");
    MPIU_Snprintf (*handle, MAX_INT_STR_LEN, "%d", shmid);

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ALLOCATE_SHARED_MEMORY);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    shmctl (shmid, IPC_RMID, &ds);  /* try to remove region */
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* MPID_nem_attach_shared_memory attaches to shared memory previously allocated by MPID_nem_allocate_shared_memory */
/* MPID_nem_attach_shared_memory (char **buf_p, const int length, const char const handle[]) triggers a warning */
#undef FUNCNAME
#define FUNCNAME MPID_nem_attach_shared_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_attach_shared_memory (char **buf_p, const int length, const char handle[])
{
    int mpi_errno = MPI_SUCCESS;
    void *buf;
    int shmid;
    struct shmid_ds ds;
    char *endptr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ATTACH_SHARED_MEMORY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ATTACH_SHARED_MEMORY);

    shmid = strtoll (handle, &endptr, 10);
    MPIU_ERR_CHKANDJUMP2 (endptr == handle || *endptr != '\0', mpi_errno, MPI_ERR_OTHER, "**attach_shar_mem", "**attach_shar_mem %s %s", "strtoll", strerror (errno));

    buf = 0;
    buf = shmat (shmid, buf, 0);
    MPIU_ERR_CHKANDJUMP2 ((MPI_Aint)buf == -1, mpi_errno, MPI_ERR_OTHER, "**attach_shar_mem", "**attach_shar_mem %s %s", "shmat", strerror (errno));

    *buf_p = buf;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ATTACH_SHARED_MEMORY);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    shmctl (shmid, IPC_RMID, &ds); /* try to remove region */
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* MPID_nem_remove_shared_memory removes the OS descriptor associated with the handle.  Once all processes detatch from the region
   the OS resource will be destroyed. */
#undef FUNCNAME
#define FUNCNAME MPID_nem_remove_shared_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_remove_shared_memory (const char const handle[])
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    int shmid;
    struct shmid_ds ds;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_REMOVE_SHARED_MEMORY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_REMOVE_SHARED_MEMORY);

    shmid = atoi (handle);

    ret = shmctl (shmid, IPC_RMID, &ds);
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem", "**remove_shar_mem %s %s", "shmctl", strerror (errno));

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_REMOVE_SHARED_MEMORY);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    shmctl (shmid, IPC_RMID, &ds); /* try to remove region */
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* MPID_nem_detach_shared_memory detaches the shared memory region from this process */
#undef FUNCNAME
#define FUNCNAME MPID_nem_detach_shared_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_detach_shared_memory (const char *buf_p, const int length)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_DETACH_SHARED_MEMORY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_DETACH_SHARED_MEMORY);

    ret = shmdt (buf_p);
    /* I'm ignoring the return code here to work around an bug with
       gm-1 when a sysv shared memory region is registered. -db */
    /* MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**detach_shar_mem", "**detach_shar_mem %s %s", "shmdt", strerror (errno));*/

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_DETACH_SHARED_MEMORY);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#else /* HAVE_SYSV_SHARED_MEM */
/* Using memory mapped files */

#define MAX_INT_STR_LEN 12 /* chars needed to store largest integer including /0 */

/* MPID_nem_allocate_shared_memory allocates a shared mem region of size "length" and attaches to it.  "handle" points to a string
   descriptor for the region to be passed in to MPID_nem_attach_shared_memory.  "handle" is dynamically allocated and should be
   freed by the caller.*/
#undef FUNCNAME
#define FUNCNAME MPID_nem_allocate_shared_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_allocate_shared_memory (char **buf_p, const int length, char *handle[])
{
    int mpi_errno = MPI_SUCCESS;
    int fd;
    int ret;
    const char dev_fname[] = "/dev/shm/nemesis_shar_tmpXXXXXX";
    const char tmp_fname[] = "/tmp/nemesis_shar_tmpXXXXXX";
    MPIU_CHKPMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ALLOCATE_SHARED_MEMORY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ALLOCATE_SHARED_MEMORY);

    /* create a file */
    /* use /dev/shm if it's there, otherwise put file in /tmp */
    MPIU_CHKPMEM_MALLOC (*handle, char *, sizeof (dev_fname), mpi_errno, "shared memory handle");
    memcpy (*handle, dev_fname, sizeof (dev_fname));
    fd = mkstemp (*handle);
    if (fd == -1)
    {
	/* creating in /dev/shm failed, fall back to /tmp.  If that doesn't work, give up. */
	MPIU_Free (*handle);
	MPIU_CHKPMEM_MALLOC (*handle, char *, sizeof (tmp_fname), mpi_errno, "shared memory handle");
	memcpy (*handle, tmp_fname, sizeof (tmp_fname));
	fd = mkstemp (*handle);
	MPIU_ERR_CHKANDJUMP2 (fd == -1, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem", "**alloc_shar_mem %s %s", "mkstmp", strerror (errno));
    }

    /* set file to "length" bytes */
    ret = lseek (fd, length-1, SEEK_SET);
    MPIU_ERR_CHKANDSTMT2 (ret == -1, mpi_errno, MPI_ERR_OTHER, goto fn_close_fail, "**alloc_shar_mem", "**alloc_shar_mem %s %s", "lseek", strerror (errno));
    do
    {
        ret = write (fd, "", 1);
    }
    while ((ret == -1 && errno == EINTR) || ret == 0);
    MPIU_ERR_CHKANDSTMT2 (ret == -1, mpi_errno, MPI_ERR_OTHER, goto fn_close_fail, "**alloc_shar_mem", "**alloc_shar_mem %s %s", "lseek", strerror (errno));

    /* mmap the file */
    *buf_p = mmap (NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    MPIU_ERR_CHKANDSTMT2 (*buf_p == MAP_FAILED, mpi_errno, MPI_ERR_OTHER, goto fn_close_fail, "**alloc_shar_mem", "**alloc_shar_mem %s %s", "mmap", strerror (errno));

    /* close the file */
    do
    {
        ret = close (fd);
    }
    while (ret == -1 && errno == EINTR);
    MPIU_ERR_CHKANDSTMT2 (ret == -1, mpi_errno, MPI_ERR_OTHER, goto fn_close_fail, "**alloc_shar_mem", "**alloc_shar_mem %s %s", "close", strerror (errno));

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ALLOCATE_SHARED_MEMORY);
    return mpi_errno;
 fn_close_fail:
    close (fd);
    unlink (*handle);
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* MPID_nem_attach_shared_memory attaches to shared memory previously allocated by MPID_nem_allocate_shared_memory */
/* MPID_nem_attach_shared_memory (char **buf_p, const int length, const char const handle[]) make a warning */
#undef FUNCNAME
#define FUNCNAME MPID_nem_attach_shared_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_attach_shared_memory (char **buf_p, const int length, const char handle[])
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    int fd;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ATTACH_SHARED_MEMORY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ATTACH_SHARED_MEMORY);

    fd = open (handle, O_RDWR);
    MPIU_ERR_CHKANDJUMP2 (fd == -1, mpi_errno, MPI_ERR_OTHER,  "**attach_shar_mem", "**attach_shar_mem %s %s", "open", strerror (errno));

     /* mmap the file */
    *buf_p = mmap (NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    MPIU_ERR_CHKANDSTMT2 (*buf_p == MAP_FAILED, mpi_errno, MPI_ERR_OTHER, goto fn_close_fail, "**attach_shar_mem", "**attach_shar_mem %s %s", "open", strerror (errno));

    /* close the file */
    do
    {
        ret = close (fd);
    }
    while (ret == -1 && errno == EINTR);
    MPIU_ERR_CHKANDSTMT2 (ret == -1, mpi_errno, MPI_ERR_OTHER, goto fn_close_fail, "**attach_shar_mem", "**attach_shar_mem %s %s", "close", strerror (errno));

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ATTACH_SHARED_MEMORY);
    return mpi_errno;
 fn_close_fail:
    close (fd);
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    unlink (handle);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* MPID_nem_remove_shared_memory removes the OS descriptor associated with the
   handle.  Once all processes detatch from the region
   the OS resource will be destroyed. */
#undef FUNCNAME
#define FUNCNAME MPID_nem_remove_shared_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_remove_shared_memory (const char handle[])
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_REMOVE_SHARED_MEMORY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_REMOVE_SHARED_MEMORY);

    ret = unlink (handle);
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER,  "**remove_shar_mem", "**remove_shar_mem %s %s", "unlink", strerror (errno));

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_REMOVE_SHARED_MEMORY);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* MPID_nem_detach_shared_memory detaches the shared memory region from this
   process */
#undef FUNCNAME
#define FUNCNAME MPID_nem_detach_shared_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_detach_shared_memory (const char *buf_p, const int length)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_DETACH_SHARED_MEMORY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_DETACH_SHARED_MEMORY);

    ret = munmap ((void *)buf_p, length);
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER,  "**detach_shar_mem", "**detach_shar_mem %s %s", "munmap", strerror (errno));

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_DETACH_SHARED_MEMORY);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#endif /* HAVE_SYSV_SHARED_MEM */
