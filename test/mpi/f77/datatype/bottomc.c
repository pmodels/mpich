/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <assert.h>
#include "mpi.h"
#include "../../include/mpitestconf.h"

/*
   Name mapping.  All routines are created with names that are lower case
   with a single trailing underscore.  This matches many compilers.
   We use #define to change the name for Fortran compilers that do
   not use the lowercase/underscore pattern
*/

#ifdef F77_NAME_UPPER
#define c_routine_ C_ROUTINE

#elif defined(F77_NAME_LOWER) || defined(F77_NAME_MIXED)
/* Mixed is ok because we use lowercase in all uses */
#define c_routine_ c_routine

#elif defined(F77_NAME_LOWER_2USCORE) || defined(F77_NAME_LOWER_USCORE) || \
      defined(F77_NAME_MIXED_USCORE)
/* Else leave name alone (routines have no underscore, so both
   of these map to a lowercase, single underscore) */
#else
#error 'Unrecognized Fortran name mapping'
#endif

void c_routine_(MPI_Fint * ftype, int *errs)
{
    int count = 5;
    int lens[2] = { 1, 1 };
    int buf[6];
    int i, rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Aint displs[2];
    MPI_Datatype types[2], newtype;
    /* create an absolute datatype for buffer that consists   */
    /*  of count, followed by R(5)                            */
    MPI_Get_address(&count, &displs[0]);
    displs[1] = 0;
    types[0] = MPI_INT;
    types[1] = MPI_Type_f2c(*ftype);
    MPI_Type_create_struct(2, lens, displs, types, &newtype);
    MPI_Type_commit(&newtype);

    if (rank == 0) {
        /* the message sent contains an int count of 5, followed
         * by the 5 MPI_INTEGER entries of the Fortran array R.
         * Here we assume MPI_INTEGER has the same size as MPI_INT
         */
        assert(sizeof(MPI_Fint) == sizeof(int));
        MPI_Send(MPI_BOTTOM, 1, newtype, 1, 0, MPI_COMM_WORLD);
    }
    else {
        MPI_Recv(buf, 6, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (buf[0] != 5)
            *errs++;
        for (i = 1; i < 6; i++)
            if (buf[i] != i)
                *errs++;
    }

    MPI_Type_free(&newtype);
}
