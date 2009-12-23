/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMI_UTILS_H_INCLUDED
#define PMI_UTILS_H_INCLUDED

#include "demux.h"

HYD_status HYD_pmcd_pmi_fill_in_proxy_args(char **proxy_args, char *control_port, int pgid);
HYD_status HYD_pmcd_pmi_fill_in_exec_launch_info(char *pmi_port, int pmi_id,
                                                 struct HYD_pg *pg);
HYD_status HYD_pmcd_pmi_allocate_kvs(struct HYD_pmcd_pmi_kvs **kvs, int pgid);
HYD_status HYD_pmcd_init_pg_scratch(struct HYD_pg *pg);
HYD_status HYD_pmcd_pmi_alloc_pg_scratch(struct HYD_pg *pg);

#endif /* PMI_UTILS_H_INCLUDED */
