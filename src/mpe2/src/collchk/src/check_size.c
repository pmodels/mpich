/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int CollChk_check_size(MPI_Comm comm, int size, char* call)
{
    /* rank, size, counter, temp comm buffer, go flag, ok flag */
    int r, s, i, buff, go, ok;
    char err_str[COLLCHK_STD_STRLEN];
    MPI_Status st;int tag=0;   /* needed for communications */

    /* get the rank and size */
    MPI_Comm_rank(comm, &r);
    MPI_Comm_size(comm, &s);

    sprintf(err_str, COLLCHK_NO_ERROR_STR);

    if (r == 0) {
        /* send 0s size to all other processes */
        buff = size;
        PMPI_Bcast(&buff, 1, MPI_INT, 0, comm);

        /* check if all processes are ok to continue */
        go = 1; /* set the go flag */
        for (i=1; i<s; i++) {
            MPI_Recv(&ok, 1, MPI_INT, i, tag, comm, &st);
            /* if the process is not ok unset the go flag */
            if (!ok) go = 0;
        }

        /* broadcast the go flag to the other processes */
        PMPI_Bcast(&go, 1, MPI_INT, 0, comm);
    }
    else {
        /* get the size from 0 */
        PMPI_Bcast(&buff, 1, MPI_INT, 0, comm);

        /* check the size from the local size */
        if (buff != size) {
            /* at this point the size parameter is inconsistant */
            /* print an error message and send an unset ok flag to 0 */
            ok = 0;
            sprintf(err_str, "Data Size (%d) Does not match Rank 0s (%d).\n",
                    size, buff);
            MPI_Send(&ok, 1, MPI_INT, 0, tag, comm);
        }
        else {
            /* at this point the size parameter is consistant  */
            /* send a set ok flag to 0 */
            ok = 1;
            MPI_Send(&ok, 1, MPI_INT, 0, tag, comm);
        }

        /* recieve the go flag from 0 */
        PMPI_Bcast(&go, 1, MPI_INT, 0, comm);
    }

    /* if the go flag is not set exit else return */
    if (!go) {
        return CollChk_err_han(err_str, COLLCHK_ERR_ROOT, call, comm);
    }

    return MPI_SUCCESS;
}
