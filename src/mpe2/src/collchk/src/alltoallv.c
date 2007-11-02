/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Alltoallv(void* sbuff, int *scnts, int *sdispls, MPI_Datatype stype,
                  void* rbuff, int *rcnts, int *rdispls, MPI_Datatype rtype,
                  MPI_Comm comm)
{
    int             g2g = 1, rank;
    char            call[COLLCHK_SM_STRLEN];

    sprintf(call, "ALLTOALLV");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        MPI_Comm_rank(comm, &rank);
        
        /* check call consistency */
        CollChk_same_call(comm, call);
        
        /* check data signature consistency */
        CollChk_dtype_alltoallv(comm, stype, scnts, rtype, rcnts, call);

        return PMPI_Alltoallv(sbuff, scnts, sdispls, stype,
                              rbuff, rcnts, rdispls, rtype, comm);
    }
    else {
        /* init not called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, comm);
    }
}

