/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Comm_spawn_multiple(int count, char **array_of_commands,
                            char ***array_of_argv, int *array_of_maxprocs,
                            MPI_Info *array_of_info, int root, MPI_Comm comm, 
                            MPI_Comm *intercomm, int *array_of_errcodes)
{
    int g2g = 1;
    char call[COLLCHK_SM_STRLEN];

    sprintf(call, "COMM_SPAWN_MULTIPLE");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        /* check for call consistancy */
        CollChk_same_call(comm, call);
        /* check for root consistancy */
        CollChk_same_root(comm, root, call);

        /* make the call */
        return PMPI_Comm_spawn_multiple(count, array_of_commands,
                                        array_of_argv, array_of_maxprocs,
                                        array_of_info, root, comm, intercomm,
                                        array_of_errcodes); 
    }
    else {
        /* init not called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, comm);
    }
}
