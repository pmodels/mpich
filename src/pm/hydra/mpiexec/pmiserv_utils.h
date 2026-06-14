/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PMISERV_UTILS_H_INCLUDED
#define PMISERV_UTILS_H_INCLUDED

#include "demux.h"
#include "pmiserv_pmi.h"

HYD_status HYD_pmcd_pmi_fill_in_proxy_args(struct HYD_string_stash *stash,
                                           char *control_port, int pgid);
HYD_status HYD_pmcd_pmi_fill_in_exec_launch_info(struct HYD_pg *pg);

#endif /* PMISERV_UTILS_H_INCLUDED */
