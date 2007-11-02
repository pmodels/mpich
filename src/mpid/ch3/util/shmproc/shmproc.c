/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* 
 * This file defines a few routines to allow one process to write to the 
 * memory of another process.  This makes use of a few special interfaces
 * provided by some operating systems.  For some Unix versions, ptrace
 * may be used.  Windows provides WriteProcessMemory etc
 * 
 */

#include "mpidi_ch3_impl.h"

#ifndef HAVE_WINDOWS_H
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef HAVE_SYS_PTRACE_H
#include <sys/ptrace.h>
#endif
#include <sys/wait.h>
#include <errno.h>
#define OFF_T off_t
#define OFF_T_CAST(a) ((off_t)(a))
#endif

#ifndef HAVE_WINDOWS_H
/* FIXME: Do we need these routines for all shmem or only for some options? */
/* Initialize for reading and writing to the designated process */
int MPIDI_SHM_InitRWProc( pid_t pid, int *fd )
{
    char filename[256];
    int mpi_errno = MPI_SUCCESS;

    MPIU_Snprintf(filename, sizeof(filename), "/proc/%d/mem", pid);
    *fd = open(filename, O_RDWR );
    if (*fd == -1) {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, 
			  FCNAME, __LINE__, MPI_ERR_OTHER, 
			  "**open", "**open %s %d %d", filename, pid, errno);
	return mpi_errno;
	
    }
    return mpi_errno;
}

#if defined(USE_RDMA_READV) || defined(USE_RDMA_WRITEV)
/* Call with vc->ch.nSharedProcessID 
 * This must be called to allow other operations
 */
int MPIDI_SHM_AttachProc( pid_t pid )
{
    int mpi_errno = MPI_SUCCESS;
    int status;

    if (ptrace(PTRACE_ATTACH, pid, 0, 0) != 0) {
	MPIU_ERR_SETANDJUMP2(mpi_errno,MPI_ERR_OTHER,"**fail", 
			     "**fail %s %d", "ptrace attach failed", errno);
    }
    if (waitpid(pid, &status, WUNTRACED) != pid) {
	MPIU_ERR_SETANDJUMP2(mpi_errno,MPI_ERR_OTHER, "**fail", 
			     "**fail %s %d", "waitpid failed", errno);
    }
 fn_fail:
    return mpi_errno;
}


/* Call with vc->ch.nSharedProcessID 
 * This should be called when access to the designated processes memory 
 * is no longer needed.  Use MPIDI_SHM_AttachProc to renew access to that
 * processes memory.
 */
int MPIDI_SHM_DetachProc( pid_t pid )
{
    int mpi_errno = MPI_SUCCESS;
    if (ptrace(PTRACE_DETACH, pid, 0, 0) != 0) {
	MPIU_ERR_SETANDJUMP2(mpi_errno,MPI_ERR_OTHER, "**fail", 
			     "**fail %s %d", "ptrace detach failed", errno);
    }
 fn_fail:
    return mpi_errno;
}

/* Read by seeking to the memory location on the file descriptor and then
   using read. */
int MPIDI_SHM_ReadProcessMemory( int fd, int pid, 
				 const char *source, char *dest, size_t len )
{
    off_t offset = OFF_T_CAST(source);
    off_t uOffset;
    int   num_read;
    int mpi_errno = MPI_SUCCESS;

    uOffset = lseek( fd, offset, SEEK_SET );
    if (uOffset != offset) {
	MPIU_ERR_SETANDJUMP2(mpi_errno,MPI_ERR_OTHER, "**fail", 
			    "**fail %s %d", "lseek failed", errno);
    }

    num_read = read( fd, dest, len );
    if (num_read < 1) {
	if (num_read == -1) {
	    MPIU_ERR_SETANDJUMP2(mpi_errno,MPI_ERR_OTHER, "**fail", 
				 "**fail %s %d", "read failed", errno);
	}
	/* If we only read part of the data, use ptrace to do what? */
	/* According to the man page on ptrace, this reads
	   a word (4 bytes) of memory at the location given by the third 
	   argument. This is use to force the page in place.
	*/
	ptrace( PTRACE_PEEKDATA, pid, source+len - num_read, 0 );
    }
    /* FIXME: Now what? Why not continue to read? */
 fn_fail:
    return mpi_errno;
}
#endif /* defined(USE_RDMA_READV) || defined(USE_RDMA_WRITEV) */

#else
/* HAVE_WINDOWS_H and use Windows interface */
/* Initialize for reading and writing to the designated process */
#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_InitRWProc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_SHM_InitRWProc( pid_t pid, int *phandle )
{
	int mpi_errno = MPI_SUCCESS;
    *phandle =
	OpenProcess(STANDARD_RIGHTS_REQUIRED | PROCESS_VM_READ | 
		    PROCESS_VM_WRITE | PROCESS_VM_OPERATION, 
		    FALSE, pid);
    if (*phandle == NULL) {
	int err = GetLastError();
	/* mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**OpenProcess", "**OpenProcess %d %d", info.pg_rank, err); */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**OpenProcess", "**OpenProcess %d %d", pid, err);
    }
    return mpi_errno;
}
#endif
