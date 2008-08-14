/* -*- Mode: C; c-basic-offset:4 ; -*-
 * vim: ts=8 sts=4 sw=4 noexpandtab
 *
 *   Copyright (C) 2007 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_zoidfs.h"
#include "ad_zoidfs_common.h"

/* open_status is helpful for bcasting values around */
struct open_status_s {
    int error;
    zoidfs_handle_t handle;
};
typedef struct open_status_s open_status;
    
static void fake_an_open(char *fname, int access_mode,
	                 int nr_datafiles, MPI_Offset strip_size,
                         ADIOI_ZOIDFS_object *zoidfs_ptr, 
			 open_status *o_status)
{
    int ret, created;
    zoidfs_sattr_t attribs;
    zoidfs_handle_t handle;

    ADIOI_ZOIDFS_makeattribs(&attribs);

    /* similar to PVFS v2: 
     *	- lookup the file
     *	- if CREATE flag passed in 
     *	    - create the file if CREATE flag passed in
     *	    - if *that* fails, lookup file again
     *	- else return error
     */
    ret = zoidfs_lookup(NULL, NULL, fname, &handle);
    if (ret == ZFSERR_NOENT) {
	/* file does not exist. create it, but only if given apropriate flag */
	if (access_mode & ADIO_CREATE) {
	    ret = zoidfs_create(NULL, NULL, 
		    fname, &attribs, &handle, &created);
	    if (ret == ZFSERR_EXIST) {
		/* could have case where many independent processes are trying
		 * to create the same file.  At this point another  process
		 * could have created the file between our lookup and create
		 * calls.  If so, just look up the file */
		ret = zoidfs_lookup(NULL, NULL, fname, &handle);
		if (ret != ZFS_OK) {
		    o_status->error = ret;
		    return;
		}
	    }
	}
    } else if ((ret == ZFS_OK) && (access_mode & ADIO_EXCL)) {
	/* lookup should not successd if opened with EXCL */
	o_status->error = ZFSERR_EXIST;
	return;
    }

    o_status->error = ret;
    o_status->handle = handle;
    return;
}


/* ADIOI_ZOIDFS_Open:
 *  one process opens (or creates) the file, then broadcasts the result to the
 *  remaining processors. 
 *
 * ADIO_Open used to perform an optimization when MPI_MODE_CREATE (and before
 * that, MPI_MODE_EXCL) was set.  Because ZoidFS handles file lookup and
 * creation more scalably than traditional file systems, ADIO_Open now skips any
 * special handling when CREATE is set.  */
void ADIOI_ZOIDFS_Open(ADIO_File fd, int *error_code)
{
    int rank, ret;
    static char myname[] = "ADIOI_ZOIDFS_OPEN";
    ADIOI_ZOIDFS_object *zoidfs_obj_ptr;

    /* since one process is doing the open, that means one process is also
     * doing the error checking.  define a struct for both the object reference
     * and the error code to broadcast to all the processors */

    open_status o_status = {0, 0}; 
    MPI_Datatype open_status_type;
    MPI_Datatype types[2] = {MPI_INT, MPI_BYTE};
    int lens[2] = {1, sizeof(ADIOI_ZOIDFS_object)};
    MPI_Aint offsets[2];
    
    zoidfs_obj_ptr = (ADIOI_ZOIDFS_object *) 
	ADIOI_Malloc(sizeof(ADIOI_ZOIDFS_object));
    /* --BEGIN ERROR HANDLING-- */
    if (zoidfs_obj_ptr == NULL) {
	*error_code = MPIO_Err_create_code(MPI_SUCCESS,
					   MPIR_ERR_RECOVERABLE,
					   myname, __LINE__,
					   MPI_ERR_UNKNOWN,
					   "Error allocating memory", 0);
	return;
    }
    /* --END ERROR HANDLING-- */

    MPI_Comm_rank(fd->comm, &rank);

    ADIOI_ZOIDFS_Init(rank, error_code);
    if (*error_code != MPI_SUCCESS)
    {
	/* ADIOI_ZOIDFS_INIT handles creating error codes on its own */
	return;
    }

    /* one process resolves name and will later bcast to others */
#ifdef ADIOI_MPE_LOGGING
    MPE_Log_event( ADIOI_MPE_open_a, 0, NULL );
#endif
    if (rank == fd->hints->ranklist[0] && fd->fs_ptr == NULL) {
	    fake_an_open(fd->filename, fd->access_mode, 
		    fd->hints->striping_factor,
		    fd->hints->striping_unit,
		    zoidfs_obj_ptr, &o_status); 
	    /* store credentials and object reference in fd */
	    *zoidfs_obj_ptr = o_status.handle;
	    fd->fs_ptr = zoidfs_obj_ptr;
    }
#ifdef ADIOI_MPE_LOGGING
    MPE_Log_event( ADIOI_MPE_open_b, 0, NULL );
#endif

    /* broadcast status and (possibly valid) object reference */
    MPI_Address(&o_status.error, &offsets[0]);
    MPI_Address(&o_status.handle, &offsets[1]);

    MPI_Type_struct(2, lens, offsets, types, &open_status_type);
    MPI_Type_commit(&open_status_type);

    /* Assertion: if we hit this Bcast, then all processes collectively
     *            called this open.
     *
     * That's because deferred open never happens with this fs.
     */
    MPI_Bcast(MPI_BOTTOM, 1, open_status_type, fd->hints->ranklist[0],
	      fd->comm);
    MPI_Type_free(&open_status_type);

    /* --BEGIN ERROR HANDLING-- */
    if (o_status.error != ZFS_OK)
    { 
	ADIOI_Free(zoidfs_obj_ptr);
	fd->fs_ptr = NULL;
	*error_code = MPIO_Err_create_code(MPI_SUCCESS,
					   MPIR_ERR_RECOVERABLE,
					   myname, __LINE__,
					   ADIOI_ZOIDFS_error_convert(o_status.error),
					   "Unknown error", 0);
	/* TODO: FIX STRING */
	return;
    }
    /* --END ERROR HANDLING-- */

    *zoidfs_obj_ptr = o_status.handle;
    fd->fs_ptr = zoidfs_obj_ptr;

    *error_code = MPI_SUCCESS;
    return;
}
