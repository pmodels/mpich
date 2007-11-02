/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* We use the proper autoconf test for mmap, so we let the coding style
   checker know  (actually, not quite, since the autoconf test doesn't 
   check that mmap works in the case in which we use it).  */
/* style: allow:mmap:3 sig:0 */
#include "mpidi_ch3_impl.h"

/* 
 * We support several ways to get and use shared memory.  To make the code
 * clearer, we divide the routines into ones that depend on which method
 * is chosen (e.g., POSIX_SHM) and routines that are independed of the method.
 * This will aid in uncovering the source of storage leaks.
 * 
 * This code uses a big #ifdef...#elif branch to separate out the code.
 * An even better approach, left for later, is to put these into separate 
 * files, making it easier to understand what code is in use.  Each
 * branch of this ifdef implements the same routines.  
 * 
 * One routine, get_mem_named, has not yet been moved within the branch.
 */

#ifdef USE_POSIX_SHM

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Get_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/*@
   MPIDI_CH3I_SHM_Get_mem - allocate and get the address and size of a 
   shared memory block

   Parameters:
+  int size - size
-  MPIDI_CH3I_Shmem_block_request_result* pOutput - output

   Notes:
@*/

int MPIDI_CH3I_SHM_Get_mem(int size, 
			   MPIDI_CH3I_Shmem_block_request_result *pOutput)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);

    if (size == 0 || size > MPIDU_MAX_SHM_BLOCK_SIZE ) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**arg");
    }

    /* Create the shared memory object */
    /* Generate_shm_string generates a unique and different string 
     each time.  We try 10 times to succeed in creating an unused filename 
     for shm_open.  */
    for (i=0; i<10; i++) {
	MPIDI_Generate_shm_string(pOutput->key,sizeof(pOutput->key));
	pOutput->id = shm_open(pOutput->key, O_EXCL | O_RDWR | O_CREAT, 0600);
	if (pOutput->id != -1)
	    break;
    }
    if (pOutput->id == -1) {
	pOutput->error = errno;
	perror("shm_open error");
	MPIU_ERR_SETANDJUMP2(mpi_errno,MPI_ERR_OTHER, 
	       "**shm_open", "**shm_open %s %d", pOutput->key, pOutput->error);
    }
    MPIU_Strncpy(pOutput->name, pOutput->key, MPIDI_MAX_SHM_NAME_LENGTH);
    if (ftruncate(pOutput->id, size) == -1) {
	MPIU_ERR_SETANDJUMP3(mpi_errno,MPI_ERR_OTHER, "**ftruncate", 
		 "**ftruncate %s %d %d", pOutput->key, size, pOutput->error);
    }

    pOutput->addr = mmap(NULL, size, PROT_READ | PROT_WRITE, 
			 MAP_SHARED /* | MAP_NORESERVE*/, pOutput->id, 0);
    if (pOutput->addr == MAP_FAILED) {
	pOutput->addr = NULL;
	perror("mmap failed");
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, 
			     "**mmap", "**mmap %d", errno);
    }

    pOutput->size = size;
    pOutput->error = MPI_SUCCESS;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Attach_to_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/*@
   MPIDI_CH3I_SHM_Attach_to_mem - attach to an existing shmem queue

   Parameters:
+  MPIDI_CH3I_Shmem_block_request_result* pInput - input
-  MPIDI_CH3I_Shmem_block_request_result* pOutput - output

   Notes:
   The shared memory segment is unlinked after attaching so only one process 
   can call this function.
@*/
int MPIDI_CH3I_SHM_Attach_to_mem(MPIDI_CH3I_Shmem_block_request_result *pInput,
				MPIDI_CH3I_Shmem_block_request_result *pOutput)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);

    /* Create the shared memory object */
    pOutput->id = shm_open(pInput->key, O_RDWR, 0600);
    if (pOutput->id == -1)
    {
	pOutput->error = errno;
	/* printf("shm_open(%s) failed, error %d\n", pInput->key, errno); */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shm_open", "**shm_open %s %d", pInput->key, errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
	return mpi_errno;
    }
    /*ftruncate(pOutput->id, size);*/ /* The sender/creator set the size */

    pOutput->addr = NULL;
    pOutput->addr = mmap(NULL, pInput->size, PROT_READ | PROT_WRITE, MAP_SHARED /* | MAP_NORESERVE*/, pOutput->id, 0);
    if (pOutput->addr == MAP_FAILED)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**mmap", "**mmap %d", errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
	return mpi_errno;
    }
    shm_unlink(pInput->key);

    pOutput->error = MPI_SUCCESS;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
    return MPI_SUCCESS;
}

