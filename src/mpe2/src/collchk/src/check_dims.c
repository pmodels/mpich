/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int CollChk_check_dims(MPI_Comm comm, int ndims, int *dims, char* call)
{
    char err_str[COLLCHK_STD_STRLEN], check[COLLCHK_SM_STRLEN];
    int i;
    
    if(CollChk_same_int(comm, ndims, call, "Ndims", err_str) != MPI_SUCCESS) {
        return CollChk_err_han(err_str, COLLCHK_ERR_DIMS, call, comm);
    }

    for (i = 0; i<ndims; i++) {
        sprintf(check, "Dims Sub %d", i);
        if (    CollChk_same_int(comm, dims[i], call, check, err_str)
             != MPI_SUCCESS ) {
            return CollChk_err_han(err_str, COLLCHK_ERR_DIMS, call, comm);
        }
    }

    return MPI_SUCCESS;
}
