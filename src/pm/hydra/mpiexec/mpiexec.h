/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIEXEC_H_INCLUDED
#define MPIEXEC_H_INCLUDED

#include "hydra_server.h"

struct HYD_ui_mpich_info_s {
    int ppn;
    int timeout;
    int print_all_exitcodes;

    enum HYD_sort_order {
        NONE = 0,
        ASCENDING = 1,
        DESCENDING = 2
    } sort_order;

    int output_from;
    char *prepend_pattern;
    char *outfile_pattern;
    char *errfile_pattern;

    char *config_file;
    int reading_config_file;
    int hostname_propagation;
};

extern struct HYD_ui_mpich_info_s HYD_ui_mpich_info;
extern struct HYD_arg_match_table HYD_mpiexec_match_table[];
extern struct HYD_exec *HYD_uii_mpx_exec_list;

HYD_status HYD_uii_mpx_get_parameters(char **t_argv);
HYD_status HYD_uii_get_current_exec(struct HYD_exec **exec);

#endif /* MPIEXEC_H_INCLUDED */
