/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 
#include "mpe_callstack.h"

#if ! defined( HAVE_MPI_ERR_FNS )
int MPI_Add_error_class(int *errorclass)
{ return MPI_SUCCESS; }

int MPI_Add_error_code(int errorclass, int *errorcode)
{ return MPI_SUCCESS; }

int MPI_Add_error_string(int errorcode, char *string)
{
    fprintf(stderr, "%s", string);
    fflush(stderr);
    return MPI_SUCCESS;
}

int MPI_Comm_call_errhandler(MPI_Comm comm, int errorcode)
{
    /* Wait for few seconds so others can finish printing error messages */
    sleep(5);
    return MPI_Abort(comm, 1);
}
#endif

int CollChk_err_han(char *err_str, int err_code, char *call, MPI_Comm comm)
{
    int   rank;
    char  msg[COLLCHK_STD_STRLEN];
    MPE_CallStack_t cstk;

    if(err_code == COLLCHK_ERR_NOT_INIT) {
        printf("Collective Checking: %s --> %s\n", call, err_str);
        fflush(stdout); fflush(stderr);
    }
    else if (strcmp(err_str, COLLCHK_NO_ERROR_STR) != 0) {    
        MPI_Comm_rank(comm, &rank);
        sprintf(msg, "Backtrace of the callstack at rank %d:\n", rank );
        write( STDERR_FILENO, msg, strlen(msg)+1 );
        MPE_CallStack_init( &cstk );
        MPE_CallStack_fancyprint( &cstk, STDERR_FILENO,
                                  "\tAt ", 1, MPE_CALLSTACK_UNLIMITED );
        sprintf(msg, "\n\nCollective Checking: %s (Rank %d) --> %s\n\n",
                     call, rank, err_str);
        MPI_Add_error_string(err_code, msg);
    }
    else {
        MPI_Add_error_string(err_code, "Error on another process");
        sleep(1);
    }

    return MPI_Comm_call_errhandler(comm, err_code);
}
