/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMISERV_UTILS_H_INCLUDED
#define PMISERV_UTILS_H_INCLUDED

#include "demux.h"
#include "pmiserv_pmi.h"

HYD_status HYD_pmcd_pmi_fill_in_proxy_args(char **proxy_args, char *control_port, int pgid);
HYD_status HYD_pmcd_pmi_fill_in_exec_launch_info(struct HYD_pg *pg);
HYD_status HYD_pmcd_pmi_alloc_pg_scratch(struct HYD_pg *pg);
HYD_status HYD_pmcd_pmi_free_pg_scratch(struct HYD_pg *pg);

#endif /* PMISERV_UTILS_H_INCLUDED */
