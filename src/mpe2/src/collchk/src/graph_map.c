/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Graph_map(MPI_Comm comm, int nnodes,
                  int *index, int *edges, int *newrank)
{
    int g2g = 1;
    char call[COLLCHK_SM_STRLEN];

    sprintf(call, "GRAPH_MAP");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        /* check for call consistancy */
        CollChk_same_call(comm, call);
        /* check for graph consistancy */
        CollChk_check_graph(comm, nnodes, index, edges, call);

        /* make the call */
        return PMPI_Graph_map(comm, nnodes, index, edges, newrank); 
    }
    else {
        /* init not called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, comm);
    }
}
