/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * This file contains a simple PMI-based implementation of the name server routines.
 */

#include "mpiimpl.h"
#include "namepub.h"

/* style: allow:fprintf:1 sig:0 */
/* For writing the name/service pair */

/* Define the name service handle */
#define MPID_MAX_NAMEPUB 64
struct MPID_NS_Handle {
    int dummy;
};                              /* unused for now */

int MPID_NS_Create(const MPIR_Info * info_ptr, MPID_NS_Handle * handle_ptr)
{
    static struct MPID_NS_Handle nsHandleWithNoData;

    MPL_UNREFERENCED_ARG(info_ptr);
    /* MPID_NS_Create() should always create a valid handle */
    *handle_ptr = &nsHandleWithNoData;  /* The name service needs no local data */
    return 0;
}

int MPID_NS_Publish(MPID_NS_Handle handle, const MPIR_Info * info_ptr,
                    const char service_name[], const char port[])
{
    int mpi_errno = MPI_SUCCESS;
    int rc;
    MPL_UNREFERENCED_ARG(info_ptr);
    MPL_UNREFERENCED_ARG(handle);

#ifdef USE_PMI2_API
    /* release the global CS for PMI calls */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    rc = PMI2_Nameserv_publish(service_name, NULL, port);
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
#elif defined(USE_PMIX_API)
    MPIR_Assert(0);
#else
    rc = PMI_Publish_name(service_name, port);
#endif
    MPIR_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_NAME, "**namepubnotpub", "**namepubnotpub %s",
                         service_name);

  fn_fail:
    return mpi_errno;
}

int MPID_NS_Lookup(MPID_NS_Handle handle, const MPIR_Info * info_ptr,
                   const char service_name[], char port[])
{
    int mpi_errno = MPI_SUCCESS;
    int rc;
    MPL_UNREFERENCED_ARG(info_ptr);
    MPL_UNREFERENCED_ARG(handle);

#ifdef USE_PMI2_API
    /* release the global CS for PMI calls */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    rc = PMI2_Nameserv_lookup(service_name, NULL, port, MPI_MAX_PORT_NAME);
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
#elif defined(USE_PMIX_API)
    MPIR_Assert(0);
#else
    rc = PMI_Lookup_name(service_name, port);
#endif
    MPIR_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_NAME, "**namepubnotfound", "**namepubnotfound %s",
                         service_name);

  fn_fail:
    return mpi_errno;
}

int MPID_NS_Unpublish(MPID_NS_Handle handle, const MPIR_Info * info_ptr, const char service_name[])
{
    int mpi_errno = MPI_SUCCESS;
    int rc;
    MPL_UNREFERENCED_ARG(info_ptr);
    MPL_UNREFERENCED_ARG(handle);

#ifdef USE_PMI2_API
    /* release the global CS for PMI calls */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    rc = PMI2_Nameserv_unpublish(service_name, NULL);
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
#elif defined(USE_PMIX_API)
    MPIR_Assert(0);
#else
    rc = PMI_Unpublish_name(service_name);
#endif
    MPIR_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_SERVICE, "**namepubnotunpub",
                         "**namepubnotunpub %s", service_name);

  fn_fail:
    return mpi_errno;
}

int MPID_NS_Free(MPID_NS_Handle * handle_ptr)
{
    /* MPID_NS_Handle is Null */
    MPL_UNREFERENCED_ARG(handle_ptr);
    return 0;
}
