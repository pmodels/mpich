/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Reduce(void* sbuff, void* rbuff, int cnt, MPI_Datatype dt, 
               MPI_Op op, int root, MPI_Comm comm)
{
    int g2g = 1;
    char call[COLLCHK_SM_STRLEN];

    sprintf(call, "REDUCE");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        /* check call consistancy */
        CollChk_same_call(comm, call);
        /* check root consistancy */
        CollChk_same_root(comm, root, call);
        /* check operation consistancy */
        CollChk_same_op(comm, op, call);

        /* check datatype signature consistancy */
        CollChk_dtype_bcast(comm, dt, cnt, root, call);

        /* make the call */
        return PMPI_Reduce(sbuff, rbuff, cnt, dt, op, root, comm);
    }
    else {
        /* init not called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, comm);
    }
}
