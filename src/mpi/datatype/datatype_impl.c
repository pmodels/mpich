/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Get_address_impl(const void *location, MPI_Aint * address)
{
    *address = (MPI_Aint) location;
    return MPI_SUCCESS;
}

int MPIR_Get_count_impl(const MPI_Status * status, MPI_Datatype datatype, MPI_Aint * count)
{
    MPI_Aint size;

    MPIR_Datatype_get_size_macro(datatype, size);
    MPIR_Assert(size >= 0 && MPIR_STATUS_GET_COUNT(*status) >= 0);
    if (size != 0) {
        /* MPI-3 says return MPI_UNDEFINED if too large for an int */
        if ((MPIR_STATUS_GET_COUNT(*status) % size) != 0)
            (*count) = MPI_UNDEFINED;
        else
            (*count) = (MPI_Aint) (MPIR_STATUS_GET_COUNT(*status) / size);
    } else {
        if (MPIR_STATUS_GET_COUNT(*status) > 0) {
            /* --BEGIN ERROR HANDLING-- */

            /* case where datatype size is 0 and count is > 0 should
             * never occur.
             */

            (*count) = MPI_UNDEFINED;
            /* --END ERROR HANDLING-- */
        } else {
            /* This is ambiguous.  However, discussions on MPI Forum
             * reached a consensus that this is the correct return
             * value
             */
            (*count) = 0;
        }
    }
    return MPI_SUCCESS;
}

