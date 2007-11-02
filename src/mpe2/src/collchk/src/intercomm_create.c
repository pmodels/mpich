/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader,
                         MPI_Comm bridge_comm, int remote_leader,
                         int tag, MPI_Comm *newcomm)
{
    int g2g = 1;
    char call[COLLCHK_SM_STRLEN];

    sprintf(call, "INTERCOMM_CREATE");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        /* check for call consistancy */
        CollChk_same_call(local_comm, call);
        /* check local leader consistancy */
        CollChk_same_local_leader(local_comm, local_leader, call);
        /* check tag consistancy */
        CollChk_same_tag(local_comm, tag, call);

        /* make the call */
        return PMPI_Intercomm_create(local_comm, local_leader, bridge_comm, 
                                     remote_leader, tag, newcomm); 
    }
    else {
        /* init not called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, local_comm);
    }
}
