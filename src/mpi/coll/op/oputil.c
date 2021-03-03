/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

typedef struct op_name {
    MPI_Op op;
    const char *short_name;     /* used in info */
} op_name_t;

static op_name_t mpi_ops[] = {
    {MPI_MAX, "max"},
    {MPI_MIN, "min"},
    {MPI_SUM, "sum"},
    {MPI_PROD, "prod"},
    {MPI_LAND, "land"},
    {MPI_BAND, "band"},
    {MPI_LOR, "lor"},
    {MPI_BOR, "bor"},
    {MPI_LXOR, "lxor"},
    {MPI_BXOR, "bxor"},
    {MPI_MINLOC, "minloc"},
    {MPI_MAXLOC, "maxloc"},
    {MPI_REPLACE, "replace"},
    {MPIX_EQUAL, "equal"},
    {MPI_NO_OP, "no_op"}
};

MPI_Datatype MPIR_Op_builtin_search_by_shortname(const char *short_name)
{
    int i;
    MPI_Op op = MPI_OP_NULL;
    for (i = 0; i < sizeof(mpi_ops) / sizeof(op_name_t); i++) {
        if (!strcmp(mpi_ops[i].short_name, short_name))
            op = mpi_ops[i].op;
    }
    return op;
}

const char *MPIR_Op_builtin_get_shortname(MPI_Op op)
{
    int i;
    MPIR_Assert(HANDLE_IS_BUILTIN(op));
    for (i = 0; i < sizeof(mpi_ops) / sizeof(op_name_t); i++) {
        if (mpi_ops[i].op == op)
            return mpi_ops[i].short_name;
    }
    return "";
}
