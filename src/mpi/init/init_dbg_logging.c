/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpi_init.h"

#if defined (MPL_USE_DBG_LOGGING)
/* extern symbol declared in mpir_dbg.h */
MPL_dbg_class MPIR_DBG_INIT;
MPL_dbg_class MPIR_DBG_PT2PT;
MPL_dbg_class MPIR_DBG_THREAD;
MPL_dbg_class MPIR_DBG_DATATYPE;
MPL_dbg_class MPIR_DBG_HANDLE;
MPL_dbg_class MPIR_DBG_COMM;
MPL_dbg_class MPIR_DBG_BSEND;
MPL_dbg_class MPIR_DBG_ERRHAND;
MPL_dbg_class MPIR_DBG_COLL;
MPL_dbg_class MPIR_DBG_OTHER;
MPL_dbg_class MPIR_DBG_REQUEST;

/* these classes might need to move out later */
MPL_dbg_class MPIR_DBG_ASSERT;
MPL_dbg_class MPIR_DBG_STRING;

void MPII_pre_init_dbg_logging(int *argc, char ***argv)
{
    /* we are ignoring any argument error here as they shouldn't affect MPI operations */
    MPL_dbg_pre_init(argc, argv);
}

void MPII_init_dbg_logging(void)
{
    /* FIXME: This is a hack to handle the common case of two worlds.
     * If the parent comm is not NULL, we always give the world number
     * as "1" (false). */
    MPL_dbg_init(MPIR_Process.comm_parent != NULL, MPIR_Process.rank);

    MPIR_DBG_INIT = MPL_dbg_class_alloc("INIT", "init");
    MPIR_DBG_PT2PT = MPL_dbg_class_alloc("PT2PT", "pt2pt");
    MPIR_DBG_THREAD = MPL_dbg_class_alloc("THREAD", "thread");
    MPIR_DBG_DATATYPE = MPL_dbg_class_alloc("DATATYPE", "datatype");
    MPIR_DBG_HANDLE = MPL_dbg_class_alloc("HANDLE", "handle");
    MPIR_DBG_COMM = MPL_dbg_class_alloc("COMM", "comm");
    MPIR_DBG_BSEND = MPL_dbg_class_alloc("BSEND", "bsend");
    MPIR_DBG_ERRHAND = MPL_dbg_class_alloc("ERRHAND", "errhand");
    MPIR_DBG_OTHER = MPL_dbg_class_alloc("OTHER", "other");
    MPIR_DBG_REQUEST = MPL_dbg_class_alloc("REQUEST", "request");
    MPIR_DBG_COLL = MPL_dbg_class_alloc("COLL", "coll");

    MPIR_DBG_ASSERT = MPL_dbg_class_alloc("ASSERT", "assert");
    MPIR_DBG_STRING = MPL_dbg_class_alloc("STRING", "string");
}

#else

void MPII_pre_init_dbg_logging(int *argc, char ***argv)
{
}

void MPII_init_dbg_logging(void)
{
}

#endif /* MPL_USE_DBG_LOGGING */
