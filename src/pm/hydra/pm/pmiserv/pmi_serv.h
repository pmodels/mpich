/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMI_SERV_H_INCLUDED
#define PMI_SERV_H_INCLUDED

/* The set of commands supported */
#define HYD_PMCD_CMD_KILLALL_PROCS      "kill_all_procs"
#define HYD_PMCD_CMD_KILLALL_PROXIES    "kill_all_proxies"
#define HYD_PMCD_CMD_LAUNCH_PROCS       "launch_procs"
#define HYD_PMCD_CMD_SHUTDOWN           "shutdown"

#define HYD_PMCD_CMD_SEP_CHAR   ';'
#define HYD_PMCD_CMD_ENV_SEP_CHAR   ','

#define HYD_PMCD_MAX_CMD_LEN    1024

extern int HYD_PMCD_pmi_serv_listenfd;

HYD_Status HYD_PMCD_pmi_serv_cb(int fd, HYD_Event_t events, void *userp);
HYD_Status HYD_PMCD_pmi_serv_cleanup(void);
void HYD_PMCD_pmi_serv_signal_cb(int signal);

#endif /* PMI_SERV_H_INCLUDED */
