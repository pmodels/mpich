/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

static char *dbg_prefix = (char *) "unknown";

void HYDU_dump_prefix(FILE * fp)
{
    fprintf(fp, "[%s] ", dbg_prefix);
    fflush(fp);
}

void HYDU_dump_noprefix(FILE * fp, const char *str, ...)
{
    va_list list;

    va_start(list, str);
    vfprintf(fp, str, list);
    fflush(fp);
    va_end(list);
}

void HYDU_dump(FILE * fp, const char *str, ...)
{
    va_list list;

    va_start(list, str);
    HYDU_dump_prefix(fp);
    vfprintf(fp, str, list);
    fflush(fp);
    va_end(list);
}

HYD_status HYDU_dbg_init(const char *str)
{
    char hostname[MAX_HOSTNAME_LEN];
    HYD_status status = HYD_SUCCESS;

    status = HYDU_gethostname(hostname);
    HYDU_ERR_POP(status, "unable to get local host name\n");

    HYDU_MALLOC(dbg_prefix, char *, strlen(hostname) + 1 + strlen(str) + 1, status);
    HYDU_snprintf(dbg_prefix, strlen(hostname) + 1 + strlen(str) + 1, "%s@%s", str, hostname);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
