/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "cdesc.h"
#include <limits.h>
#include <assert.h>

/* Fortran 2008 specifies a maximum rank of 15 */
#define MAX_RANK  (15)

int cdesc_create_datatype(CFI_cdesc_t * cdesc, MPI_Aint oldcount, MPI_Datatype oldtype,
                          MPI_Datatype * newtype)
{
    MPI_Datatype types[MAX_RANK + 1];   /* Use a fixed size array to avoid malloc. + 1 for oldtype */
    int mpi_errno = MPI_SUCCESS;
    int accum_elems = 1;
    int accum_sm = cdesc->elem_len;
    int done = 0;               /* Have we created a datatype for oldcount of oldtype? */
    int last;                   /* Index of the last successfully created datatype in types[] */
    int extent;
    int i, j;

#ifdef HAVE_ERROR_CHECKING
    {
        int size;
        assert(cdesc->rank <= MAX_RANK);
        PMPI_Type_size(oldtype, &size);
        /* When cdesc->elem_len != size, things suddenly become complicated. Generally, it is hard to create
         * a composite datatype based on two datatypes. Currently we don't support it and doubt it is useful.
         */
        assert(cdesc->elem_len == size);
    }
#endif

    types[0] = oldtype;
    i = 0;
    done = 0;
    while (i < cdesc->rank && !done) {
        if (oldcount % accum_elems) {
            /* oldcount should be a multiple of accum_elems, otherwise we might need an
             * MPI indexed datatype to describle the irregular region, which is not supported yet.
             */
            mpi_errno = MPI_ERR_INTERN;
            last = 0;
            goto fn_fail;
        }

        extent = oldcount / accum_elems;
        if (extent > cdesc->dim[i].extent) {
            extent = cdesc->dim[i].extent;
        } else {
            /* Up to now, we have accumulated enough elements */
            done = 1;
        }

        if (extent <= INT_MAX) {
            if (cdesc->dim[i].sm == accum_sm) {
                mpi_errno = PMPI_Type_contiguous(extent, types[i], &types[i + 1]);
            } else {
                mpi_errno = PMPI_Type_create_hvector(extent, 1, cdesc->dim[i].sm,
                                                     types[i], &types[i + 1]);
            }
        } else {
            if (cdesc->dim[i].sm == accum_sm) {
                mpi_errno = PMPI_Type_contiguous_c(extent, types[i], &types[i + 1]);
            } else {
                mpi_errno = PMPI_Type_create_hvector_c(extent, 1, cdesc->dim[i].sm,
                                                       types[i], &types[i + 1]);
            }
        }
        if (mpi_errno != MPI_SUCCESS) {
            last = i;
            goto fn_fail;
        }

        mpi_errno = PMPI_Type_commit(&types[i + 1]);
        if (mpi_errno != MPI_SUCCESS) {
            last = i + 1;
            goto fn_fail;
        }

        accum_sm = cdesc->dim[i].sm * cdesc->dim[i].extent;
        accum_elems *= cdesc->dim[i].extent;
        i++;
    }

    if (done) {
        *newtype = types[i];
        last = i - 1;   /* To avoid freeing newtype */
    } else {
        /* If # of elements given by "oldcount oldtype" is bigger than
         * what cdesc describles, then we will reach here.
         */
        last = i;
        mpi_errno = MPI_ERR_ARG;
        goto fn_fail;
    }

  fn_exit:
    for (j = 1; j <= last; j++)
        PMPI_Type_free(&types[j]);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
