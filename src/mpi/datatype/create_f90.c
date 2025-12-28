/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

struct fortran_int_model {
    int range;
    MPI_Datatype dtype;
};

struct fortran_real_model {
    int digits;
    int exponent;
    MPI_Datatype dtype;
};

#define MPIR_MAX_F90_MODELS 10
static int num_f90_ints, num_f90_reals, num_f90_complexes;
static struct fortran_int_model f90_ints[MPIR_MAX_F90_MODELS];
static struct fortran_real_model f90_reals[MPIR_MAX_F90_MODELS];
static struct fortran_real_model f90_complexes[MPIR_MAX_F90_MODELS];

int MPIR_add_f90_int(int range, MPI_Datatype dt)
{
    MPIR_Assert(num_f90_ints < MPIR_MAX_F90_MODELS);
    f90_ints[num_f90_ints++] = (struct fortran_int_model) {
    range, dt};
    return MPI_SUCCESS;
}

int MPIR_add_f90_real(int digits, int exponent, MPI_Datatype dt)
{
    MPIR_Assert(num_f90_reals < MPIR_MAX_F90_MODELS);
    f90_reals[num_f90_reals++] = (struct fortran_real_model) {
    digits, exponent, dt};
    return MPI_SUCCESS;
}

int MPIR_add_f90_complex(int digits, int exponent, MPI_Datatype dt)
{
    MPIR_Assert(num_f90_complexes < MPIR_MAX_F90_MODELS);
    f90_complexes[num_f90_complexes++] = (struct fortran_real_model) {
    digits, exponent, dt};
    return MPI_SUCCESS;
}

int MPIR_check_f90_fallback(void)
{
    /* This is called in MPIR_Abi_set_fortran_info_impl after all fixed-size types are added.
     * In case none of the fixed types are supported, we fallback to support INTEGER, REAL,
     * DOUBLE, and COMPLEX */
    if (num_f90_ints == 0) {
        MPI_Datatype dt = MPI_INTEGER;
        MPIR_DATATYPE_REPLACE_BUILTIN(dt);
        MPIR_Assert(dt != MPI_DATATYPE_NULL);
        int integer_size = MPIR_Datatype_get_basic_size(dt);
        switch (integer_size) {
            case 1:
                MPIR_add_f90_int(2, dt);
                break;
            case 2:
                MPIR_add_f90_int(4, dt);
                break;
            case 4:
                MPIR_add_f90_int(9, dt);
                break;
            case 8:
                MPIR_add_f90_int(18, dt);
                break;
            case 16:
                MPIR_add_f90_int(38, dt);
                break;
            default:
                MPIR_Assert(0);
        }
    }

    if (num_f90_reals == 0) {
        MPI_Datatype dt[2] = { MPI_REAL, MPI_DOUBLE };
        for (int i = 0; i < 2; i++) {
            MPIR_DATATYPE_REPLACE_BUILTIN(dt[i]);
            MPIR_Assert(dt[i] != MPI_DATATYPE_NULL);
            int dt_size = MPIR_Datatype_get_basic_size(dt[i]);
            switch (dt_size) {
                case 2:
                    MPIR_add_f90_real(3, 4, dt[i]);
                    break;
                case 4:
                    MPIR_add_f90_real(6, 37, dt[i]);
                    break;
                case 8:
                    MPIR_add_f90_real(15, 307, dt[i]);
                    break;
                case 16:
                    MPIR_add_f90_real(33, 4931, dt[i]);
                    break;
                default:
                    MPIR_Assert(0);
            }
        }
    }

    if (num_f90_complexes == 0) {
        MPI_Datatype dt = MPI_COMPLEX;
        MPIR_DATATYPE_REPLACE_BUILTIN(dt);
        MPIR_Assert(dt != MPI_DATATYPE_NULL);
        int complex_size = MPIR_Datatype_get_basic_size(dt);
        switch (complex_size) {
            case 4:
                MPIR_add_f90_complex(3, 4, dt);
                break;
            case 8:
                MPIR_add_f90_complex(6, 37, dt);
                break;
            case 16:
                MPIR_add_f90_complex(15, 307, dt);
                break;
            case 32:
                MPIR_add_f90_complex(33, 4931, dt);
                break;
            default:
                MPIR_Assert(0);
        }
    }

    return MPI_SUCCESS;
}

static int MPIR_Create_unnamed_predefined(MPI_Datatype old, int combiner,
                                          int r, int p, MPI_Datatype * new_ptr);

