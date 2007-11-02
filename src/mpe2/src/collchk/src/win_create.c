/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Win_create(void *base, MPI_Aint size, int disp_unit,
                   MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
    int g2g = 1, ierr;
    char call[COLLCHK_SM_STRLEN];

    sprintf(call, "WIN_CREATE");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        /* check for call consistancy */
        CollChk_same_call(comm, call);

        /* make the call */
        ierr = PMPI_Win_create(base, size, disp_unit, info, comm, win); 

        /* add the comm to the window */
        CollChk_add_win(*win, comm);

        return ierr;
    }
    else {
        /* init not called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, comm);
    }
}
