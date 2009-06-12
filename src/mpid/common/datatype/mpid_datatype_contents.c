/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>
#include <limits.h>

/*@
  MPID_Datatype_set_contents - store contents information for use in
                               MPI_Type_get_contents.

  Returns MPI_SUCCESS on success, MPI error code on error.
@*/
int MPID_Datatype_set_contents(MPID_Datatype *new_dtp,
			       int combiner,
			       int nr_ints,
			       int nr_aints,
			       int nr_types,
			       int array_of_ints[],
			       MPI_Aint array_of_aints[],
			       MPI_Datatype array_of_types[])
{
    int i, contents_size, align_sz = 8, epsilon, mpi_errno;
    int struct_sz, ints_sz, aints_sz, types_sz;
    MPID_Datatype_contents *cp;
    MPID_Datatype *old_dtp;
    char *ptr;

#ifdef HAVE_MAX_STRUCT_ALIGNMENT
    if (align_sz > HAVE_MAX_STRUCT_ALIGNMENT) {
	align_sz = HAVE_MAX_STRUCT_ALIGNMENT;
    }
#endif

    struct_sz = sizeof(MPID_Datatype_contents);
    types_sz  = nr_types * sizeof(MPI_Datatype);
    ints_sz   = nr_ints * sizeof(int);
    aints_sz  = nr_aints * sizeof(MPI_Aint);

    /* pad the struct, types, and ints before we allocate.
     *
     * note: it's not necessary that we pad the aints,
     *       because they are last in the region.
     */
    if ((epsilon = struct_sz % align_sz)) {
	struct_sz += align_sz - epsilon;
    }
    if ((epsilon = types_sz % align_sz)) {
	types_sz += align_sz - epsilon;
    }
    if ((epsilon = ints_sz % align_sz)) {
	ints_sz += align_sz - epsilon;
    }

    contents_size = struct_sz + types_sz + ints_sz + aints_sz;

    cp = (MPID_Datatype_contents *) MPIU_Malloc(contents_size);
    /* --BEGIN ERROR HANDLING-- */
    if (cp == NULL) {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
					 MPIR_ERR_RECOVERABLE,
					 "MPID_Datatype_set_contents",
					 __LINE__,
					 MPI_ERR_OTHER,
					 "**nomem",
					 0);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    cp->combiner = combiner;
    cp->nr_ints  = nr_ints;
    cp->nr_aints = nr_aints;
    cp->nr_types = nr_types;

    /* arrays are stored in the following order: types, ints, aints,
     * following the structure itself.
     */
    ptr = ((char *) cp) + struct_sz;
    /* Fortran90 combiner types do not have a "base" type */
    if (nr_types > 0) {
	MPIU_Memcpy(ptr, array_of_types, nr_types * sizeof(MPI_Datatype));
    }
    
    ptr = ((char *) cp) + struct_sz + types_sz;
    if (nr_ints > 0) {
	MPIU_Memcpy(ptr, array_of_ints, nr_ints * sizeof(int));
    }

    ptr = ((char *) cp) + struct_sz + types_sz + ints_sz;
    if (nr_aints > 0) {
	MPIU_Memcpy(ptr, array_of_aints, nr_aints * sizeof(MPI_Aint));
    }
    new_dtp->contents = cp;

    /* increment reference counts on all the derived types used here */
    for (i=0; i < nr_types; i++) {
	if (HANDLE_GET_KIND(array_of_types[i]) != HANDLE_KIND_BUILTIN) {
	    MPID_Datatype_get_ptr(array_of_types[i], old_dtp);
	    MPID_Datatype_add_ref(old_dtp);
	}
    }

    return MPI_SUCCESS;
}

