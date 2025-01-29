/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpiimpl.h>
#include <stdlib.h>
#include <limits.h>
#include "mpir_op_util.h"

#define MPIR_MAX_PAIRTYPES 20   /* some number we know it is sufficient */
struct MPIR_pairtype MPIR_pairtypes[MPIR_MAX_PAIRTYPES];
int MPIR_num_pairtypes;

/* PAIRTYPE_SIZE_EXTENT - calculates size, extent, etc. for pairtype by
 * defining the appropriate C type.
 */
#define PAIRTYPE_SIZE_EXTENT(ut1_,ut2_, type_extent_, true_ub_, idx_offset_) \
    {                                                                   \
        struct foo_ { ut1_ a; ut2_ b; };                                \
        idx_offset_  = offsetof(struct foo_, b);                        \
        type_extent_ = (MPI_Aint) sizeof(struct foo_);                  \
        true_ub_ = idx_offset_ + (MPI_Aint) sizeof(ut2_);               \
    }

static int create_pairtype(MPI_Datatype value_type, MPIR_Datatype ** new_dtp)
{
    int mpi_errno = MPI_SUCCESS;

    MPI_Aint type_extent, true_ub, idx_offset;
    MPI_Datatype value_datatype;
    switch (value_type) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) \
        case (mpi_type_) : { \
            value_datatype = mpi_type_; \
            PAIRTYPE_SIZE_EXTENT(c_type_, int, type_extent, true_ub, idx_offset); \
            break; \
        }
            MPIR_OP_TYPE_GROUP(INTEGER)
                MPIR_OP_TYPE_GROUP(FLOATING_POINT)
        case MPI_DATATYPE_NULL:
            value_datatype = MPI_DATATYPE_NULL;
            break;
        default:
            /* --BEGIN ERROR HANDLING-- */
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "create_pairtype",
                                             __LINE__, MPI_ERR_OTHER, "**dtype", 0);
            return mpi_errno;
            /* --END ERROR HANDLING-- */
    }

    if (value_datatype == MPI_DATATYPE_NULL) {
        MPI_Datatype dt;
        mpi_errno = MPII_Type_zerolen(&dt);
        MPIR_Datatype_get_ptr(dt, *new_dtp);
        goto fn_exit;
    }

    MPI_Aint blklens[2] = { 1, 1 };
    MPI_Aint disps[2] = { 0, idx_offset };
    MPI_Datatype types[2] = { value_datatype, MPIR_INT_INTERNAL };

    MPI_Datatype dt;
    mpi_errno = MPIR_Type_struct(2, blklens, disps, types, &dt);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Datatype *dptr;
    MPIR_Datatype_get_ptr(dt, dptr);

    /* set contents -
     *     The only named non-builtin datatypes are these pair types.
     */
    int ints[3] = { 2, 1, 1 };
    MPI_Aint aints[2] = { 0, idx_offset };
    mpi_errno = MPIR_Datatype_set_contents(dptr, MPI_COMBINER_STRUCT, 3, 2, 0, 2,
                                           ints, aints, NULL, types);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Assertp(dptr->true_ub == true_ub);
    MPIR_Assertp(dptr->extent == type_extent);

    int idx = dptr->handle & 0xff;
    MPIR_Assertp(MPIR_num_pairtypes == idx);
    MPIR_Assertp(idx < MPIR_MAX_PAIRTYPES);

    MPI_Datatype datatype = dptr->handle;
    mpi_errno = MPIR_Type_commit_impl(&datatype);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_pairtypes[idx].value_type = value_type;
    MPIR_num_pairtypes++;

    *new_dtp = dptr;
  fn_exit:
    return mpi_errno;
  fn_fail:
    *new_dtp = NULL;
    goto fn_exit;
}

int MPIR_Datatype_init_pairtypes(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *dptr;

#define CREATE_PAIRTYPE(T) \
    do { \
        mpi_errno = create_pairtype(MPIR_##T##_INTERNAL, &dptr); \
        MPIR_ERR_CHECK(mpi_errno); \
        MPIR_Assert(dptr->handle == MPI_##T##_INT); \
        dptr->basic_type = MPI_##T##_INT; \
        MPL_strncpy(dptr->name, "MPI_" #T "_INT", MPI_MAX_OBJECT_NAME); \
    } while (0)

    CREATE_PAIRTYPE(FLOAT);
    CREATE_PAIRTYPE(DOUBLE);
    CREATE_PAIRTYPE(LONG);
    CREATE_PAIRTYPE(SHORT);
    CREATE_PAIRTYPE(LONG_DOUBLE);

  fn_fail:
    return mpi_errno;
}

int MPIR_Datatype_finalize_pairtypes(void)
{
    int mpi_errno = MPI_SUCCESS;

    for (int i = 0; i < MPIR_num_pairtypes; i++) {
        MPI_Datatype pairtype = MPI_FLOAT_INT + i;
        MPIR_Type_free_impl(&pairtype);
    }

    MPIR_num_pairtypes = 0;

    return mpi_errno;
}
