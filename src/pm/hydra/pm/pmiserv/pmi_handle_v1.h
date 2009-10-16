/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMI_HANDLE_V1_H_INCLUDED
#define PMI_HANDLE_V1_H_INCLUDED

#include "pmi_handle.h"

#define PMI_V1_DELIM " "

extern struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_v1;

/* PMI handles */
HYD_status HYD_pmcd_pmi_handle_v1_initack(int fd, char *args[]);
HYD_status HYD_pmcd_pmi_handle_v1_get_maxes(int fd, char *args[]);
HYD_status HYD_pmcd_pmi_handle_v1_get_appnum(int fd, char *args[]);
HYD_status HYD_pmcd_pmi_handle_v1_get_my_kvsname(int fd, char *args[]);
HYD_status HYD_pmcd_pmi_handle_v1_barrier_in(int fd, char *args[]);
HYD_status HYD_pmcd_pmi_handle_v1_put(int fd, char *args[]);
HYD_status HYD_pmcd_pmi_handle_v1_get(int fd, char *args[]);
HYD_status HYD_pmcd_pmi_handle_v1_get_usize(int fd, char *args[]);
HYD_status HYD_pmcd_pmi_handle_v1_finalize(int fd, char *args[]);

#endif /* PMI_HANDLE_V1_H_INCLUDED */
