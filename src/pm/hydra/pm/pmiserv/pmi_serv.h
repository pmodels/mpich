/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMI_SERV_H_INCLUDED
#define PMI_SERV_H_INCLUDED

#include "pmi_common.h"

HYD_Status HYD_PMCD_pmi_connect_cb(int fd, HYD_Event_t events, void *userp);
HYD_Status HYD_PMCD_pmi_cmd_cb(int fd, HYD_Event_t events, void *userp);
HYD_Status HYD_PMCD_pmi_serv_control_cb(int fd, HYD_Event_t events, void *userp);
HYD_Status HYD_PMCD_pmi_serv_cleanup(void);
void HYD_PMCD_pmi_serv_signal_cb(int signal);

#endif /* PMI_SERV_H_INCLUDED */
