/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMI_HANDLE_V2_H_INCLUDED
#define PMI_HANDLE_V2_H_INCLUDED

#include "pmi_handle.h"

#define PMI_V2_DELIM ";"

extern struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_v2;

/* PMI handles */
HYD_status HYD_pmcd_pmi_handle_v2_fullinit(int fd, char *args[]);
HYD_status HYD_pmcd_pmi_handle_v2_job_getid(int fd, char *args[]);
HYD_status HYD_pmcd_pmi_handle_v2_info_putnodeattr(int fd, char *args[]);
HYD_status HYD_pmcd_pmi_handle_v2_info_getnodeattr(int fd, char *args[]);
HYD_status HYD_pmcd_pmi_handle_v2_info_getjobattr(int fd, char *args[]);
HYD_status HYD_pmcd_pmi_handle_v2_kvs_put(int fd, char *args[]);
HYD_status HYD_pmcd_pmi_handle_v2_kvs_get(int fd, char *args[]);
HYD_status HYD_pmcd_pmi_handle_v2_kvs_fence(int fd, char *args[]);
HYD_status HYD_pmcd_pmi_handle_v2_finalize(int fd, char *args[]);

#endif /* PMI_HANDLE_V2_H_INCLUDED */
