/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIT_ERR_H_INCLUDED
#define MPIT_ERR_H_INCLUDED

#include "mpichconf.h"

/* Rationale:
 *   MPI Tool interface need work even before MPI_Init or after MPI_FINALIZE,
 *   and it should never abort -- otherwise, it interferes normal MPI functions.
 *   MPIR_ERR_... routines will abort on failure by default. In addition,
 *   setting error code that relying on MPI error utility functions are also
 *   inapproprate, therefore, we have separate error macros for MPI tool interfaces.
 *
 *   On the other hand, due to the constraints, the error handling in tool
 *   interface are rather simplified. Since it cannot fail, the failing diagnosis
 *   message is not going to be used, and the application developers are supposed
 *   to check return values and handle errors according to their application logic.
 *
 *   To maintain a parallel interface with rest of the MPICH error handling calls,
 *   corresponding macros in mpir_err.h are ported here with MPIT_ prefix.
 */

#include <assert.h>
#define MPIT_Assert(cond_) assert(cond_);

#define MPIT_ERR_POP() \
    do { \
        goto fn_fail; \
    } while (0)

#define MPIT_ERRTEST_ARGNULL(arg) \
    if (arg == NULL) { \
        mpi_errno = MPI_T_ERR_INVALID; \
        goto fn_fail; \
    }

#define MPIT_ERRTEST_ARGNEG(arg) \
    if (arg < 0) { \
        mpi_errno = MPI_T_ERR_INVALID; \
        goto fn_fail; \
    }

#define MPIT_ERRTEST_MPIT_INITIALIZED() \
    if (!MPIR_T_is_initialized()) { \
        mpi_errno = MPI_T_ERR_NOT_INITIALIZED; \
        goto fn_fail; \
    }

#define MPIT_ERRTEST_CAT_INDEX(index_) \
    if ((index_) < 0 || ((unsigned) index_) >= utarray_len(cat_table)) { \
        mpi_errno = MPI_T_ERR_INVALID_INDEX; \
        goto fn_fail; \
    }

#define MPIT_ERRTEST_ENUM_HANDLE(handle_) \
    if ((handle_) == MPI_T_ENUM_NULL) { \
        mpi_errno = MPI_T_ERR_INVALID_HANDLE; \
        goto fn_fail; \
    } else if ((handle_)->kind != MPIR_T_ENUM_HANDLE) { \
        mpi_errno = MPI_T_ERR_INVALID_HANDLE; \
        goto fn_fail; \
    }

#define MPIT_ERRTEST_ENUM_ITEM(enum_, index_) \
    if ((index_) < 0 || ((unsigned) index_) >= utarray_len((enum_)->items)) { \
        mpi_errno = MPI_T_ERR_INVALID_ITEM; \
        goto fn_fail; \
    }

#define MPIT_ERRTEST_CVAR_INDEX(index_) \
    if ((index_) < 0 || ((unsigned) index_) >= utarray_len(cvar_table)) { \
        mpi_errno = MPI_T_ERR_INVALID_INDEX; \
        goto fn_fail; \
    }

#define MPIT_ERRTEST_CVAR_HANDLE(handle_) \
    if ((handle_) == MPI_T_CVAR_HANDLE_NULL) { \
        mpi_errno = MPI_T_ERR_INVALID_HANDLE; \
        goto fn_fail; \
    } else if ((handle_)->kind != MPIR_T_CVAR_HANDLE) { \
        mpi_errno = MPI_T_ERR_INVALID_HANDLE; \
        goto fn_fail; \
    }

#define MPIT_ERRTEST_PVAR_INDEX(index_) \
    if ((index_) < 0 || ((unsigned) index_) >= utarray_len(pvar_table)) { \
        mpi_errno = MPI_T_ERR_INVALID_INDEX; \
        goto fn_fail; \
    }

#define MPIT_ERRTEST_PVAR_HANDLE(handle_) \
    if (handle_ == MPI_T_PVAR_HANDLE_NULL) { \
        mpi_errno = MPI_T_ERR_INVALID_HANDLE; \
        goto fn_fail; \
    } else if ((handle_)->kind != MPIR_T_PVAR_HANDLE) { \
        mpi_errno = MPI_T_ERR_INVALID_HANDLE; \
        goto fn_fail; \
    }

#define MPIT_ERRTEST_PVAR_SESSION(session_) \
    if ((session_) == MPI_T_PVAR_SESSION_NULL) { \
        mpi_errno = MPI_T_ERR_INVALID_SESSION; \
        goto fn_fail; \
    } else if ((session_)->kind != MPIR_T_PVAR_SESSION) { \
        mpi_errno = MPI_T_ERR_INVALID_SESSION; \
        goto fn_fail; \
    }

#define MPIT_ERRTEST_PVAR_CLASS(class_) \
    if (class_ < MPIR_T_PVAR_CLASS_FIRST || class_ >= MPIR_T_PVAR_CLASS_LAST) { \
        mpi_errno = MPI_T_ERR_INVALID_NAME; \
        goto fn_fail; \
    }

#define MPIT_ERRTEST_EVENT_REG_HANDLE(handle_) \
    if ((handle_)->kind != MPIR_T_EVENT_REG) { \
        mpi_errno = MPI_T_ERR_INVALID_HANDLE; \
        goto fn_fail; \
    }

#define MPIT_ERRTEST_EVENT_INSTANCE_HANDLE(handle_) \
    if ((handle_)->kind != MPIR_T_EVENT_INSTANCE) { \
        mpi_errno = MPI_T_ERR_INVALID_HANDLE; \
        goto fn_fail; \
    }

#endif /* MPIT_ERR_H_INCLUDED */
