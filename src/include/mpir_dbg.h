/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
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
