/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Win_fence(int assert, MPI_Win win)
{
    int g2g = 1;
    char call[COLLCHK_SM_STRLEN];
    MPI_Comm comm;

    sprintf(call, "WIN_FENCE");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        /* get the comm */
        CollChk_get_win(win, &comm);

        /* check for call consistancy */
        CollChk_same_call(comm, call);

        /* make the call */
        return PMPI_Win_fence(assert, win); 
    }
    else {
        /* init not called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, comm);
    }
}

