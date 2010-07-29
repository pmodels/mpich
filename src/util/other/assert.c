/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#  define MPIU_ASSERT_FMT_MSG_MAX_SIZE 2048


/* assertion helper routines
 *
 * These exist to de-clutter the post-processed code and reduce the chance that
 * all of the assertion code will confuse the compiler into making bad
 * optimization decisions.  By the time one of these functions is called, the
 * assertion has already failed and we can do more-expensive things because we
 * are on the way out anyway. */

int MPIR_Assert_fail(const char *cond, const char *file_name, int line_num)
{
    MPL_VG_PRINTF_BACKTRACE("Assertion failed in file %s at line %d: %s\n",
                            file_name, line_num, cond);
    MPIU_Internal_error_printf("Assertion failed in file %s at line %d: %s\n",
                               file_name, line_num, cond);
    MPIU_DBG_MSG_FMT(ALL, TERSE,
                     (MPIU_DBG_FDEST,
                      "Assertion failed in file %s at line %d: %s",
                      file_name, line_num, cond));
    MPID_Abort(NULL, MPI_SUCCESS, 1, NULL);
    return MPI_ERR_INTERN; /* never get here, abort should kill us */
}

int MPIR_Assert_fail_fmt(const char *cond, const char *file_name, int line_num, const char *fmt, ...)
{
    char msg[MPIU_ASSERT_FMT_MSG_MAX_SIZE] = {'\0'};
    va_list vl;

    va_start(vl,fmt);
    vsnprintf(msg, sizeof(msg), fmt, vl); /* don't check rc, can't handle it anyway */

    MPL_VG_PRINTF_BACKTRACE("Assertion failed in file %s at line %d: %s\n",
                            file_name, line_num, cond);
    MPL_VG_PRINTF_BACKTRACE("%s\n", msg);

    MPIU_Internal_error_printf("Assertion failed in file %s at line %d: %s\n",
                               file_name, line_num, cond);
    MPIU_Internal_error_printf("%s\n", msg);

    MPIU_DBG_MSG_FMT(ALL, TERSE,
                     (MPIU_DBG_FDEST,
                      "Assertion failed in file %s at line %d: %s",
                      file_name, line_num, cond));
    MPIU_DBG_MSG_FMT(ALL, TERSE, (MPIU_DBG_FDEST,"%s",msg));

    MPID_Abort(NULL, MPI_SUCCESS, 1, NULL);
    return MPI_ERR_INTERN; /* never get here, abort should kill us */
}