int MPIR_Type_create_f90_integer_impl(int range, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;

    MPI_Datatype basetype = MPI_DATATYPE_NULL;
    for (int i = 0; i < num_f90_ints; i++) {
        if (f90_ints[i].range >= range) {
            basetype = f90_ints[i].dtype;
        }
    }

    if (basetype == MPI_DATATYPE_NULL) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         "MPI_Type_create_f90_integer",
                                         __LINE__, MPI_ERR_OTHER,
                                         "**f90typeintnone", "**f90typeintnone %d", range);
    } else {
        mpi_errno = MPIR_Create_unnamed_predefined(basetype,
                                                   MPI_COMBINER_F90_INTEGER, range, -1, newtype);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_f90_real_impl(int precision, int range, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;

    MPI_Datatype basetype = MPI_DATATYPE_NULL;
    for (int i = 0; i < num_f90_reals; i++) {
        if (f90_reals[i].digits >= precision && f90_reals[i].exponent >= range) {
            basetype = f90_reals[i].dtype;
        }
    }

    if (basetype == MPI_DATATYPE_NULL) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         "MPI_Type_create_f90_real",
                                         __LINE__, MPI_ERR_OTHER,
                                         "**f90typerealnone",
                                         "**f90typerealnone %d %d", precision, range);
    } else {
        mpi_errno = MPIR_Create_unnamed_predefined(basetype,
                                                   MPI_COMBINER_F90_REAL, range, precision,
                                                   newtype);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_create_f90_complex_impl(int precision, int range, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;

    MPI_Datatype basetype = MPI_DATATYPE_NULL;
    for (int i = 0; i < num_f90_complexes; i++) {
        if (f90_complexes[i].digits >= precision && f90_complexes[i].exponent >= range) {
            basetype = f90_complexes[i].dtype;
            break;
        }
    }

    if (basetype == MPI_DATATYPE_NULL) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         "MPI_Type_create_f90_complex",
                                         __LINE__, MPI_ERR_OTHER,
                                         "**f90typecomplexnone",
                                         "**f90typecomplexnone %d %d", precision, range);
    } else {
        mpi_errno = MPIR_Create_unnamed_predefined(basetype,
                                                   MPI_COMBINER_F90_COMPLEX, range, precision,
                                                   newtype);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* The MPI standard requires that the datatypes that are returned from
   the MPI_Create_f90_xxx be both predefined and return the r (and p
   if real/complex) with which it was created.  The following contains routines
   that keep track of the datatypes that have been created.  The interface
   that is used is a single routine that is passed as input the
   chosen base function and combiner, along with r,p, and returns the
   appropriate datatype, creating it if necessary */

/* This gives the maximum number of distinct types returned by any one of the
   MPI_Type_create_f90_xxx routines */
#define MAX_F90_TYPES 64
typedef struct {
    int combiner;
    int r, p;
    MPI_Datatype d;
} F90Predefined;
static int nAlloc = 0;
static F90Predefined f90Types[MAX_F90_TYPES];

static int MPIR_FreeF90Datatypes(void *d)
{
    MPIR_Datatype *dptr;

    for (int i = 0; i < nAlloc; i++) {
        MPIR_Datatype_get_ptr(f90Types[i].d, dptr);
        MPIR_Datatype_free(dptr);
    }
    return 0;
}

static int MPIR_Create_unnamed_predefined(MPI_Datatype old, int combiner,
                                          int r, int p, MPI_Datatype * new_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    F90Predefined *type;

    *new_ptr = MPI_DATATYPE_NULL;

    /* Has this type been defined already? */
    for (int i = 0; i < nAlloc; i++) {
        type = &f90Types[i];
        if (type->combiner == combiner && type->r == r && type->p == p) {
            *new_ptr = type->d;
            return mpi_errno;
        }
    }

    /* Create a new type and remember it */
    if (nAlloc >= MAX_F90_TYPES) {
        return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                    "MPIF_Create_unnamed_predefined", __LINE__,
                                    MPI_ERR_INTERN, "**f90typetoomany", 0);
    }
    if (nAlloc == 0) {
        /* Install the finalize callback that frees these datatypes.
         * Set the priority high enough that this will be executed
         * before the handle allocation check */
        MPIR_Add_finalize(MPIR_FreeF90Datatypes, 0, 2);
    }

    type = &f90Types[nAlloc++];
    type->combiner = combiner;
    type->r = r;
    type->p = p;

    /* Create a contiguous type from one instance of the named type */
    mpi_errno = MPIR_Type_contiguous(1, old, &type->d);
    MPIR_ERR_CHECK(mpi_errno);

    /* Initialize the contents data */
    {
        MPIR_Datatype *new_dtp = NULL;
        int vals[2];
        int nvals = 0;

        switch (combiner) {
            case MPI_COMBINER_F90_INTEGER:
                nvals = 1;
                vals[0] = r;
                break;

            case MPI_COMBINER_F90_REAL:
            case MPI_COMBINER_F90_COMPLEX:
                nvals = 2;
                vals[0] = p;
                vals[1] = r;
                break;
        }

        MPIR_Datatype_get_ptr(type->d, new_dtp);
        mpi_errno = MPIR_Datatype_set_contents(new_dtp, combiner, nvals, 0, 0, 0,
                                               vals, NULL, NULL, NULL);
        MPIR_ERR_CHECK(mpi_errno);

#ifndef NDEBUG
        {
            MPI_Datatype old_basic = MPI_DATATYPE_NULL;
            MPI_Datatype new_basic = MPI_DATATYPE_NULL;
            /* we used MPIR_Type_contiguous and then stomped it's contents
             * information, so make sure that the basic_type is usable by
             * MPIR_Type_commit_impl */
            MPIR_Datatype_get_basic_type(old, old_basic);
            MPIR_Datatype_get_basic_type(new_dtp->handle, new_basic);
            MPIR_Assert(new_basic == old_basic);
        }
#endif

        /* the MPI Standard requires that these types are pre-committed
         * (MPI-2.2, sec 16.2.5, pg 492) */
        mpi_errno = MPIR_Type_commit_impl(&type->d);
        MPIR_ERR_CHECK(mpi_errno);
    }

    *new_ptr = type->d;

  fn_fail:
    return mpi_errno;
}
