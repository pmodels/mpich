/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_ARG_H_INCLUDED
#define HYDRA_ARG_H_INCLUDED

#include "hydra_base.h"

struct HYD_arg_match_table {
    const char *arg;
     HYD_status(*handler_fn) (char *arg, char ***argv_p);
    void (*help_fn) (void);
};

HYD_status HYD_arg_parse_array(char ***argv, struct HYD_arg_match_table *match_table);
HYD_status HYD_arg_set_str(char *arg, char **var, const char *val);
HYD_status HYD_arg_set_int(char *arg, int *var, int val);

#endif /* HYDRA_ARG_H_INCLUDED */
