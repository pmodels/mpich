/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* The order of entries in this table must match the definitions in
   mpi.h.in */
MPIR_op_function *MPIR_Op_table[] = {
    NULL,
    MPIR_MAXF,
    MPIR_MINF,
    MPIR_SUM,
    MPIR_PROD,
    MPIR_LAND,
    MPIR_BAND,
    MPIR_LOR,
    MPIR_BOR,
    MPIR_LXOR,
    MPIR_BXOR,
    MPIR_MINLOC,
    MPIR_MAXLOC,
    MPIR_REPLACE,
    MPIR_NO_OP
};

MPIR_Op_check_dtype_fn *MPIR_Op_check_dtype_table[] = {
    NULL,
    MPIR_MAXF_check_dtype,
    MPIR_MINF_check_dtype,
    MPIR_SUM_check_dtype,
    MPIR_PROD_check_dtype,
    MPIR_LAND_check_dtype,
    MPIR_BAND_check_dtype,
    MPIR_LOR_check_dtype,
    MPIR_BOR_check_dtype,
    MPIR_LXOR_check_dtype,
    MPIR_BXOR_check_dtype,
    MPIR_MINLOC_check_dtype,
    MPIR_MAXLOC_check_dtype,
    MPIR_REPLACE_check_dtype,
    MPIR_NO_OP_check_dtype
};

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

/* for some predefined datatypes, e.g. from MPI_Type_create_f90_xxx, we need
 * use its basic type for operations */
MPI_Datatype MPIR_Op_get_alt_datatype(MPI_Op op, MPI_Datatype datatype)
{
    MPI_Datatype alt_dt = MPI_DATATYPE_NULL;
    if (!HANDLE_IS_BUILTIN(datatype)) {
        MPIR_Datatype *dt_ptr;
        MPIR_Datatype_get_ptr(datatype, dt_ptr);

        if (dt_ptr && dt_ptr->contents) {
            int combiner = dt_ptr->contents->combiner;
            if (combiner == MPI_COMBINER_F90_REAL ||
                combiner == MPI_COMBINER_F90_COMPLEX || combiner == MPI_COMBINER_F90_INTEGER) {
                if (MPI_SUCCESS == (*MPIR_OP_HDL_TO_DTYPE_FN(op)) (dt_ptr->basic_type)) {
                    alt_dt = dt_ptr->basic_type;
                }
            }
        }
    }
    return alt_dt;
}
