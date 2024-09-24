/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpif90model.h"

#ifndef HAVE_FORTRAN_BINDING
int MPIR_Type_create_f90_integer_impl(int range, MPI_Datatype * newtype)
{
    *newtype = MPI_DATATYPE_NULL;
    return MPI_SUCCESS;
}

int MPIR_Type_create_f90_real_impl(int precision, int range, MPI_Datatype * newtype)
{
    *newtype = MPI_DATATYPE_NULL;
    return MPI_SUCCESS;
}

int MPIR_Type_create_f90_complex_impl(int precision, int range, MPI_Datatype * newtype)
{
    *newtype = MPI_DATATYPE_NULL;
    return MPI_SUCCESS;
}

#else /* HAVE_FORTRAN_BINDING */
typedef struct intModel {
    int range, kind, bytes;
} intModel;

typedef struct realModel {
    int digits, exponent;
    MPI_Datatype dtype;
} realModel;

static int MPIR_Create_unnamed_predefined(MPI_Datatype old, int combiner,
                                          int r, int p, MPI_Datatype * new_ptr);

int MPIR_Type_create_f90_integer_impl(int range, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;

    static intModel f90_integer_map[] = { MPIR_F90_INTEGER_MODEL_MAP {0, 0, 0} };

    MPI_Datatype basetype = MPI_DATATYPE_NULL;
    for (int i = 0; f90_integer_map[i].range > 0; i++) {
        if (f90_integer_map[i].range >= range) {
            /* Find the corresponding INTEGER type */
            int bytes = f90_integer_map[i].bytes;
            switch (bytes) {
                case 1:
                    basetype = MPI_INTEGER1;
                    break;
                case 2:
                    basetype = MPI_INTEGER2;
                    break;
                case 4:
                    basetype = MPI_INTEGER4;
                    break;
                case 8:
                    basetype = MPI_INTEGER8;
                    break;
                default:
                    break;
            }
            break;
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
    MPI_Datatype basetype;


    static int setupPredefTypes = 1;
    static realModel f90_real_model[2] = {
        {MPIR_F90_REAL_MODEL, MPI_REAL},
        {MPIR_F90_DOUBLE_MODEL, MPI_DOUBLE_PRECISION}
    };

    /* MPI 2.1, Section 16.2, page 473 lines 12-27 make it clear that
     * these types must returned the combiner version. */
    if (setupPredefTypes) {
        setupPredefTypes = 0;
        for (int i = 0; i < 2; i++) {
            MPI_Datatype oldType = f90_real_model[i].dtype;
            mpi_errno = MPIR_Create_unnamed_predefined(oldType,
                                                       MPI_COMBINER_F90_REAL,
                                                       f90_real_model[i].digits,
                                                       f90_real_model[i].exponent,
                                                       &f90_real_model[i].dtype);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    basetype = MPI_DATATYPE_NULL;
    for (int i = 0; i < 2; i++) {
        if (f90_real_model[i].digits >= precision && f90_real_model[i].exponent >= range) {
            basetype = f90_real_model[i].dtype;
            break;
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
    MPI_Datatype basetype;

    static int setupPredefTypes = 1;
    static realModel f90_real_model[2] = {
        {MPIR_F90_REAL_MODEL, MPI_COMPLEX},
        {MPIR_F90_DOUBLE_MODEL, MPI_DOUBLE_COMPLEX}
    };

    /* MPI 2.1, Section 16.2, page 473 lines 12-27 make it clear that
     * these types must return the combiner version. */
    if (setupPredefTypes) {
        setupPredefTypes = 0;
        for (int i = 0; i < 2; i++) {
            MPI_Datatype oldType = f90_real_model[i].dtype;
            mpi_errno = MPIR_Create_unnamed_predefined(oldType,
                                                       MPI_COMBINER_F90_COMPLEX,
                                                       f90_real_model[i].digits,
                                                       f90_real_model[i].exponent,
                                                       &f90_real_model[i].dtype);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    basetype = MPI_DATATYPE_NULL;
    for (int i = 0; i < 2; i++) {
        if (f90_real_model[i].digits >= precision && f90_real_model[i].exponent >= range) {
            basetype = f90_real_model[i].dtype;
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

#ifdef HAVE_FORTRAN_BINDING
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
#endif

/*
   The simple approach used here is to store (unordered) the precision and
   range of each type that is created as a result of a user-call to
   one of the MPI_Type_create_f90_xxx routines.  The standard requires
   that the *same* handle be returned to allow simple == comparisons.
   A finalize callback is added to free up any remaining storage
*/

#if 0
/* Given the requested range and the length of the type in bytes,
   return the matching datatype */

int MPIR_Match_f90_int(int range, int length, MPI_Datatype * newtype)
{
    /* Has this type been requested before? */
    for (int i = 0; i < nAllocInteger; i++) {
        if (f90IntTypes[i].r == range) {
            *newtype = f90IntTypes[i].d;
            return 0;
        }
    }

    /* No.  Try to add it */
    if (nAllocInteger >= MAX_F90_TYPES) {
        int line = -1;          /* Hack to suppress warning message from
                                 * extracterrmsgs, since this code should
                                 * reflect the routine from which it was called,
                                 * since the decomposition of the internal routine
                                 * is of no relevance to either the user or
                                 * developer */
        return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                    "MPI_Type_create_f90_integer", __LINE__,
                                    MPI_ERR_INTERN, "**f90typetoomany", 0);
    }

    /* Temporary */
    *newtype = MPI_DATATYPE_NULL;

    mpi_errno = MPIR_Create_unnamed_predefined(,, r, -1, newtype);
    f90IntTypes[nAllocInteger].r = range;
    f90IntTypes[nAllocInteger].p = -1;
    f90IntTypes[nAllocInteger++].d = *newtype;

    return 0;
}

/* Build a new type */
static int MPIR_Create_unnamed_predefined(MPI_Datatype old, int combiner,
                                          int r, int p, MPI_Datatype * new_ptr)
{
    int mpi_errno;

    /* Create a contiguous type from one instance of the named type */
    mpi_errno = MPIR_Type_contiguous(1, old, new_ptr);

    /* Initialize the contents data */
    if (mpi_errno == MPI_SUCCESS) {
        MPIR_Datatype *new_dtp;
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

        MPIR_Datatype_get_ptr(*new_ptr, new_dtp);
        mpi_errno = MPIR_Datatype_set_contents(new_dtp, combiner, nvals, 0, 0, 0,
                                               vals, NULL, NULL, NULL);
    }

    return mpi_errno;
}
#endif
#endif /* HAVE_FORTRAN_BINDING */
