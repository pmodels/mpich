/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm)
{
    int g2g = 1;
    char call[COLLCHK_SM_STRLEN];

    sprintf(call, "INTERCOMM_MERGE");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        /* check for call consistancy */
        CollChk_same_call(intercomm, call);
        /* check high-low consistancy */
        CollChk_same_high_low(intercomm, high, call);
        /* make the call */
        return PMPI_Intercomm_merge(intercomm, high, newintracomm); 
    }
    else {
        /* init not called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, intercomm);
    }
}
