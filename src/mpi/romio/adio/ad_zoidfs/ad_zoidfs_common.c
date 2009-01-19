/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   Copyright (C) 2003 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_zoidfs.h"
#include "ad_zoidfs_common.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

/* keyval hack to both tell us if we've already initialized zoidfs and also
 * close it down when mpi exits */
int ADIOI_ZOIDFS_Initialized = MPI_KEYVAL_INVALID;

void ADIOI_ZOIDFS_End(int *error_code)
{
    int ret;
    static char myname[] = "ADIOI_ZOIDFS_END";

    ret = zoidfs_finalize();

    /* --BEGIN ERROR HANDLING-- */
    if (ret != 0 ) {
	*error_code = MPIO_Err_create_code(MPI_SUCCESS,
					   MPIR_ERR_RECOVERABLE,
					   myname, __LINE__,
					   ADIOI_ZOIDFS_error_convert(ret),
					   "Error in zoidfs_finalize", 0);
	return;
    }
    /* --END ERROR HANDLING-- */

    *error_code = MPI_SUCCESS;
}

int ADIOI_ZOIDFS_End_call(MPI_Comm comm, int keyval, 
			 void *attribute_val, void *extra_state)
{
    int error_code;
    ADIOI_ZOIDFS_End(&error_code);
    return error_code;
}

void ADIOI_ZOIDFS_Init(int rank, int *error_code )
{
    int ret;
    static char myname[] = "ADIOI_ZOIDFS_INIT";

    /* do nothing if we've already fired up the zoidfs interface */
    if (ADIOI_ZOIDFS_Initialized != MPI_KEYVAL_INVALID) {
	*error_code = MPI_SUCCESS;
	return;
    }

    ret = zoidfs_init();
    if (ret < 0 ) {
	*error_code = MPIO_Err_create_code(MPI_SUCCESS,
					   MPIR_ERR_RECOVERABLE,
					   myname, __LINE__,
					   ADIOI_ZOIDFS_error_convert(ret),
					   "Error in zoidfs_init",
					   0);
	return;
    }
    
    MPI_Keyval_create(MPI_NULL_COPY_FN, ADIOI_ZOIDFS_End_call,
		      &ADIOI_ZOIDFS_Initialized, (void *)0); 
    /* just like romio does, we make a dummy attribute so we 
     * get cleaned up */
    MPI_Attr_put(MPI_COMM_WORLD, ADIOI_ZOIDFS_Initialized, (void *)0);
}

void ADIOI_ZOIDFS_makeattribs(zoidfs_sattr_t * attribs)
{
    struct timeval now;

    memset(attribs, 0, sizeof(zoidfs_sattr_t));

    attribs->mask = ZOIDFS_ATTR_SETABLE;
    attribs->mode = 0644;
    attribs->uid = geteuid();
    attribs->gid = getegid(); 

    gettimeofday(&now, NULL);
    attribs->atime.seconds = now.tv_sec;
    attribs->atime.useconds = now.tv_usec;
    attribs->mtime.seconds = now.tv_sec;
    attribs->mtime.useconds = now.tv_usec;
}

int ADIOI_ZOIDFS_error_convert(int error)
{
    return MPI_UNDEFINED;
}

/* 
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */
