/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIEXEC_H_INCLUDED
#define MPIEXEC_H_INCLUDED

#include "hydra_server.h"

struct HYD_ui_mpich_info_s {
    int ppn;
    int print_all_exitcodes;

    enum HYD_sort_order {
        NONE = 0,
        ASCENDING = 1,
        DESCENDING = 2
    } sort_order;
};

extern struct HYD_ui_mpich_info_s HYD_ui_mpich_info;

HYD_status HYD_uii_mpx_get_parameters(char **t_argv);

extern struct HYD_exec *HYD_uii_mpx_exec_list;

#endif /* MPIEXEC_H_INCLUDED */
