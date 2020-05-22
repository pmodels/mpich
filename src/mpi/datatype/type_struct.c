/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "datatype.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_struct */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_struct = PMPI_Type_struct
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_struct  MPI_Type_struct
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_struct as PMPI_Type_struct
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_struct(int count, int *array_of_blocklengths,
                    MPI_Aint * array_of_displacements,
                    MPI_Datatype * array_of_types, MPI_Datatype * newtype)
    __attribute__ ((weak, alias("PMPI_Type_struct")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_struct
#define MPI_Type_struct PMPI_Type_struct

static MPI_Aint struct_alignsize(int count, const MPI_Datatype * oldtype_array)
{
    MPI_Aint max_alignsize = 0, tmp_alignsize;

    for (int i = 0; i < count; i++) {
        if (HANDLE_IS_BUILTIN(oldtype_array[i])) {
            tmp_alignsize = MPIR_Datatype_builtintype_alignment(oldtype_array[i]);
        } else {
            MPIR_Datatype *dtp;
            MPIR_Datatype_get_ptr(oldtype_array[i], dtp);
            tmp_alignsize = dtp->alignsize;
        }
        if (max_alignsize < tmp_alignsize)
            max_alignsize = tmp_alignsize;
    }

    return max_alignsize;
}

static int type_struct(int count,
                       const int *blocklength_array,
                       const MPI_Aint * displacement_array,
                       const MPI_Datatype * oldtype_array, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int i, old_are_contig = 1, definitely_not_contig = 0;
    int found_true_lb = 0, found_true_ub = 0, found_el_type = 0, found_lb = 0, found_ub = 0;
    MPI_Aint el_sz = 0;
    MPI_Aint size = 0;
    MPI_Datatype el_type = MPI_DATATYPE_NULL;
    MPI_Aint true_lb_disp = 0, true_ub_disp = 0, lb_disp = 0, ub_disp = 0;

    MPIR_Datatype *new_dtp;

    if (count == 0)
        return MPII_Type_zerolen(newtype);

#ifdef MPID_STRUCT_DEBUG
    MPII_Datatype_printf(oldtype_array[0], 1, displacement_array[0], blocklength_array[0], 1);
    for (i = 1; i < count; i++) {
        MPII_Datatype_printf(oldtype_array[i], 1, displacement_array[i], blocklength_array[i], 0);
    }
#endif

    /* allocate new datatype object and handle */
    new_dtp = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         "MPIR_Type_struct", __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* handle is filled in by MPIR_Handle_obj_alloc() */
    MPIR_Object_set_ref(new_dtp, 1);
    new_dtp->is_committed = 0;
    new_dtp->attributes = NULL;
    new_dtp->name[0] = 0;
    new_dtp->contents = NULL;
    new_dtp->flattened = NULL;

    new_dtp->typerep.handle = NULL;

    /* check for junk struct with all zero blocks */
    for (i = 0; i < count; i++)
        if (blocklength_array[i] != 0)
            break;

    if (i == count) {
        MPIR_Handle_obj_free(&MPIR_Datatype_mem, new_dtp);
        return MPII_Type_zerolen(newtype);
    }

    for (i = 0; i < count; i++) {
        MPI_Aint tmp_lb, tmp_ub, tmp_true_lb, tmp_true_ub;
        MPI_Aint tmp_el_sz;
        MPI_Datatype tmp_el_type;
        MPIR_Datatype *old_dtp = NULL;
        int old_is_contig;

        /* Interpreting typemap to not include 0 blklen things. -- Rob
         * Ross, 10/31/2005
         */
        if (blocklength_array[i] == 0)
            continue;

        if (HANDLE_IS_BUILTIN(oldtype_array[i])) {
            tmp_el_sz = MPIR_Datatype_get_basic_size(oldtype_array[i]);
            tmp_el_type = oldtype_array[i];

            MPII_DATATYPE_BLOCK_LB_UB((MPI_Aint) (blocklength_array[i]),
                                      displacement_array[i],
                                      0, tmp_el_sz, tmp_el_sz, tmp_lb, tmp_ub);
            tmp_true_lb = tmp_lb;
            tmp_true_ub = tmp_ub;

            size += tmp_el_sz * blocklength_array[i];
        } else {
            MPIR_Datatype_get_ptr(oldtype_array[i], old_dtp);

            /* Ensure that "builtin_element_size" fits into an int datatype. */
            MPIR_Ensure_Aint_fits_in_int(old_dtp->builtin_element_size);

            tmp_el_sz = old_dtp->builtin_element_size;
            tmp_el_type = old_dtp->basic_type;

            MPII_DATATYPE_BLOCK_LB_UB((MPI_Aint) blocklength_array[i],
                                      displacement_array[i],
                                      old_dtp->lb, old_dtp->ub, old_dtp->extent, tmp_lb, tmp_ub);
            tmp_true_lb = tmp_lb + (old_dtp->true_lb - old_dtp->lb);
            tmp_true_ub = tmp_ub + (old_dtp->true_ub - old_dtp->ub);

            size += old_dtp->size * blocklength_array[i];
        }

        /* element size and type */
        if (found_el_type == 0) {
            el_sz = tmp_el_sz;
            el_type = tmp_el_type;
            found_el_type = 1;
        } else if (el_sz != tmp_el_sz) {
            el_sz = -1;
            el_type = MPI_DATATYPE_NULL;
        } else if (el_type != tmp_el_type) {
            /* Q: should we set el_sz = -1 even though the same? */
            el_type = MPI_DATATYPE_NULL;
        }

        /* keep lowest lb/true_lb and highest ub/true_ub
         *
         * note: checking for contiguity at the same time, to avoid
         *       yet another pass over the arrays
         */
        if (!found_true_lb) {
            found_true_lb = 1;
            true_lb_disp = tmp_true_lb;
        } else if (true_lb_disp > tmp_true_lb) {
            /* element starts before previous */
            true_lb_disp = tmp_true_lb;
            definitely_not_contig = 1;
        }

        if (!found_lb) {
            found_lb = 1;
            lb_disp = tmp_lb;
        } else if (lb_disp > tmp_lb) {
            /* lb before previous */
            lb_disp = tmp_lb;
            definitely_not_contig = 1;
        }

        if (!found_true_ub) {
            found_true_ub = 1;
            true_ub_disp = tmp_true_ub;
        } else if (true_ub_disp < tmp_true_ub) {
            true_ub_disp = tmp_true_ub;
        } else {
            /* element ends before previous ended */
            definitely_not_contig = 1;
        }

        if (!found_ub) {
            found_ub = 1;
            ub_disp = tmp_ub;
        } else if (ub_disp < tmp_ub) {
            ub_disp = tmp_ub;
        } else {
            /* ub before previous */
            definitely_not_contig = 1;
        }

        MPIR_Datatype_is_contig(oldtype_array[i], &old_is_contig);
        if (!old_is_contig)
            old_are_contig = 0;
    }

    new_dtp->n_builtin_elements = -1;   /* TODO */
    new_dtp->builtin_element_size = el_sz;
    new_dtp->basic_type = el_type;

    new_dtp->true_lb = true_lb_disp;
    new_dtp->lb = lb_disp;

    new_dtp->true_ub = true_ub_disp;
    new_dtp->ub = ub_disp;

    new_dtp->alignsize = struct_alignsize(count, oldtype_array);

    new_dtp->extent = new_dtp->ub - new_dtp->lb;
    /* account for padding */
    MPI_Aint epsilon = (new_dtp->alignsize > 0) ?
        new_dtp->extent % ((MPI_Aint) (new_dtp->alignsize)) : 0;

    if (epsilon) {
        new_dtp->ub += ((MPI_Aint) (new_dtp->alignsize) - epsilon);
        new_dtp->extent = new_dtp->ub - new_dtp->lb;
    }

    new_dtp->size = size;

    /* new type is contig for N types if its size and extent are the
     * same, and the old type was also contiguous, and we didn't see
     * something noncontiguous based on true ub/ub.
     */
    if (((MPI_Aint) (new_dtp->size) == new_dtp->extent) &&
        old_are_contig && (!definitely_not_contig)) {
        new_dtp->is_contig = 1;
    } else {
        new_dtp->is_contig = 0;
    }

    mpi_errno = MPIR_Typerep_create_struct(count, blocklength_array, displacement_array,
                                           oldtype_array, new_dtp);
    MPIR_ERR_CHECK(mpi_errno);

    *newtype = new_dtp->handle;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_struct(int count,
                     const int *blocklength_array,
                     const MPI_Aint * displacement_array,
                     const MPI_Datatype * oldtype_array, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;

    /* detect if the old MPI_LB/MPI_UB API is used */
    bool using_old_api = false;
    for (int i = 0; i < count; i++) {
        if (oldtype_array[i] == MPI_LB || oldtype_array[i] == MPI_UB) {
            using_old_api = true;
            break;
        }
    }

    if (!using_old_api) {
        mpi_errno =
            type_struct(count, blocklength_array, displacement_array, oldtype_array, newtype);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        int *real_blocklength_array = (int *) MPL_malloc(count * sizeof(int), MPL_MEM_DATATYPE);
        MPI_Aint *real_displacement_array = (MPI_Aint *) MPL_malloc(count * sizeof(MPI_Aint),
                                                                    MPL_MEM_DATATYPE);
        MPI_Datatype *real_oldtype_array = (MPI_Datatype *) MPL_malloc(count * sizeof(MPI_Datatype),
                                                                       MPL_MEM_DATATYPE);

        int real_count = 0;
        for (int i = 0; i < count; i++) {
            if (oldtype_array[i] != MPI_LB && oldtype_array[i] != MPI_UB) {
                real_blocklength_array[real_count] = blocklength_array[i];
                real_displacement_array[real_count] = displacement_array[i];
                real_oldtype_array[real_count] = oldtype_array[i];
                real_count++;
            }
        }

        MPI_Datatype tmptype;
        mpi_errno = type_struct(real_count, real_blocklength_array, real_displacement_array,
                                real_oldtype_array, &tmptype);
        MPIR_ERR_CHECK(mpi_errno);

        MPL_free(real_oldtype_array);
        MPL_free(real_displacement_array);
        MPL_free(real_blocklength_array);

        MPIR_Datatype *tmptype_ptr;
        MPIR_Datatype_get_ptr(tmptype, tmptype_ptr);

        MPI_Aint lb = tmptype_ptr->lb, ub = tmptype_ptr->ub;
        for (int i = 0; i < count; i++) {
            if (oldtype_array[i] == MPI_LB)
                lb = displacement_array[i];
            else if (oldtype_array[i] == MPI_UB)
                ub = displacement_array[i];
        }

        mpi_errno = MPIR_Type_create_resized(tmptype, lb, ub - lb, newtype);
        MPIR_ERR_CHECK(mpi_errno);

        MPIR_Type_free_impl(&tmptype);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_struct_impl(int count, const int *array_of_blocklengths,
                          const MPI_Aint * array_of_displacements,
                          const MPI_Datatype * array_of_types, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    int i, *ints;
    MPIR_Datatype *new_dtp;
    MPIR_CHKLMEM_DECL(1);

    mpi_errno = MPIR_Type_struct(count,
                                 array_of_blocklengths,
                                 array_of_displacements, array_of_types, &new_handle);
    MPIR_ERR_CHECK(mpi_errno);


    MPIR_CHKLMEM_MALLOC(ints, int *, (count + 1) * sizeof(int), mpi_errno, "contents integer array",
                        MPL_MEM_BUFFER);

    ints[0] = count;
    for (i = 0; i < count; i++) {
        ints[i + 1] = array_of_blocklengths[i];
    }

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp, MPI_COMBINER_STRUCT, count + 1,     /* ints (count, blocklengths) */
                                           count,       /* aints (displacements) */
                                           count,       /* types */
                                           ints, array_of_displacements, array_of_types);

    MPIR_ERR_CHECK(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif

/*@
    MPI_Type_struct - Creates a struct datatype

Input Parameters:
+ count - number of blocks (integer) -- also number of
entries in arrays array_of_types ,
array_of_displacements  and array_of_blocklengths
. array_of_blocklengths - number of elements in each block (array)
. array_of_displacements - byte displacement of each block (array)
- array_of_types - type of elements in each block (array
of handles to datatype objects)

Output Parameters:
. newtype - new datatype (handle)

.N Deprecated
The replacement for this routine is 'MPI_Type_create_struct'

Notes:
If an upperbound is set explicitly by using the MPI datatype 'MPI_UB', the
corresponding index must be positive.

The MPI standard originally made vague statements about padding and alignment;
this was intended to allow the simple definition of structures that could
be sent with a count greater than one.  For example,
.vb
    struct { int a; char b; } foo;
.ve
may have 'sizeof(foo) > sizeof(int) + sizeof(char)'; for example,
'sizeof(foo) == 2*sizeof(int)'.  The initial version of the MPI standard
defined the extent of a datatype as including an `epsilon` that would have
allowed an implementation to make the extent an MPI datatype
for this structure equal to '2*sizeof(int)'.
However, since different systems might define different paddings, there was
much discussion by the MPI Forum about what was the correct value of
epsilon, and one suggestion was to define epsilon as zero.
This would have been the best thing to do in MPI 1.0, particularly since
the 'MPI_UB' type allows the user to easily set the end of the structure.
Unfortunately, this change did not make it into the final document.
Currently, this routine does not add any padding, since the amount of
padding needed is determined by the compiler that the user is using to
build their code, not the compiler used to construct the MPI library.
A later version of MPICH may provide for some natural choices of padding
(e.g., multiple of the size of the largest basic member), but users are
advised to never depend on this, even with vendor MPI implementations.
Instead, if you define a structure datatype and wish to send or receive
multiple items, you should explicitly include an 'MPI_UB' entry as the
last member of the structure.  For example, the following code can be used
for the structure foo
.vb
    blen[0] = 1; array_of_displacements[0] = 0; oldtypes[0] = MPI_INT;
    blen[1] = 1; array_of_displacements[1] = &foo.b - &foo; oldtypes[1] = MPI_CHAR;
    blen[2] = 1; array_of_displacements[2] = sizeof(foo); oldtypes[2] = MPI_UB;
    MPI_Type_struct(3, blen, array_of_displacements, oldtypes, &newtype);
.ve

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_COUNT
.N MPI_ERR_EXHAUSTED
@*/
int MPI_Type_struct(int count,
                    int *array_of_blocklengths,
                    MPI_Aint * array_of_displacements,
                    MPI_Datatype * array_of_types, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_STRUCT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_STRUCT);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            int i;
            MPIR_Datatype *datatype_ptr;

            MPIR_ERRTEST_COUNT(count, mpi_errno);
            if (count > 0) {
                MPIR_ERRTEST_ARGNULL(array_of_blocklengths, "array_of_blocklengths", mpi_errno);
                MPIR_ERRTEST_ARGNULL(array_of_displacements, "array_of_displacements", mpi_errno);
                MPIR_ERRTEST_ARGNULL(array_of_types, "array_of_types", mpi_errno);
            }

            for (i = 0; i < count; i++) {
                MPIR_ERRTEST_ARGNEG(array_of_blocklengths[i], "blocklength", mpi_errno);
                MPIR_ERRTEST_DATATYPE(array_of_types[i], "datatype[i]", mpi_errno);

                if (array_of_types[i] != MPI_DATATYPE_NULL && !HANDLE_IS_BUILTIN(array_of_types[i])) {
                    MPIR_Datatype_get_ptr(array_of_types[i], datatype_ptr);
                    MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                }
            }
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno =
        MPIR_Type_struct_impl(count, array_of_blocklengths, array_of_displacements, array_of_types,
                              newtype);
    if (mpi_errno)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_STRUCT);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_type_struct", "**mpi_type_struct %d %p %p %p %p", count,
                                 array_of_blocklengths, array_of_displacements, array_of_types,
                                 newtype);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