int MPIR_Pack_impl(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                   void *outbuf, MPI_Aint outsize, MPI_Aint * position, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint actual_pack_bytes;
    void *buf = (void *) ((char *) outbuf + *position);
    mpi_errno = MPIR_Typerep_pack(inbuf, incount, datatype, 0, buf, outsize, &actual_pack_bytes,
                                  MPIR_TYPEREP_FLAG_NONE);
    MPIR_ERR_CHECK(mpi_errno);
    *position += actual_pack_bytes;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Pack_external_impl(const char datarep[],
                            const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                            void *outbuf, MPI_Aint outsize, MPI_Aint * position)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint actual_pack_bytes;
    void *buf = (void *) ((char *) outbuf + *position);
    mpi_errno = MPIR_Typerep_pack_external(inbuf, incount, datatype, buf, &actual_pack_bytes);
    MPIR_ERR_CHECK(mpi_errno);
    *position += actual_pack_bytes;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Unpack_impl(const void *inbuf, MPI_Aint insize, MPI_Aint * position,
                     void *outbuf, MPI_Aint outcount, MPI_Datatype datatype, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint actual_unpack_bytes;
    void *buf = (void *) ((char *) inbuf + *position);
    mpi_errno =
        MPIR_Typerep_unpack(buf, insize, outbuf, outcount, datatype, 0, &actual_unpack_bytes,
                            MPIR_TYPEREP_FLAG_NONE);
    MPIR_ERR_CHECK(mpi_errno);
    *position += actual_unpack_bytes;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Unpack_external_impl(const char datarep[],
                              const void *inbuf, MPI_Aint insize, MPI_Aint * position,
                              void *outbuf, MPI_Aint outcount, MPI_Datatype datatype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint actual_unpack_bytes;
    void *buf = (void *) ((char *) inbuf + *position);
    mpi_errno = MPIR_Typerep_unpack_external(buf, outbuf, outcount, datatype, &actual_unpack_bytes);
    MPIR_ERR_CHECK(mpi_errno);
    *position += actual_unpack_bytes;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void MPIR_Pack_size(MPI_Aint incount, MPI_Datatype datatype, MPI_Aint * size)
{
    MPI_Aint typesize;
    MPIR_Datatype_get_size_macro(datatype, typesize);
    *size = incount * typesize;
}

int MPIR_Pack_size_impl(MPI_Aint incount, MPI_Datatype datatype, MPIR_Comm * comm_ptr,
                        MPI_Aint * size)
{
    MPIR_Pack_size(incount, datatype, size);
    return MPI_SUCCESS;
}

int MPIR_Pack_external_size_impl(const char *datarep,
                                 MPI_Aint incount, MPI_Datatype datatype, MPI_Aint * size)
{
    *size = incount * MPIR_Typerep_size_external32(datatype);
    return MPI_SUCCESS;
}

int MPIR_Status_set_elements_x_impl(MPI_Status * status, MPI_Datatype datatype, MPI_Count count)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Count size_x;

    MPIR_Datatype_get_size_macro(datatype, size_x);

    /* overflow check, should probably be a real error check? */
    if (count != 0) {
        MPIR_Assert(size_x >= 0 && count > 0);
        MPIR_Assert(count * size_x < MPIR_COUNT_MAX);
    }

    MPIR_STATUS_SET_COUNT(*status, size_x * count);

    return mpi_errno;
}

int MPIR_Type_commit_impl(MPI_Datatype * datatype_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *datatype_ptr;

    MPIR_Assert(!HANDLE_IS_BUILTIN(*datatype_p));

    MPIR_Datatype_get_ptr(*datatype_p, datatype_ptr);

    if (datatype_ptr->is_committed == 0) {
        datatype_ptr->is_committed = 1;

        MPIR_Typerep_commit(*datatype_p);

        MPL_DBG_MSG_D(MPIR_DBG_DATATYPE, TERSE, "# contig blocks = %d\n",
                      (int) datatype_ptr->typerep.num_contig_blocks);

        MPID_Type_commit_hook(datatype_ptr);

    }
    return mpi_errno;
}

int MPIR_Type_free_impl(MPI_Datatype * datatype)
{
    MPIR_Datatype *datatype_ptr = NULL;

    MPIR_Datatype_get_ptr(*datatype, datatype_ptr);
    MPIR_Assert(datatype_ptr);
    MPIR_Datatype_ptr_release(datatype_ptr);
    *datatype = MPI_DATATYPE_NULL;

    return MPI_SUCCESS;
}

int MPIR_Type_get_extent_impl(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent)
{
    MPI_Count lb_x, extent_x;

    MPIR_Type_get_extent_x_impl(datatype, &lb_x, &extent_x);
    *lb = (lb_x > MPIR_AINT_MAX) ? MPI_UNDEFINED : (MPI_Aint) lb_x;
    *extent = (extent_x > MPIR_AINT_MAX) ? MPI_UNDEFINED : (MPI_Aint) extent_x;
    return MPI_SUCCESS;
}

int MPIR_Type_get_extent_x_impl(MPI_Datatype datatype, MPI_Count * lb, MPI_Count * extent)
{
    if (HANDLE_IS_BUILTIN(datatype)) {
        *lb = 0;
        *extent = MPIR_Datatype_get_basic_size(datatype);
    } else {
        MPIR_Datatype *datatype_ptr = NULL;
        MPIR_Datatype_get_ptr(datatype, datatype_ptr);
        *lb = datatype_ptr->lb;
        *extent = datatype_ptr->extent; /* derived, should be same as ub - lb */
    }
    return MPI_SUCCESS;
}

int MPIR_Type_get_true_extent_impl(MPI_Datatype datatype, MPI_Aint * true_lb,
                                   MPI_Aint * true_extent)
{
    MPI_Count true_lb_x, true_extent_x;

    MPIR_Type_get_true_extent_x_impl(datatype, &true_lb_x, &true_extent_x);
    *true_lb = (true_lb_x > MPIR_AINT_MAX) ? MPI_UNDEFINED : (MPI_Aint) true_lb_x;
    *true_extent = (true_extent_x > MPIR_AINT_MAX) ? MPI_UNDEFINED : (MPI_Aint) true_extent_x;

    return MPI_SUCCESS;
}

/* any non-MPI functions go here, especially non-static ones */

int MPIR_Type_get_true_extent_x_impl(MPI_Datatype datatype, MPI_Count * true_lb,
                                     MPI_Count * true_extent)
{
    if (HANDLE_IS_BUILTIN(datatype)) {
        *true_lb = 0;
        *true_extent = MPIR_Datatype_get_basic_size(datatype);
    } else {
        MPIR_Datatype *datatype_ptr = NULL;
        MPIR_Datatype_get_ptr(datatype, datatype_ptr);
        *true_lb = datatype_ptr->true_lb;
        *true_extent = datatype_ptr->true_ub - datatype_ptr->true_lb;
    }

    return MPI_SUCCESS;
}

int MPIR_Type_get_value_index_impl(MPI_Datatype value_type, MPI_Datatype index_type,
                                   MPI_Datatype * pair_type)
{
    int mpi_errno = MPI_SUCCESS;

    if (index_type == MPI_INT) {
        if (value_type == MPI_FLOAT) {
            *pair_type = MPI_FLOAT_INT;
        } else if (value_type == MPI_DOUBLE) {
            *pair_type = MPI_DOUBLE_INT;
        } else if (value_type == MPI_LONG) {
            *pair_type = MPI_LONG_INT;
        } else if (value_type == MPI_SHORT) {
            *pair_type = MPI_SHORT_INT;
        } else if (value_type == MPI_INT) {
            *pair_type = MPI_2INT;
        } else if (value_type == MPI_LONG_DOUBLE) {
            *pair_type = MPI_LONG_DOUBLE_INT;
        } else {
            *pair_type = MPI_DATATYPE_NULL;
        }
    } else {
        *pair_type = MPI_DATATYPE_NULL;
    }

    return mpi_errno;
}

int MPIR_Type_size_impl(MPI_Datatype datatype, MPI_Aint * size)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Datatype_get_size_macro(datatype, *size);
    MPIR_Assert(*size >= 0);

    return mpi_errno;
}

int MPIR_Type_size_x_impl(MPI_Datatype datatype, MPI_Count * size)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Datatype_get_size_macro(datatype, *size);

    return mpi_errno;
}

