/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Bcast(void* buff, int cnt, MPI_Datatype dt, int root, MPI_Comm comm)
{
    int g2g = 1;
    char call[COLLCHK_SM_STRLEN];

    sprintf(call, "BCAST");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        /* check call consistency */
        CollChk_same_call(comm, call);
        /* check root consistency */
        CollChk_same_root(comm, root, call);

        /* check datatype signature consistancy */
        CollChk_dtype_bcast(comm, dt, cnt, 0, call);

        /* make the call */
        return (PMPI_Bcast(buff, cnt, dt, root, comm));
    }
    else {
        /* init not called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, comm);
    }
}

