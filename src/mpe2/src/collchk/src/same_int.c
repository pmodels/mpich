/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int CollChk_same_int(MPI_Comm comm, int val, char* call, char* check,
                     char* err_str)
{
    /* rank, size, counter, temp comm buffer, go flag, ok flag */
    int r, s, i, buff, go, ok;
    MPI_Status st;int tag=0;   /* needed for communications */
    int inter;                 /* flag if communicator is intra or inter */
    MPI_Comm usecomm;          /* needed if the communicator is inter */

    /* set the error string */
    sprintf(err_str, COLLCHK_NO_ERROR_STR);
    /* see if the communicator is inter or intra */
    MPI_Comm_test_inter(comm, &inter);
    /* if it is make it an intra */
    if(inter) {
        /* tempcomm = comm; */
        /* MPI_Comm_free(&comm); */
        PMPI_Intercomm_merge(comm, 0, &usecomm);
        /* MPI_Comm_free(&tempcomm); */
    }
    else {
        usecomm = comm;
    }

    /* get the rank and size */
    MPI_Comm_rank(usecomm, &r);
    MPI_Comm_size(usecomm, &s);

    /* check the value */
    if (r == 0) {
        /* send 0s value to all other processes */
        buff = val;
        PMPI_Bcast(&buff, 1, MPI_INT, 0, usecomm);
        /* check if all processes are ok to continue */
        go = 1; /* set the go flag */
        for (i=1; i<s; i++) {
            MPI_Recv(&ok, 1, MPI_INT, i, tag, usecomm, &st);
            /* if the process is not ok unset the go flag */
            if (ok != 1) go = 0;
        }

        /* broadcast the go flag to the other processes */
        PMPI_Bcast(&go, 1, MPI_INT, 0, usecomm);
    }
    else {
        /* get the value from 0 */
        PMPI_Bcast(&buff, 1, MPI_INT, 0, usecomm);

        /* check the global value from the local value */
        if (buff != val) {
            /* at this point the parameter is inconsistent */
            /* send an unset ok flag to 0 */
            sprintf(err_str, "%s Parameter (%d) is inconsistent with "
                             "rank 0 (%d)", check, val, buff);
            ok = 0;
            MPI_Send(&ok, 1, MPI_INT, 0, tag, usecomm);
        }
        else {
            /* at this point the parameter is consistent  */
            /* send a set ok flag to 0 */
            ok = 1;
            MPI_Send(&ok, 1, MPI_INT, 0, tag, usecomm);
        }

        /* recieve the go flag from 0 */
        PMPI_Bcast(&go, 1, MPI_INT, 0, usecomm);
    }

    if (go != 1) {
        return MPI_ERR_ARG;
    }

    return MPI_SUCCESS;
}
