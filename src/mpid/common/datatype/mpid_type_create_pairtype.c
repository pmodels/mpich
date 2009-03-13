/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>
#include <limits.h>

/* PAIRTYPE_SIZE_EXTENT - calculates size, extent, etc. for pairtype by
 * defining the appropriate C type.
 */
#define PAIRTYPE_SIZE_EXTENT(mt1_,ut1_,mt2_,ut2_)			\
    {									\
	struct { ut1_ a; ut2_ b; } foo;					\
	type_size   = sizeof(foo.a) + sizeof(foo.b);			\
	type_extent = (MPI_Aint) sizeof(foo);				\
	el_size = (sizeof(foo.a) == sizeof(foo.b)) ? (int) sizeof(foo.a) : -1; \
	true_ub = (MPI_VOID_PTR_CAST_TO_MPI_AINT ((char *) &foo.b -     \
                                                  (char *) &foo.a)) +   \
                  (MPI_Aint) sizeof(foo.b);                             \
	alignsize = MPIR_MAX(MPID_Datatype_get_basic_size(mt1_),	\
                             MPID_Datatype_get_basic_size(mt2_));	\
    }

/*@
  MPID_Type_create_pairtype - create necessary data structures for certain
  pair types (all but MPI_2INT etc., which never have the size != extent
  issue).

  This function is different from the other MPID_Type_create functions in that
  it fills in an already- allocated MPID_Datatype.  This is important for
  allowing configure-time determination of the MPI type values (these types
  are stored in the "direct" space, for those familiar with how MPICH2 deals
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
int MPID_Type_create_pairtype(MPI_Datatype type,
			      MPID_Datatype *new_dtp)
{
    int err, mpi_errno = MPI_SUCCESS;
    int type_size, alignsize;
    MPI_Aint type_extent, true_ub, el_size;

    /* handle is filled in by MPIU_Handle_obj_alloc() */
    MPIU_Object_set_ref(new_dtp, 1);
    new_dtp->is_permanent = 1;
    new_dtp->is_committed = 1; /* predefined types are pre-committed */
    new_dtp->attributes   = NULL;
    new_dtp->cache_id     = 0;
    new_dtp->name[0]      = 0;
    new_dtp->contents     = NULL;

    new_dtp->dataloop       = NULL;
    new_dtp->dataloop_size  = -1;
    new_dtp->dataloop_depth = -1;
    new_dtp->hetero_dloop       = NULL;
    new_dtp->hetero_dloop_size  = -1;
    new_dtp->hetero_dloop_depth = -1;

    switch(type) {
	case MPI_FLOAT_INT:
	    PAIRTYPE_SIZE_EXTENT(MPI_FLOAT, float, MPI_INT, int);
	    break;
	case MPI_DOUBLE_INT:
	    PAIRTYPE_SIZE_EXTENT(MPI_DOUBLE, double, MPI_INT, int);
	    break;
	case MPI_LONG_INT:
	    PAIRTYPE_SIZE_EXTENT(MPI_LONG, long, MPI_INT, int);
	    break;
	case MPI_SHORT_INT:
	    PAIRTYPE_SIZE_EXTENT(MPI_SHORT, short, MPI_INT, int);
	    break;
	case MPI_LONG_DOUBLE_INT:
	    PAIRTYPE_SIZE_EXTENT(MPI_LONG_DOUBLE, long double, MPI_INT, int);
	    break;
	default:
	    /* --BEGIN ERROR HANDLING-- */
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
					     MPIR_ERR_RECOVERABLE,
					     "MPID_Type_create_pairtype",
					     __LINE__,
					     MPI_ERR_OTHER,
					     "**dtype", 0);
	    return mpi_errno;
	    /* --END ERROR HANDLING-- */
    }

    new_dtp->n_elements      = 2;
    new_dtp->element_size    = el_size;
    new_dtp->eltype          = MPI_DATATYPE_NULL;

    new_dtp->has_sticky_lb   = 0;
    new_dtp->true_lb         = 0;
    new_dtp->lb              = 0;

    new_dtp->has_sticky_ub   = 0;
    new_dtp->true_ub         = true_ub;

    new_dtp->size            = type_size;
    new_dtp->ub              = type_extent; /* possible padding */
    new_dtp->extent          = type_extent;
    new_dtp->alignsize       = alignsize;

   /* place maximum on alignment based on padding rules */
    /* There are some really wierd rules for structure alignment;
       these capture the ones of which we are aware. */
    switch(type) {
	case MPI_SHORT_INT:
	case MPI_LONG_INT:
#ifdef HAVE_MAX_INTEGER_ALIGNMENT
	    new_dtp->alignsize       = MPIR_MIN(new_dtp->alignsize,
						HAVE_MAX_INTEGER_ALIGNMENT);
#endif
	    break;
	case MPI_FLOAT_INT:
#ifdef HAVE_MAX_FP_ALIGNMENT
	    new_dtp->alignsize       = MPIR_MIN(new_dtp->alignsize,
						HAVE_MAX_FP_ALIGNMENT);
#endif
	    break;
	case MPI_DOUBLE_INT:
#ifdef HAVE_MAX_DOUBLE_FP_ALIGNMENT
	    new_dtp->alignsize       = MPIR_MIN(new_dtp->alignsize,
						HAVE_MAX_DOUBLE_FP_ALIGNMENT);
#elif defined(HAVE_MAX_FP_ALIGNMENT)
	    new_dtp->alignsize       = MPIR_MIN(new_dtp->alignsize,
						HAVE_MAX_FP_ALIGNMENT);
#endif
	    break;
	case MPI_LONG_DOUBLE_INT:
#ifdef HAVE_MAX_LONG_DOUBLE_FP_ALIGNMENT
	    new_dtp->alignsize       = MPIR_MIN(new_dtp->alignsize,
					HAVE_MAX_LONG_DOUBLE_FP_ALIGNMENT);
#elif defined(HAVE_MAX_FP_ALIGNMENT)
	    new_dtp->alignsize       = MPIR_MIN(new_dtp->alignsize,
						HAVE_MAX_FP_ALIGNMENT);
#endif
	    break;
    }

    new_dtp->is_contig       = (((MPI_Aint) type_size) == type_extent) ? 1 : 0;
    new_dtp->max_contig_blocks = (((MPI_Aint) type_size) == type_extent) ? 1 : 2;

    /* fill in dataloops -- only case where we precreate dataloops
     *
     * this is necessary because these types aren't committed by the
     * user, which is the other place where we create dataloops. so
     * if the user uses one of these w/out building some more complex
     * type and then committing it, then the dataloop will be missing.
     */

