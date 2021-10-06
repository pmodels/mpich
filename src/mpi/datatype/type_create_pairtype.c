/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpiimpl.h>
#include <stdlib.h>
#include <limits.h>

/* PAIRTYPE_SIZE_EXTENT - calculates size, extent, etc. for pairtype by
 * defining the appropriate C type.
 */
#define PAIRTYPE_SIZE_EXTENT(ut1_,ut2_, type_size_, type_extent_,       \
                             el_size_, true_ub_)                        \
    {                                                                   \
        struct { ut1_ a; ut2_ b; } foo;                                 \
        type_size_   = sizeof(foo.a) + sizeof(foo.b);                   \
        type_extent_ = (MPI_Aint) sizeof(foo);                          \
        el_size_ = (sizeof(foo.a) == sizeof(foo.b)) ? (int) sizeof(foo.a) : -1; \
        true_ub_ = ((MPI_Aint) ((char *) &foo.b - (char *) &foo.a)) +   \
            (MPI_Aint) sizeof(foo.b);                                   \
    }

/*@
  MPIR_Type_create_pairtype - create necessary data structures for certain
  pair types (all but MPI_2INT etc., which never have the size != extent
  issue).

  This function is different from the other MPIR_Type_create functions in that
  it fills in an already- allocated MPIR_Datatype.  This is important for
  allowing configure-time determination of the MPI type values (these types
  are stored in the "direct" space, for those familiar with how MPICH deals
  with type allocation).

Input Parameters:
+ type - name of pair type (e.g. MPI_FLOAT_INT)
- new_dtp - pointer to previously allocated datatype structure, which is
            filled in by this function

  Return Value:
  MPI_SUCCESS on success, MPI errno on failure.

  Note:
  Section 4.9.3 (MINLOC and MAXLOC) of the MPI-1 standard specifies that
  these types should be built as if by the following (e.g. MPI_FLOAT_INT):

    type[0] = MPI_FLOAT
    type[1] = MPI_INT
    disp[0] = 0
    disp[1] = sizeof(float) <---- questionable displacement!
    block[0] = 1
    block[1] = 1
    MPI_TYPE_STRUCT(2, block, disp, type, MPI_FLOAT_INT)

  However, this is a relatively naive approach that does not take struct
  padding into account when setting the displacement of the second element.
  Thus in our implementation we have chosen to instead use the actual
  difference in starting locations of the two types in an actual struct.
@*/
int MPIR_Type_create_pairtype(MPI_Datatype type, MPIR_Datatype * new_dtp)
{
    int mpi_errno = MPI_SUCCESS;
    int type_size, alignsize;
    MPI_Aint type_extent, true_ub, el_size;

    /* handle is filled in by MPIR_Handle_obj_alloc() */
    MPIR_Object_set_ref(new_dtp, 1);
    new_dtp->is_committed = 0;
    new_dtp->attributes = NULL;
    new_dtp->name[0] = 0;
    new_dtp->contents = NULL;
    new_dtp->flattened = NULL;

    new_dtp->typerep.handle = MPIR_TYPEREP_HANDLE_NULL;

    switch (type) {
        case MPI_FLOAT_INT:
            PAIRTYPE_SIZE_EXTENT(float, int, type_size, type_extent, el_size, true_ub);
            alignsize = MPL_MAX(ALIGNOF_FLOAT, ALIGNOF_INT);
            break;
        case MPI_DOUBLE_INT:
            PAIRTYPE_SIZE_EXTENT(double, int, type_size, type_extent, el_size, true_ub);
            alignsize = MPL_MAX(ALIGNOF_DOUBLE, ALIGNOF_INT);
            break;
        case MPI_LONG_INT:
            PAIRTYPE_SIZE_EXTENT(long, int, type_size, type_extent, el_size, true_ub);
            alignsize = ALIGNOF_LONG;
            break;
        case MPI_SHORT_INT:
            PAIRTYPE_SIZE_EXTENT(short, int, type_size, type_extent, el_size, true_ub);
            alignsize = ALIGNOF_INT;
            break;
        case MPI_LONG_DOUBLE_INT:
            PAIRTYPE_SIZE_EXTENT(long double, int, type_size, type_extent, el_size, true_ub);
            alignsize = MPL_MAX(ALIGNOF_LONG_DOUBLE, ALIGNOF_INT);
            break;
        default:
            /* --BEGIN ERROR HANDLING-- */
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                             MPIR_ERR_RECOVERABLE,
                                             "MPIR_Type_create_pairtype",
                                             __LINE__, MPI_ERR_OTHER, "**dtype", 0);
            return mpi_errno;
            /* --END ERROR HANDLING-- */
    }

    new_dtp->n_builtin_elements = 2;
    new_dtp->builtin_element_size = el_size;
    new_dtp->basic_type = type;

    new_dtp->true_lb = 0;
    new_dtp->lb = 0;

    new_dtp->true_ub = true_ub;

    new_dtp->size = type_size;
    new_dtp->ub = type_extent;  /* possible padding */
    new_dtp->extent = type_extent;

    MPI_Aint epsilon = type_extent % alignsize;
    if (epsilon) {
        new_dtp->ub += alignsize - epsilon;
        new_dtp->extent += alignsize - epsilon;
    }

    new_dtp->is_contig = (((MPI_Aint) type_size) == type_extent) ? 1 : 0;

    mpi_errno = MPIR_Typerep_create_pairtype(type, new_dtp);

    return mpi_errno;
}
