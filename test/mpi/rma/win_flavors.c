/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include <stdio.h>
#include "mpitest.h"

/* MPI-3 is not yet standardized -- allow MPI-3 routines to be switched off.
 */

#if !defined(USE_STRICT_MPI) && defined(MPICH2)
#  define TEST_MPI3_ROUTINES 1
#endif

#define ELEM_SIZE 8

int main( int argc, char *argv[] )
{
    int     rank;
    int     errors = 0, all_errors = 0;
    int    *flavor, *model, flag;
    void   *buf;
    MPI_Win window;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /** Create using MPI_Win_create() **/

#ifdef TEST_MPI3_ROUTINES

    if (rank > 0)
      MPI_Alloc_mem(rank*ELEM_SIZE, MPI_INFO_NULL, &buf);
    else
      buf = NULL;

    MPI_Win_create(buf, rank*ELEM_SIZE, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &window);
    MPI_Win_get_attr(window, MPIX_WIN_CREATE_FLAVOR, &flavor, &flag);

    if (!flag) {
      printf("%d: MPI_Win_create - Error, no flavor\n", rank);
      errors++;
    } else if (*flavor != MPIX_WIN_FLAVOR_CREATE) {
      printf("%d: MPI_Win_create - Error, bad flavor (%d)\n", rank, *flavor);
      errors++;
    }

    MPI_Win_get_attr(window, MPIX_WIN_MODEL, &model, &flag);

    if (!flag) {
      printf("%d: MPI_Win_create - Error, no model\n", rank);
      errors++;
    } else if ( ! (*model == MPIX_WIN_SEPARATE || *model == MPIX_WIN_UNIFIED) ) {
      printf("%d: MPI_Win_create - Error, bad model (%d)\n", rank, *model);
      errors++;
    }

    MPI_Win_free(&window);

    if (buf)
      MPI_Free_mem(buf);

    /** Create using MPI_Win_allocate() **/

    MPIX_Win_allocate(rank*ELEM_SIZE, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &buf, &window);

    if (rank > 0 && buf == NULL) {
      printf("%d: MPI_Win_allocate - Error, bad base pointer\n", rank);
      errors++;
    }

    MPI_Win_get_attr(window, MPIX_WIN_CREATE_FLAVOR, &flavor, &flag);

    if (!flag) {
      printf("%d: MPI_Win_allocate - Error, no flavor\n", rank);
      errors++;
    } else if (*flavor != MPIX_WIN_FLAVOR_ALLOCATE) {
      printf("%d: MPI_Win_allocate - Error, bad flavor (%d)\n", rank, *flavor);
      errors++;
    }

    MPI_Win_get_attr(window, MPIX_WIN_MODEL, &model, &flag);

    if (!flag) {
      printf("%d: MPI_Win_allocate - Error, no model\n", rank);
      errors++;
    } else if (*model != MPIX_WIN_SEPARATE && *model != MPIX_WIN_UNIFIED) {
      printf("%d: MPI_Win_allocate - Error, bad model (%d)\n", rank, *model);
      errors++;
    }

    MPI_Win_free(&window);

    /** Create using MPI_Win_create_dynamic() **/

    MPIX_Win_create_dynamic(MPI_INFO_NULL, MPI_COMM_WORLD, &window);

    MPI_Win_get_attr(window, MPIX_WIN_CREATE_FLAVOR, &flavor, &flag);

    if (!flag) {
      printf("%d: MPI_Win_create_dynamic - Error, no flavor\n", rank);
      errors++;
    } else if (*flavor != MPIX_WIN_FLAVOR_DYNAMIC) {
      printf("%d: MPI_Win_create_dynamic - Error, bad flavor (%d)\n", rank, *flavor);
      errors++;
    }

    MPI_Win_get_attr(window, MPIX_WIN_MODEL, &model, &flag);

    if (!flag) {
      printf("%d: MPI_Win_create_dynamic - Error, no model\n", rank);
      errors++;
    } else if (*model != MPIX_WIN_SEPARATE && *model != MPIX_WIN_UNIFIED) {
      printf("%d: MPI_Win_create_dynamic - Error, bad model (%d)\n", rank, *model);
      errors++;
    }

    MPI_Win_free(&window);

#endif /* TEST_MPI3_ROUTINES */

    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

    MPI_Finalize();

    return 0;
}
