/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Scatterv(void *sbuff, int *scnts, int* displs, MPI_Datatype stype,
                 void *rbuff, int rcnt, MPI_Datatype rtype,
                 int root, MPI_Comm comm)
{
    char            call[COLLCHK_SM_STRLEN];
    int             g2g = 1, rank;
    int             are2buffs;

    sprintf(call, "SCATTERV");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        MPI_Comm_rank(comm, &rank);

        /* check for call consistency */
        CollChk_same_call(comm, call);
        /* check for same root     */
        CollChk_same_root(comm, root, call);

        /* check for datatype signature consistency */
#if defined( HAVE_MPI_IN_PLACE )
        /* are2buffs = ( rbuff != MPI_IN_PLACE || rank != root ); */
        are2buffs = ( rank == root ? rbuff != MPI_IN_PLACE : 1 );
#else
        are2buffs = 1;
#endif
        CollChk_dtype_scatterv(comm, stype, scnts, rtype, rcnt,
                              root, are2buffs, call);

        /* make the call */
        return PMPI_Scatterv(sbuff, scnts, displs, stype,
                             rbuff, rcnt, rtype, root, comm);
    }
    else {
        /* init has not been called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, comm);
    }
}
