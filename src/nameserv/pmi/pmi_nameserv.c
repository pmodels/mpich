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
    MPL_UNREFERENCED_ARG(info_ptr);
    MPL_UNREFERENCED_ARG(handle);

    return MPIR_pmi_publish(service_name, port);
}

int MPID_NS_Lookup(MPID_NS_Handle handle, const MPIR_Info * info_ptr,
                   const char service_name[], char port[])
{
    MPL_UNREFERENCED_ARG(info_ptr);
    MPL_UNREFERENCED_ARG(handle);

    return MPIR_pmi_lookup(service_name, port);
}

int MPID_NS_Unpublish(MPID_NS_Handle handle, const MPIR_Info * info_ptr, const char service_name[])
{
    MPL_UNREFERENCED_ARG(info_ptr);
    MPL_UNREFERENCED_ARG(handle);

    return MPIR_pmi_unpublish(service_name);
}

int MPID_NS_Free(MPID_NS_Handle * handle_ptr)
{
    /* MPID_NS_Handle is Null */
    MPL_UNREFERENCED_ARG(handle_ptr);
    return 0;
}
