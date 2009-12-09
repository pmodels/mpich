/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMI_SERV_H_INCLUDED
#define PMI_SERV_H_INCLUDED

#include "pm_utils.h"

HYD_status HYD_pmcd_pmi_connect_cb(int fd, HYD_event_t events, void *userp);
HYD_status HYD_pmcd_pmi_cmd_cb(int fd, HYD_event_t events, void *userp);
HYD_status HYD_pmcd_pmi_serv_control_connect_cb(int fd, HYD_event_t events, void *userp);
HYD_status HYD_pmcd_pmi_serv_control_cb(int fd, HYD_event_t events, void *userp);
HYD_status HYD_pmcd_pmi_serv_cleanup(void);
HYD_status HYD_pmcd_pmi_serv_ckpoint(void);
void HYD_pmcd_pmi_serv_signal_cb(int signal);

#endif /* PMI_SERV_H_INCLUDED */