static MPI_Datatype type_match_size(MPI_Datatype type_list[], int nTypes, int size)
{
    for (int i = 0; i < nTypes; i++) {
        MPI_Aint tsize;
        MPIR_Datatype_get_size_macro(type_list[i], tsize);
        if (tsize == size) {
            return type_list[i];
        }
    }
    return MPI_DATATYPE_NULL;
}


int MPIR_Type_match_size_impl(int typeclass, int size, MPI_Datatype * datatype)
{
    int mpi_errno = MPI_SUCCESS;

    /* Note that all of the datatype have values, even if the type is
     * not available. We test for that case separately.  We also
     * prefer the Fortran types to the C type, if they are available */
    static MPI_Datatype real_types[] = {
        MPI_REAL2, MPI_REAL4, MPI_REAL8, MPI_REAL16,
    };
    static MPI_Datatype int_types[] = {
        MPI_INTEGER1, MPI_INTEGER2, MPI_INTEGER4, MPI_INTEGER8, MPI_INTEGER16,
    };
    static MPI_Datatype complex_types[] = {
        MPI_COMPLEX4, MPI_COMPLEX8, MPI_COMPLEX16, MPI_COMPLEX32,
    };
    static MPI_Datatype logical_types[] = {
        MPI_LOGICAL1, MPI_LOGICAL2, MPI_LOGICAL4, MPI_LOGICAL8, MPI_LOGICAL16,
    };


    /* The following implementation follows the suggestion in the
     * MPI-2 standard.
     * The version in the MPI-2 spec makes use of the Fortran optional types;
     * currently, we don't support these from C (see mpi.h.in).
     * Thus, we look at the candidate types and make use of the first fit.
     * Note that the standard doesn't require that this routine return
     * any particular choice of MPI datatype; e.g., it is not required
     * to return MPI_INTEGER4 if a 4-byte integer is requested.
     */
    int n;
    MPI_Datatype matched_datatype = MPI_DATATYPE_NULL;
#ifdef HAVE_ERROR_CHECKING
    const char *tname = NULL;
#endif

    switch (typeclass) {
        case MPI_TYPECLASS_REAL:
            n = sizeof(real_types) / sizeof(MPI_Datatype);
            matched_datatype = type_match_size(real_types, n, size);
#ifdef HAVE_ERROR_CHECKING
            tname = "MPI_TYPECLASS_REAL";
#endif
            break;
        case MPI_TYPECLASS_INTEGER:
            n = sizeof(int_types) / sizeof(MPI_Datatype);
            matched_datatype = type_match_size(int_types, n, size);
#ifdef HAVE_ERROR_CHECKING
            tname = "MPI_TYPECLASS_INTEGER";
#endif
            break;
        case MPI_TYPECLASS_COMPLEX:
            n = sizeof(complex_types) / sizeof(MPI_Datatype);
            matched_datatype = type_match_size(complex_types, n, size);
#ifdef HAVE_ERROR_CHECKING
            tname = "MPI_TYPECLASS_COMPLEX";
#endif
            break;
        case MPI_TYPECLASS_LOGICAL:
            n = sizeof(logical_types) / sizeof(MPI_Datatype);
            matched_datatype = type_match_size(logical_types, n, size);
#ifdef HAVE_ERROR_CHECKING
            tname = "MPI_TYPECLASS_LOGICAL";
#endif
            break;
        default:
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_ARG, break, "**typematchnoclass");
    }

    if (matched_datatype == MPI_DATATYPE_NULL) {
        MPIR_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_ARG, "**typematchsize", "**typematchsize %s %d",
                             tname, size);
    } else {
        *datatype = matched_datatype;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_get_name_impl(MPIR_Datatype * datatype_ptr, char *type_name, int *resultlen)
{
    if (datatype_ptr == NULL) {
        MPL_strncpy(type_name, "MPI_DATATYPE_NULL", MPI_MAX_OBJECT_NAME);
    } else {
        /* Include the null in MPI_MAX_OBJECT_NAME */
        MPL_strncpy(type_name, datatype_ptr->name, MPI_MAX_OBJECT_NAME);
    }
    *resultlen = (int) strlen(type_name);

    return MPI_SUCCESS;
}

int MPIR_Type_set_name_impl(MPIR_Datatype * datatype_ptr, const char *type_name)
{
    /* Include the null in MPI_MAX_OBJECT_NAME */
    MPL_strncpy(datatype_ptr->name, type_name, MPI_MAX_OBJECT_NAME);

    return MPI_SUCCESS;
}