#ifdef MPID_NEEDS_DLOOP_ALL_BYTES
    /* If MPID implementation needs use to reduce everything to
       a byte stream, do that. */
    err = MPID_Dataloop_create_pairtype(type,
					&(new_dtp->dataloop),
					&(new_dtp->dataloop_size),
					&(new_dtp->dataloop_depth),
					MPID_DATALOOP_ALL_BYTES);
#else
    err = MPID_Dataloop_create_pairtype(type,
					&(new_dtp->dataloop),
					&(new_dtp->dataloop_size),
					&(new_dtp->dataloop_depth),
					MPID_DATALOOP_HOMOGENEOUS);
#endif

    if (!err) {
	err = MPID_Dataloop_create_pairtype(type,
					    &(new_dtp->hetero_dloop),
					    &(new_dtp->hetero_dloop_size),
					    &(new_dtp->hetero_dloop_depth),
					    MPID_DATALOOP_HETEROGENEOUS);
    }

    /* --BEGIN ERROR HANDLING-- */
    if (err) {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
					 MPIR_ERR_RECOVERABLE,
					 "MPID_Dataloop_create_pairtype",
					 __LINE__,
					 MPI_ERR_OTHER,
					 "**nomem",
					 0);
	return mpi_errno;

    }
    /* --END ERROR HANDLING-- */

    return mpi_errno;
}
