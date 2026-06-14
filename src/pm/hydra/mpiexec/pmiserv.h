/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PMISERV_H_INCLUDED
#define PMISERV_H_INCLUDED

#include "pmiserv_common.h"

HYD_status HYD_pmcd_pmiserv_proxy_init_cb(int fd, HYD_event_t events, void *userp);
HYD_status HYD_pmcd_pmiserv_control_listen_cb(int fd, HYD_event_t events, void *userp);
HYD_status HYD_pmcd_pmiserv_cleanup_all_pgs(void);
HYD_status HYD_pmcd_pmiserv_send_signal(struct HYD_proxy *proxy, int signum);
HYD_status HYD_control_listen(void);
HYD_status HYD_send_exec_info(struct HYD_proxy *proxy);

#endif /* PMISERV_H_INCLUDED */
