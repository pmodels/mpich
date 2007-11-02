/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

/*
 * MPIDI_CH3_Comm_spawn_multiple()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Comm_spawn_multiple
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Comm_spawn_multiple(int count, char **commands, 
                                  char ***argvs, int *maxprocs, 
                                  MPID_Info **info_ptrs, int root,
                                  MPID_Comm *comm_ptr, MPID_Comm
                                  **intercomm, int *errcodes) 
{
    char port_name[MPI_MAX_PORT_NAME];
    int *info_keyval_sizes, i, mpi_errno=MPI_SUCCESS;
    PMI_keyval_t **info_keyval_vectors, preput_keyval_vector;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_COMM_SPAWN_MULTIPLE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_COMM_SPAWN_MULTIPLE);
    if (comm_ptr->rank == root)
    {
        info_keyval_sizes = (int *) MPIU_Malloc(count * sizeof(int));
        /* FIXME: TEMPORARILY set all user-provided info to NULL. PMI is not
           using it anyway. */
        for (i=0; i<count; i++) info_keyval_sizes[i] = 0;
        info_keyval_vectors = NULL;

        mpi_errno = MPID_Open_port(port_name, 0);
        if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "Unable to open a port for the spawned processes to connect to");
	    goto fn_exit;
	}
        
        preput_keyval_vector.key = "PARENT_ROOT_PORT_NAME";
        preput_keyval_vector.val = port_name;

        mpi_errno = PMI_Spawn_multiple(count, (const char **)
                                       commands, 
                                       (const char ***) argvs,
                                       maxprocs, info_keyval_sizes,
                                       (const PMI_keyval_t **)
                                       info_keyval_vectors, 1, 
                                       &preput_keyval_vector,
                                       errcodes);

        if (mpi_errno != 0)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_spawn_multiple", "**pmi_spawn_multiple %d", mpi_errno);
            goto fn_exit;
        }

        MPIU_Free(info_keyval_sizes);
    }

    mpi_errno = MPIDI_CH3_Comm_accept(port_name, root, comm_ptr, intercomm); 
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "Unable to accept a connection from the spawned processes");
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_COMM_SPAWN_MULTIPLE);
    return mpi_errno;
}
