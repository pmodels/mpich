/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 


static const char* CollChk_get_op_string(MPI_Op op);
static const char* CollChk_get_op_string(MPI_Op op)
{
    if ( op == MPI_MAX )
        return "MPI_MAX";
    else if ( op == MPI_MIN )
        return "MPI_MIN";
    else if ( op == MPI_SUM )
        return "MPI_SUM";
    else if ( op == MPI_PROD )
        return "MPI_PROD";
    else if ( op == MPI_LAND )
        return "MPI_LAND";
    else if ( op == MPI_BAND )
        return "MPI_BAND";
    else if ( op == MPI_LOR )
        return "MPI_LOR";
    else if ( op == MPI_BOR )
        return "MPI_BOR";
    else if ( op == MPI_LXOR )
        return "MPI_LXOR";
    else if ( op == MPI_BXOR )
        return "MPI_BXOR";
    else if ( op == MPI_MAXLOC )
        return "MPI_MAXLOC";
    else if ( op == MPI_MINLOC )
        return "MPI_MINLOC";
    else
        return "USER_OP\0";
}


int CollChk_same_op(MPI_Comm comm, MPI_Op op, char* call)
{
    int r, s, i, go, ok;    /* rank, size, counter, go flag, ok flag */
    char buff[COLLCHK_SM_STRLEN];          /* temp communication buffer */
    char op_str[15];        /* the operation name in string format */
    char err_str[COLLCHK_STD_STRLEN];      /* error string */
    int tag=0;              /* needed for communication */
    MPI_Status st;

    /* get rank and size */
    MPI_Comm_rank(comm, &r);
    MPI_Comm_size(comm, &s);

    sprintf(err_str, COLLCHK_NO_ERROR_STR);
    sprintf(op_str, "%s", CollChk_get_op_string(op));

    if (r == 0) {
        /* send the name of the op to the other processes */
        strcpy(buff, op_str);
        PMPI_Bcast(buff, 15, MPI_CHAR, 0, comm);
        /* ask the other processes if they are ok to continue */
        go = 1;     /* sets the go flag */
        for(i=1; i<s; i++) {
            MPI_Recv(&ok, 1, MPI_INT, i, tag, comm, &st);
            /* if a process has made a bad call unset the go flag */
            if (ok != 1)
               go = 0;
        }

        /* broadcast to the go flag */
        PMPI_Bcast(&go, 1, MPI_INT, 0, comm);
    }
    else {
        /* recieve 0's op name */
        PMPI_Bcast(buff, 15, MPI_CHAR, 0, comm);
        /* check it against the local op name */
        if (strcmp(buff, op_str) != 0) {
            /* at this point the op is not consistant */
            /* print an error message and send a unset ok flag to 0 */
            ok = 0;
            sprintf(err_str, "Inconsistent operation (%s) to "
                             "Rank 0's operation(%s).", op_str, buff);
            MPI_Send(&ok, 1, MPI_INT, 0, tag, comm);
        }
        else {
            /* at this point the op is consistant */
            /* send an set ok flag to 0 */
            ok = 1;
            MPI_Send(&ok, 1, MPI_INT, 0, tag, comm);
        }
        /* get the go flag from 0 */
        PMPI_Bcast(&go, 1, MPI_INT, 0, comm);
    }
    /* if the go flag is not set exit else return */
    if (go != 1) {
        return CollChk_err_han(err_str, COLLCHK_ERR_OP, call, comm);
    }

    return MPI_SUCCESS;
}