/*@
   MPIDI_CH3I_SHM_Unlink_mem - 

   Notes:
@*/
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Unlink_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_Unlink_mem(MPIDI_CH3I_Shmem_block_request_result *p)
{
    int ret_val;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_UNLINK_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_UNLINK_MEM);

    /* printf("[%d] unlinking '%s'\n", MPIR_Process.comm_world->rank, p->name); */
    ret_val = shm_unlink(p->name);
    if (ret_val == -1)
    {
#ifdef ENOENT
	if (errno != ENOENT)
#endif
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shm_unlink", "**shm_unlink %s %d", p->key, ret_val);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_UNLINK_MEM);
	return mpi_errno;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_UNLINK_MEM);
    return mpi_errno;
}

/*@
   MPIDI_CH3I_SHM_Release_mem - 

   Notes:
@*/
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Release_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_Release_mem(MPIDI_CH3I_Shmem_block_request_result *p)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);

    close(p->id);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);
    return MPI_SUCCESS;
}
/* END OF POSIX_SHM */

#elif defined(USE_SYSV_SHM)
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Get_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/* MPIDI_CH3I_SHM_Get_mem - allocate and get the address and size of a 
   shared memory block */
int MPIDI_CH3I_SHM_Get_mem(int size, 
			   MPIDI_CH3I_Shmem_block_request_result *pOutput)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);

    if (size == 0 || size > MPIDU_MAX_SHM_BLOCK_SIZE ) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**arg");
    }

    /* Create the shared memory object */
    srand(getpid());
    for (i=0; i<10; i++)
    {
	int shmflag = IPC_EXCL | IPC_CREAT;
#ifdef HAVE_SHM_RW
	/* Not all systems support the SHM_R and W flags.  Add them 
	   if we can */
	shmflag |= SHM_R | SHM_W;
#endif	
	pOutput->key = rand();
	pOutput->id = shmget(pOutput->key, size, shmflag );
	if (pOutput->id != -1)
	    break;
    }
    if (pOutput->id == -1) {
	pOutput->error = errno;
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**shmget", 
			     "**shmget %d", pOutput->error);
    }

    pOutput->addr = NULL;
    pOutput->addr = shmat(pOutput->id, NULL, SHM_RND);
    if (pOutput->addr == (void*)-1) {
	pOutput->error = errno;
	pOutput->addr = NULL;
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, 
			     "**shmat", "**shmat %d", pOutput->error); 
    }

    pOutput->size = size;
    pOutput->error = MPI_SUCCESS;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
    return MPI_SUCCESS;
 fn_fail:
    goto fn_exit;
}
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Attach_to_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/*   MPIDI_CH3I_SHM_Attach_to_mem - attach to an existing shmem queue
*/
int MPIDI_CH3I_SHM_Attach_to_mem(MPIDI_CH3I_Shmem_block_request_result *pInput,
                                MPIDI_CH3I_Shmem_block_request_result *pOutput)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);

    /* Create the shared memory object */
    {
	int shmflag = 0;
#ifdef HAVE_SHM_RW
	shmflag |= SHM_R | SHM_W;
#endif	
	pOutput->id = shmget(pInput->key, pInput->size, shmflag );
    }
    if (pOutput->id == -1)
    {
	pOutput->error = errno;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmget", "**shmget %d", pOutput->error); /*"Error in shmget, %d", pOutput->error);*/
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
	return mpi_errno;
    }

    pOutput->addr = NULL;
    pOutput->addr = shmat(pOutput->id, NULL, SHM_RND);
    if (pOutput->addr == (void*)-1)
    {
	pOutput->error = errno;
	pOutput->addr = NULL;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmat", "**shmat %d", pOutput->error); /*"Error from shmat %d", pOutput->error);*/
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
	return mpi_errno;
    }
    shmctl(pOutput->id, IPC_RMID, NULL);

    pOutput->error = MPI_SUCCESS;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Unlink_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_Unlink_mem(MPIDI_CH3I_Shmem_block_request_result *p)
{
    int ret_val;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_UNLINK_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_UNLINK_MEM);

    ret_val = shmctl(p->id, IPC_RMID, NULL);
    if (ret_val == -1)
    {
#ifdef EIDRM
	if (errno != EIDRM)
#endif
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmctl", "**shmctl %d %d", p->id, ret_val);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_UNLINK_MEM);
	return mpi_errno;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_UNLINK_MEM);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Release_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_Release_mem(MPIDI_CH3I_Shmem_block_request_result *p)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);

    shmdt(p->addr);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);
    return MPI_SUCCESS;
}
/* END OF SVSY_SHM */