void MPID_Datatype_free_contents(MPID_Datatype *dtp)
{
    int i, struct_sz = sizeof(MPID_Datatype_contents);
    int align_sz = 8, epsilon;
    MPID_Datatype *old_dtp;
    MPI_Datatype *array_of_types;

    if ((epsilon = struct_sz % align_sz)) {
	struct_sz += align_sz - epsilon;
    }

    /* note: relies on types being first after structure */
    array_of_types = (MPI_Datatype *) ((char *)dtp->contents + struct_sz);

    for (i=0; i < dtp->contents->nr_types; i++) {
	if (HANDLE_GET_KIND(array_of_types[i]) != HANDLE_KIND_BUILTIN) {
	    MPID_Datatype_get_ptr(array_of_types[i], old_dtp);
	    MPID_Datatype_release(old_dtp);
	}
    }

    MPIU_Free(dtp->contents);
    dtp->contents = NULL;
}

void MPIDI_Datatype_get_contents_ints(MPID_Datatype_contents *cp,
				      int *user_ints)
{
    char *ptr;
    int align_sz = 8, epsilon;
    int struct_sz, types_sz;

#ifdef HAVE_MAX_STRUCT_ALIGNMENT
    if (align_sz > HAVE_MAX_STRUCT_ALIGNMENT) {
	align_sz = HAVE_MAX_STRUCT_ALIGNMENT;
    }
#endif

    struct_sz = sizeof(MPID_Datatype_contents);
    types_sz  = cp->nr_types * sizeof(MPI_Datatype);

    /* pad the struct, types, and ints before we allocate.
     *
     * note: it's not necessary that we pad the aints,
     *       because they are last in the region.
     */
    if ((epsilon = struct_sz % align_sz)) {
	struct_sz += align_sz - epsilon;
    }
    if ((epsilon = types_sz % align_sz)) {
	types_sz += align_sz - epsilon;
    }

    ptr = ((char *) cp) + struct_sz + types_sz;
    MPIU_Memcpy(user_ints, ptr, cp->nr_ints * sizeof(int));

    return;
}

void MPIDI_Datatype_get_contents_aints(MPID_Datatype_contents *cp,
				       MPI_Aint *user_aints)
{
    char *ptr;
    int align_sz = 8, epsilon;
    int struct_sz, ints_sz, aints_sz, types_sz;

#ifdef HAVE_MAX_STRUCT_ALIGNMENT
    if (align_sz > HAVE_MAX_STRUCT_ALIGNMENT) {
	align_sz = HAVE_MAX_STRUCT_ALIGNMENT;
    }
#endif

    struct_sz = sizeof(MPID_Datatype_contents);
    types_sz  = cp->nr_types * sizeof(MPI_Datatype);
    ints_sz   = cp->nr_ints * sizeof(int);
    /* FIXME: Why is this here?  The value is never used.  Should it be? */
    aints_sz  = cp->nr_aints * sizeof(MPI_Aint);

    /* pad the struct, types, and ints before we allocate.
     *
     * note: it's not necessary that we pad the aints,
     *       because they are last in the region.
     */
    if ((epsilon = struct_sz % align_sz)) {
	struct_sz += align_sz - epsilon;
    }
    if ((epsilon = types_sz % align_sz)) {
	types_sz += align_sz - epsilon;
    }
    if ((epsilon = ints_sz % align_sz)) {
	ints_sz += align_sz - epsilon;
    }

    ptr = ((char *) cp) + struct_sz + types_sz + ints_sz;
    MPIU_Memcpy(user_aints, ptr, cp->nr_aints * sizeof(MPI_Aint));

    return;
}

void MPIDI_Datatype_get_contents_types(MPID_Datatype_contents *cp,
				       MPI_Datatype *user_types)
{
    char *ptr;
    int align_sz = 8, epsilon;
    int struct_sz;

#ifdef HAVE_MAX_STRUCT_ALIGNMENT
    if (align_sz > HAVE_MAX_STRUCT_ALIGNMENT) {
	align_sz = HAVE_MAX_STRUCT_ALIGNMENT;
    }
#endif

    struct_sz = sizeof(MPID_Datatype_contents);

    /* pad the struct, types, and ints before we allocate.
     *
     * note: it's not necessary that we pad the aints,
     *       because they are last in the region.
     */
    if ((epsilon = struct_sz % align_sz)) {
	struct_sz += align_sz - epsilon;
    }

    ptr = ((char *) cp) + struct_sz;
    MPIU_Memcpy(user_types, ptr, cp->nr_types * sizeof(MPI_Datatype));

    return;
}
