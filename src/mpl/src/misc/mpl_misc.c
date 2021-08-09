/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpl.h"
#include <assert.h>


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
#elif defined (MPL_HAVE_DECL__SC_NPROCESSORS_ONLN) && MPL_HAVE_DECL__SC_NPROCESSORS_ONLN
    int count = sysconf(_SC_NPROCESSORS_ONLN);
    return (count > 0) ? count : 1;
#else
    /* Neither MPL_HAVE_GET_NPROCS nor MPL_HAVE_DECL__SC_NPROCESSORS_ONLN are defined.
     * Should not reach here. */
    assert(0);
    return 1;
#endif
}
