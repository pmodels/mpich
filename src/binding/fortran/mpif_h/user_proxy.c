/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi_fortimpl.h"
#include "mpl.h"

/* ---- user op ----------------- */
struct F77_op_state {
    F77_OpFunction *opfn;
};

static void F77_op_proxy(void *invec, void *inoutvec, MPI_Count len, MPI_Datatype datatype,
                         void *extra_state)
{
    struct F77_op_state *p = extra_state;
    MPI_Fint len_i = (MPI_Fint) len;
    MPI_Fint datatype_i = MPI_Type_c2f(datatype);

    p->opfn(invec, inoutvec, &len_i, &datatype_i);
}

static void F77_op_free(void *extra_state)
{
    MPL_free(extra_state);
}

int MPII_op_create(F77_OpFunction * opfn, MPI_Fint commute, MPI_Fint * op)
{
    struct F77_op_state *p = MPL_malloc(sizeof(struct F77_op_state), MPL_MEM_OTHER);
    p->opfn = opfn;

    MPI_Op op_i;
    int ret = MPIX_Op_create_x(F77_op_proxy, F77_op_free, commute, p, &op_i);
    if (ret == MPI_SUCCESS) {
        *op = MPI_Op_c2f(op_i);
    } else {
        MPL_free(p);
    }

    return ret;
}
