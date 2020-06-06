/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_WAIT_H_INCLUDED
#define CH4_WAIT_H_INCLUDED

#include "ch4_impl.h"

/* MPID_Test, MPID_Testall, MPID_Testany, MPID_Testsome */

MPL_STATIC_INLINE_PREFIX int MPID_Test(MPIR_Request * request_ptr, int *flag, MPI_Status * status)
{
    return MPIR_Test_impl(request_ptr, flag, status);
}

MPL_STATIC_INLINE_PREFIX int MPID_Testall(int count, MPIR_Request * request_ptrs[],
                                          int *flag, MPI_Status array_of_statuses[],
                                          int requests_property)
{
    return MPIR_Testall_impl(count, request_ptrs, flag, array_of_statuses, requests_property);
}

MPL_STATIC_INLINE_PREFIX int MPID_Testany(int count, MPIR_Request * request_ptrs[],
                                          int *indx, int *flag, MPI_Status * status)
{
    return MPIR_Testany_impl(count, request_ptrs, indx, flag, status);
}

MPL_STATIC_INLINE_PREFIX int MPID_Testsome(int incount, MPIR_Request * request_ptrs[],
                                           int *outcount, int array_of_indices[],
                                           MPI_Status array_of_statuses[])
{
    return MPIR_Testsome_impl(incount, request_ptrs, outcount, array_of_indices, array_of_statuses);
}

/* MPID_Wait, MPID_Waitall, MPID_Waitany, MPID_Waitsome */

MPL_STATIC_INLINE_PREFIX int MPID_Wait(MPIR_Request * request_ptr, MPI_Status * status)
{
    return MPIR_Wait_impl(request_ptr, status);
}

MPL_STATIC_INLINE_PREFIX int MPID_Waitall(int count, MPIR_Request * request_ptrs[],
                                          MPI_Status array_of_statuses[], int request_properties)
{
    return MPIR_Waitall_impl(count, request_ptrs, array_of_statuses, request_properties);
}

MPL_STATIC_INLINE_PREFIX int MPID_Waitany(int count, MPIR_Request * request_ptrs[],
                                          int *indx, MPI_Status * status)
{
    return MPIR_Waitany_impl(count, request_ptrs, indx, status);
}

MPL_STATIC_INLINE_PREFIX int MPID_Waitsome(int incount, MPIR_Request * request_ptrs[],
                                           int *outcount, int array_of_indices[],
                                           MPI_Status array_of_statuses[])
{
    return MPIR_Waitsome_impl(incount, request_ptrs, outcount, array_of_indices, array_of_statuses);
}

#endif /* CH4_WAIT_H_INCLUDED */
