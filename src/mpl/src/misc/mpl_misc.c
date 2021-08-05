/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ft=c.mpich : */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl.h"

#if defined (MPL_HAVE_SYS_SYSINFO_H)
#include <sys/sysinfo.h>
#endif

#if defined (MPL_HAVE_UNISTD_H)
#include <unistd.h>
#endif

int MPL_get_nprocs(void)
{
#if defined (MPL_HAVE_GET_NPROCS)
    return get_nprocs();
#elif defined (MPL_HAVE_DECL__SC_NPROCESSORS_ONLN)
    int count = sysconf(_SC_NPROCESSORS_ONLN);
    return (count > 0) ? count : 0;
#else
    /* Neither MPL_HAVE_GET_NPROCS nor MPL_HAVE_DECL__SC_NPROCESSORS_ONLN are defined.
     * Should not reach here. */
    MPIR_Assert(0);
#endif
}
