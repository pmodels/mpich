/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIEXEC_H_INCLUDED
#define MPIEXEC_H_INCLUDED

#include "csi.h"

typedef enum {
    HYD_LCHI_PROPAGATE_NOTSET,
    HYD_LCHI_PROPAGATE_ALL,
    HYD_LCHI_PROPAGATE_NONE,
    HYD_LCHI_PROPAGATE_LIST
} HYD_LCHI_Propagate_t;

#define prop_to_str(prop) \
    (((prop) == HYD_LCHI_PROPAGATE_NOTSET) ? "HYD_LCHI_PROPAGATE_NOTSET" :        \
     ((prop) == HYD_LCHI_PROPAGATE_ALL) ? "HYD_LCHI_PROPAGATE_ALL" :	\
     ((prop) == HYD_LCHI_PROPAGATE_NONE) ? "HYD_LCHI_PROPAGATE_NONE" :            \
     ((prop) == HYD_LCHI_PROPAGATE_LIST) ? "HYD_LCHI_PROPAGATE_LIST": "UNKNOWN")

typedef struct {
    int debug_level;

    struct {
	HYD_CSI_Env_t        * added_env_list;
	HYD_LCHI_Propagate_t   prop;
	HYD_CSI_Env_t        * prop_env_list;
	char                 * wdir;
    } global;

    struct HYD_LCHI_Local {
	HYD_CSI_Exec_t        * exec;
	int                     num_procs;
	char                  * hostfile;
	HYD_CSI_Env_t         * added_env_list;
	HYD_LCHI_Propagate_t    prop;
	HYD_CSI_Env_t         * prop_env_list;
	struct HYD_LCHI_Local * next;
    } * local;
} HYD_LCHI_Params_t;

void HYD_LCHI_Dump_params(HYD_LCHI_Params_t params);
void HYD_LCHI_Env_copy(HYD_CSI_Env_t * dest, HYD_CSI_Env_t src);
HYD_Status HYD_LCHI_Global_env_list(HYD_CSI_Env_t **);
HYD_Status HYD_LCHI_Create_env_list(HYD_LCHI_Propagate_t prop, HYD_CSI_Env_t * added_env_list,
				    HYD_CSI_Env_t * prop_env_list, HYD_CSI_Env_t ** env_list);
HYD_Status HYD_LCHI_Get_parameters(int t_argc, char ** t_argv, HYD_LCHI_Params_t * handle);

HYD_Status HYD_LCHI_stdout_cb(int fd, HYD_CSI_Event_t events);
HYD_Status HYD_LCHI_stderr_cb(int fd, HYD_CSI_Event_t events);
HYD_Status HYD_LCHI_stdin_cb(int fd, HYD_CSI_Event_t events);

#endif /* MPIEXEC_H_INCLUDED */
