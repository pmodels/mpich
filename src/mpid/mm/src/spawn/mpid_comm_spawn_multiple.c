/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pmi.h"

/*@
   MPID_Comm_spawn_multiple - short description

   Input Arguments:
+  int count - count
.  char *array_of_commands[] - commands
.  char* *array_of_argv[] - arguments
.  int array_of_maxprocs[] - maxprocs
.  MPI_Info array_of_info[] - infos
.  int root - root
-  MPI_Comm comm - communicator

   Output Arguments:
+  MPI_Comm *intercomm - intercommunicator
-  int array_of_errcodes[] - error codes

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPID_Comm_spawn_multiple(int count, char *array_of_commands[], char* *array_of_argv[], int array_of_maxprocs[], MPI_Info array_of_info[], int root, MPID_Comm *comm_ptr, MPID_Comm **intercomm, int array_of_errcodes[]) 
{
    char pszPortName[MPI_MAX_PORT_NAME];
    MPI_Info info, prepost_info;
    int same_domain;
    MPIDI_STATE_DECL(MPID_STATE_MPID_COMM_SPAWN_MULTIPLE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_COMM_SPAWN_MULTIPLE);

    PMPI_Info_create(&info);
    if (comm_ptr->rank == root)
    {
	PMPI_Info_create(&prepost_info);
	PMPI_Open_port(MPI_INFO_NULL, pszPortName);
	PMPI_Info_set(prepost_info, MPICH_PARENT_PORT_KEY, pszPortName);
	/*if (g_bSpawnCalledFromMPIExec) PMPI_Info_set(prepost_info, MPICH_EXEC_IS_PARENT_KEY, "yes");*/
	PMI_Spawn_multiple(count, (const char **)array_of_commands, 
	    (const char ***)array_of_argv, array_of_maxprocs, 
	    array_of_info, array_of_errcodes, 
	    &same_domain, (void*)prepost_info);
	PMPI_Info_free(&prepost_info);
	if (same_domain)
	{
	    /* set same domain for accept */
	    PMPI_Info_set(info, MPICH_PMI_SAME_DOMAIN_KEY, "yes");
	}
    }
    /*PMPI_Comm_accept(pszPortName, info, root, comm, intercomm);*/
    if (comm_ptr->rank == root)
    {
	PMPI_Close_port(pszPortName);
    }
    PMPI_Info_free(&info);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_COMM_SPAWN_MULTIPLE);
    return MPI_SUCCESS;
}

int MPID_Comm_spawn(char *command, char *argv[], int maxprocs, MPI_Info info, int root,
		    MPID_Comm *comm, MPID_Comm *intercomm, int array_of_errcodes[])
{
    return MPI_SUCCESS;
}
