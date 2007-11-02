/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Gatherv(void *sbuff, int scnt, MPI_Datatype stype,
                void *rbuff, int *rcnts, int *displs, MPI_Datatype rtype,
                int root, MPI_Comm comm)
{
    int             g2g = 1, rank;
    char            call[COLLCHK_SM_STRLEN];
    int             are2buffs;

    sprintf(call, "GATHERV");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        MPI_Comm_rank(comm, &rank);

        /* check call consistency */
        CollChk_same_call(comm, call);
        /* check for root consistency */
        CollChk_same_root(comm, root, call);

        /* check for datatype signature consistency */
#if defined( HAVE_MPI_IN_PLACE )
        are2buffs = ( rank == root ? sbuff != MPI_IN_PLACE : 1 );
#else
        are2buffs = 1;
#endif
        CollChk_dtype_scatterv(comm, rtype, rcnts, stype, scnt,
                               root, are2buffs, call);

        return PMPI_Gatherv(sbuff, scnt, stype, rbuff, rcnts, displs, rtype,
                            root, comm);
    }
    else {
        /* init not called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, comm);
    }
}
