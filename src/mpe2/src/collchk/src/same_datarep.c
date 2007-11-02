/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int CollChk_same_datarep(MPI_Comm comm, char* datarep, char *call)
{
    int r, s, i, go, ok;     /* rank, size, counter, go flag, ok flag */
    char buff[COLLCHK_STD_STRLEN];          /* temp communication buffer */
    char err_str[COLLCHK_STD_STRLEN];       /* error string */
    MPI_Status st;int tag=0; /* needed for communication */
    int inter;               /* flag for inter or intra communicator */
    MPI_Comm usecomm;        /* needed if intercommunicator */

    /* set the error string */
    sprintf(err_str, COLLCHK_NO_ERROR_STR);

    /* test if the communicator is intra of inter */
    MPI_Comm_test_inter(comm, &inter);
    /* if inter then convert to intra */
    if (inter) {
        PMPI_Intercomm_merge(comm, 0, &usecomm);
    }
    else {
        usecomm = comm;
    }

    /* get rank and size */
    MPI_Comm_rank(usecomm, &r);
    MPI_Comm_size(usecomm, &s);

    if (r == 0) {
        /* send the datarep to the other processes */
        strcpy(buff, datarep);
        PMPI_Bcast(buff, COLLCHK_STD_STRLEN, MPI_CHAR, 0, usecomm);
        /* ask the other processes if they are ok to continue */
        go = 1;     /* sets the go flag */
        for(i=1; i<s; i++) {
            MPI_Recv(&ok, 1, MPI_INT, i, tag, usecomm, &st);
            /* if a process has a bad datarep unset the go flag */
            if(!ok) go = 0;
        }

        /* broadcast to the go flag */
        PMPI_Bcast(&go, 1, MPI_INT, 0, usecomm);
    }
    else {
        /* recieve 0's datarep */
        PMPI_Bcast(buff, COLLCHK_STD_STRLEN, MPI_CHAR, 0, usecomm);
        /* check it against the local datarep */
        if (strcmp(buff, datarep) != 0) {
            /* at this point the call is not consistant */
            /* print an error message and send a unset ok flag to 0 */
            ok = 0;
            sprintf(err_str, "Datarep (%s) is Inconsistent with "
                             "Rank 0's (%s).", datarep, buff);
            MPI_Send(&ok, 1, MPI_INT, 0, tag, usecomm);
        }
        else {
            /* at this point the call is consistant */
            /* send an set ok flag to 0 */
            ok = 1;
            MPI_Send(&ok, 1, MPI_INT, 0, tag, usecomm);
        }
        /* get the go flag from 0 */
        PMPI_Bcast(&go, 1, MPI_INT, 0, usecomm);
    }
    /* if the go flag is not set exit else return */
    if(!go) {
        return CollChk_err_han(err_str, COLLCHK_ERR_DATAREP, call, usecomm);
    }

    return MPI_SUCCESS;
}
