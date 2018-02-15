/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef PTL_IMPL_H_INCLUDED
#define PTL_IMPL_H_INCLUDED

#include <mpidimpl.h>
#include "portals4.h"

#define MPIDI_PTL_CASE_STR(x) case x: return #x

static inline const char *MPIDI_PTL_strerror(int ret)
{
    switch (ret) {
            MPIDI_PTL_CASE_STR(PTL_OK);
            MPIDI_PTL_CASE_STR(PTL_ARG_INVALID);
            MPIDI_PTL_CASE_STR(PTL_CT_NONE_REACHED);
            MPIDI_PTL_CASE_STR(PTL_EQ_DROPPED);
            MPIDI_PTL_CASE_STR(PTL_EQ_EMPTY);
            MPIDI_PTL_CASE_STR(PTL_FAIL);
            MPIDI_PTL_CASE_STR(PTL_IN_USE);
            MPIDI_PTL_CASE_STR(PTL_INTERRUPTED);
            MPIDI_PTL_CASE_STR(PTL_IGNORED);
            MPIDI_PTL_CASE_STR(PTL_LIST_TOO_LONG);
            MPIDI_PTL_CASE_STR(PTL_NO_INIT);
            MPIDI_PTL_CASE_STR(PTL_NO_SPACE);
            MPIDI_PTL_CASE_STR(PTL_PID_IN_USE);
            MPIDI_PTL_CASE_STR(PTL_PT_FULL);
            MPIDI_PTL_CASE_STR(PTL_PT_EQ_NEEDED);
            MPIDI_PTL_CASE_STR(PTL_PT_IN_USE);
        default:
            return "UNKNOWN";
    }
}

#define MPIDI_PTL_ERR  MPIR_ERR_CHKANDJUMP4

#define MPIDI_PTL_CHK_STATUS(STATUS,STR)                        \
    do {                                                                \
        MPIDI_PTL_ERR(STATUS!=PTL_OK,                           \
                              mpi_errno,                                \
                              MPI_ERR_OTHER,                            \
                              "**"#STR,                                 \
                              "**"#STR" %s %d %s %s",                   \
                              __FILE__,                                 \
                              __LINE__,                                 \
                              FCNAME,                                   \
                              MPIDI_PTL_strerror(STATUS));      \
    } while (0)

#endif /* PTL_IMPL_H_INCLUDED */
