/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidi_ch3_impl.h"

/* FIXME:
   Place all of this within the mpid_comm_spawn_multiple file */

#ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS

/* Define the name of the kvs key used to provide the port name to the
   children */
#define PARENT_PORT_KVSKEY "PARENT_ROOT_PORT_NAME"

/*
 * MPIDI_CH3_Comm_spawn_multiple()
 */
int MPIDI_Comm_spawn_multiple(int count, char **commands,
                                  char ***argvs, const int *maxprocs,
                                  MPIR_Info **info_ptrs, int root,
                                  MPIR_Comm *comm_ptr, MPIR_Comm
                                  **intercomm, int *errcodes) 
{
    char port_name[MPI_MAX_PORT_NAME];
    int i, mpi_errno=MPI_SUCCESS;
    int *pmi_errcodes = 0;
    int total_num_processes, should_accept = 1;

    MPIR_FUNC_ENTER;

    if (comm_ptr->rank == root) {
	/* create an array for the pmi error codes */
	total_num_processes = 0;
	for (i=0; i<count; i++) {
	    total_num_processes += maxprocs[i];
	}
	pmi_errcodes = (int*)MPL_malloc(sizeof(int) * total_num_processes, MPL_MEM_DYNAMIC);
	if (pmi_errcodes == NULL) {
	    MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
	}

	/* initialize them to 0 */
	for (i=0; i<total_num_processes; i++)
	    pmi_errcodes[i] = 0;

	/* Open a port for the spawned processes to connect to */
	/* FIXME: info may be needed for port name */
        mpi_errno = MPID_Open_port(NULL, port_name);
	/* --BEGIN ERROR HANDLING-- */
        MPIR_ERR_CHECK(mpi_errno);
	/* --END ERROR HANDLING-- */

	/* Spawn the processes */
        MPIR_PMI_KEYVAL_t preput;
        preput.key = PARENT_PORT_KVSKEY;
        preput.val = port_name;

        mpi_errno = MPIR_pmi_spawn_multiple(count, commands, argvs, maxprocs, info_ptrs, 1, &preput, pmi_errcodes);
        MPIR_ERR_CHECK(mpi_errno);

	if (errcodes != MPI_ERRCODES_IGNORE) {
	    for (i=0; i<total_num_processes; i++) {
		/* FIXME: translate the pmi error codes here */
		errcodes[i] = pmi_errcodes[i];
                /* We want to accept if any of the spawns succeeded.
                   Alternatively, this is the same as we want to NOT accept if
                   all of them failed.  should_accept = NAND(e_0, ..., e_n)
                   Remember, success equals false (0). */
                should_accept = should_accept && errcodes[i];
	    }
            should_accept = !should_accept; /* the `N' in NAND */
	}
    }

    if (errcodes != MPI_ERRCODES_IGNORE) {
        MPIR_Errflag_t errflag = MPIR_ERR_NONE;
        mpi_errno = MPIR_Bcast(&should_accept, 1, MPI_INT, root, comm_ptr, &errflag);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Bcast(&total_num_processes, 1, MPI_INT, root, comm_ptr, &errflag);
        MPIR_ERR_CHECK(mpi_errno);
        
        mpi_errno = MPIR_Bcast(errcodes, total_num_processes, MPI_INT, root, comm_ptr, &errflag);
        MPIR_ERR_CHECK(mpi_errno);

        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    }

    if (should_accept) {
        mpi_errno = MPID_Comm_accept(port_name, NULL, root, comm_ptr, intercomm); 
        MPIR_ERR_CHECK(mpi_errno);
    }
    else {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**pmi_spawn_multiple");
    }

    if (comm_ptr->rank == root) {
	/* Close the port opened for the spawned processes to connect to */
	mpi_errno = MPID_Close_port(port_name);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIR_ERR_POP(mpi_errno);
	}
	/* --END ERROR HANDLING-- */
    }

 fn_exit:
    MPL_free(pmi_errcodes);
    MPIR_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* FIXME: What does this function do?  Who calls it?  Can we assume that
   it is called only dynamic process operations (specifically spawn) 
   are supported?  Do we need the concept of a port? For example,
   could a channel that supported only shared memory call this (it doesn't
   look like it right now, so this could go into util/sock, perhaps?
   
   It might make more sense to have this function provided as a function 
   pointer as part of the channel init setup, particularly since this
   function appears to access channel-specific storage (MPIDI_CH3_Process) */


/* This function is used only with mpid_init to set up the parent communicator
   if there is one.  The routine should be in this file because the parent 
   port name is setup with the "preput" arguments to PMI_Spawn_multiple */
static char *parent_port_name = 0;    /* Name of parent port if this
					 process was spawned (and is root
					 of comm world) or null */

int MPIDI_CH3_GetParentPort(char ** parent_port)
{
    int mpi_errno = MPI_SUCCESS;
    char val[MPIDI_MAX_KVS_VALUE_LEN];

    if (parent_port_name == NULL) {
        mpi_errno = MPIR_pmi_kvs_get(-1, PARENT_PORT_KVSKEY, val, sizeof(val));
        MPIR_ERR_CHECK(mpi_errno);

	parent_port_name = MPL_strdup(val);
        MPIR_ERR_CHKANDJUMP(parent_port_name == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    *parent_port = parent_port_name;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
void MPIDI_CH3_FreeParentPort(void)
{
    MPL_free( parent_port_name );
    parent_port_name = 0;
}
#endif
