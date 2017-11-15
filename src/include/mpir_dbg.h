/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_DBG_H_INCLUDED
#define MPIR_DBG_H_INCLUDED

#if defined (MPL_USE_DBG_LOGGING)
extern MPL_dbg_class MPIR_DBG_INIT;
extern MPL_dbg_class MPIR_DBG_PT2PT;
extern MPL_dbg_class MPIR_DBG_THREAD;
extern MPL_dbg_class MPIR_DBG_DATATYPE;
extern MPL_dbg_class MPIR_DBG_COMM;
extern MPL_dbg_class MPIR_DBG_BSEND;
extern MPL_dbg_class MPIR_DBG_ERRHAND;
extern MPL_dbg_class MPIR_DBG_OTHER;
extern MPL_dbg_class MPIR_DBG_COLL;
extern MPL_dbg_class MPIR_DBG_REQUEST;
extern MPL_dbg_class MPIR_DBG_ASSERT;
#endif /* MPL_USE_DBG_LOGGING */

#endif /* MPIR_DBG_H_INCLUDED */
