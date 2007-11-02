/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int CollChk_check_graph(MPI_Comm comm, int nnodes, int *index, int* edges,
                        char* call)
{
    char err_str[COLLCHK_STD_STRLEN], check[COLLCHK_SM_STRLEN];
    int idx, e;

    e = -1;
    if (    CollChk_same_int(comm, nnodes, call, "Nnodes", err_str)
         != MPI_SUCCESS ) {
        return CollChk_err_han(err_str, COLLCHK_ERR_GRAPH, call, comm);
    }

    for ( idx=0; idx<nnodes; idx++ ) {
        sprintf(check, "Index Sub %d", idx);

        if (    CollChk_same_int(comm, index[idx], call, check, err_str)
             != MPI_SUCCESS) {
            return CollChk_err_han(err_str, COLLCHK_ERR_GRAPH, call, comm);
        }
        
        e = index[idx];
    }

    /*
        The e=index[idx] looks very suspicious in the following loop.
        1) The loop does not modify the array index[] ??
        2) if e is modified, so is the termination condition of the loop!???
    */
    for (idx=0; idx<e; idx++) {
        sprintf(check, "Edges Sub %d", idx);
        
        if(    CollChk_same_int(comm, edges[idx], call, check, err_str)
            != MPI_SUCCESS) {
            return CollChk_err_han(err_str, COLLCHK_ERR_GRAPH, call, comm);
        }

        e = index[idx];
    }

    return MPI_SUCCESS;
}
