/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Cart_create(MPI_Comm comm_old, int ndims, int *dims, int *periods,
                    int reorder, MPI_Comm *comm_cart)
{
    int g2g = 1;
    char call[COLLCHK_SM_STRLEN];

    sprintf(call, "CART_CREATE");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        /* check for call consistancy */
        CollChk_same_call(comm_old, call);
        /* check for dims consistancy */
        CollChk_check_dims(comm_old, ndims, dims, call);

        /* make the call */
        return PMPI_Cart_create(comm_old, ndims, dims, periods, reorder,
                                comm_cart); 
    }
    else {
        /* init not called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, comm_old);
    }
}

