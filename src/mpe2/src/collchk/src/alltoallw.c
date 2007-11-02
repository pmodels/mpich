/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 


int MPI_Alltoallw(void* sbuff, int *scnts, int *sdispls, MPI_Datatype *stypes,
                  void* rbuff, int *rcnts, int *rdispls, MPI_Datatype *rtypes,
                  MPI_Comm comm)
{
    int              g2g = 1, r;
    char             call[COLLCHK_SM_STRLEN];

    sprintf(call, "ALLTOALLW");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        MPI_Comm_rank(comm, &r);

        /* check call consistency */
        CollChk_same_call(comm, call);

        /* check data signature consistancy */
        CollChk_dtype_alltoallw(comm, stypes, scnts, rtypes, rcnts, call);

        /* make the call */
        return PMPI_Alltoallw(sbuff, scnts, sdispls, stypes,
                              rbuff, rcnts, rdispls, rtypes, comm);
    }
    else {
        /* init not called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, comm);
    }
}