#elif defined(USE_WINDOWS_SHM)
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Get_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/* MPIDI_CH3I_SHM_Get_mem - allocate and get the address and size of a 
   shared memory block */
int MPIDI_CH3I_SHM_Get_mem(int size, 
			   MPIDI_CH3I_Shmem_block_request_result *pOutput)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);

    if (size == 0 || size > MPIDU_MAX_SHM_BLOCK_SIZE ) {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	return mpi_errno;
    }

    /* Create the shared memory object */
    MPIDI_Generate_shm_string(pOutput->key,sizeof(pOutput->key));
    pOutput->id = CreateFileMapping(
	INVALID_HANDLE_VALUE,
	NULL,
	PAGE_READWRITE,
	0, 
	size,
	pOutput->key);
    if (pOutput->id == NULL) 
    {
	pOutput->error = GetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**CreateFileMapping", "**CreateFileMapping %d", pOutput->error); /*"Error in CreateFileMapping, %d", pOutput->error);*/
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	return mpi_errno;
    }

    pOutput->addr = NULL;
    pOutput->addr = MapViewOfFileEx(
	pOutput->id,
	FILE_MAP_WRITE,
	0, 0,
	size,
	NULL
	);
    if (pOutput->addr == NULL)
    {
	pOutput->error = GetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**MapViewOfFileEx", "**MapViewOfFileEx %d", pOutput->error);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	return mpi_errno;
    }

    pOutput->size = size;
    pOutput->error = MPI_SUCCESS;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
    return MPI_SUCCESS;
}
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Attach_to_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/*
   MPIDI_CH3I_SHM_Attach_to_mem - attach to an existing shmem queue
*/
int MPIDI_CH3I_SHM_Attach_to_mem(MPIDI_CH3I_Shmem_block_request_result *pInput, MPIDI_CH3I_Shmem_block_request_result *pOutput)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);

    /* Create the shared memory object */
    MPIU_DBG_PRINTF(("MPIDI_CH3I_SHM_Attach_to_mem: Creating file mapping of size %d named %s\n", pInput->size, pInput->key));
    pOutput->id = CreateFileMapping(
	INVALID_HANDLE_VALUE,
	NULL,
	PAGE_READWRITE,
	0, 
	pInput->size,
	pInput->key);
    if (pOutput->id == NULL) 
    {
	pOutput->error = GetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**CreateFileMapping", "**CreateFileMapping %d", pOutput->error); /*"Error in CreateFileMapping, %d", pOutput->error);*/
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
	return mpi_errno;
    }

    pOutput->addr = NULL;
    pOutput->addr = MapViewOfFileEx(
	pOutput->id,
	FILE_MAP_WRITE,
	0, 0,
	pInput->size,
	NULL
	);
    if (pOutput->addr == NULL)
    {
	pOutput->error = GetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**MapViewOfFileEx", "**MapViewOfFileEx %d", pOutput->error);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
	return mpi_errno;
    }

    pOutput->error = MPI_SUCCESS;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Unlink_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_Unlink_mem(MPIDI_CH3I_Shmem_block_request_result *p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_UNLINK_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_UNLINK_MEM);

    /* Windows requires no special handling */

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_UNLINK_MEM);
    return mpi_errno;
}

/*@
   MPIDI_CH3I_SHM_Release_mem - 

   Notes:
@*/
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Release_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_Release_mem(MPIDI_CH3I_Shmem_block_request_result *p)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);

    UnmapViewOfFile(p->addr);
    CloseHandle(p->id);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);
    return MPI_SUCCESS;
}
/* END OF WINDOWS_SHM */

#else
#error No shared memory subsystem defined
#endif

