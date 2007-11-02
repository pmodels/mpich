/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Graph_create(MPI_Comm comm_old, int nnodes, int *index,
                     int *edges, int reorder, MPI_Comm *comm_graph)
{
    int g2g = 1;
    char call[COLLCHK_SM_STRLEN];

    sprintf(call, "GRAPH_CREATE");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        /* check for call consistancy */
        CollChk_same_call(comm_old, call);
        /* check for graph consistancy */
        CollChk_check_graph(comm_old, nnodes, index, edges, call);

        /* make the call */
        return PMPI_Graph_create(comm_old, nnodes, index, edges, reorder,
                                 comm_graph); 
    }
    else {
        /* init not called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, comm_old);
    }
}
