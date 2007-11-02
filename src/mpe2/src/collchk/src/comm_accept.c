/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Comm_accept(char *portname, MPI_Info info, int root,
                    MPI_Comm comm, MPI_Comm *newcomm)
{
    int g2g = 1;
    char call[COLLCHK_SM_STRLEN];

    sprintf(call, "COMM_ACCEPT");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        /* check for call consistancy */
        CollChk_same_call(comm, call);
        /* check for root consistancy */
        CollChk_same_root(comm, root, call);

        /* make the call */
        return PMPI_Comm_accept(portname, info, root, comm, newcomm); 
    }
    else {
        /* init not called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, comm);
    }
}
