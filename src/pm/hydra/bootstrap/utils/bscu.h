/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BSCU_H_INCLUDED
#define BSCU_H_INCLUDED

#include "hydra.h"
#include "hydra_sig.h"
#include "csi.h"
#include "bsci.h"

typedef struct HYD_BSCU_Procstate {
    int pid;
    int exit_status;
} HYD_BSCU_Procstate_t;

extern HYD_BSCU_Procstate_t *HYD_BSCU_Procstate;
extern int HYD_BSCU_Num_procs;
extern int HYD_BSCU_Completed_procs;

HYD_Status HYD_BSCU_Init_exit_status(void);
HYD_Status HYD_BSCU_Finalize_exit_status(void);
HYD_Status HYD_BSCU_Init_io_fds(void);
HYD_Status HYD_BSCU_Finalize_io_fds(void);
HYD_Status HYD_BSCU_Create_process(char **client_arg, int *in, int *out, int *err, int *pid);
HYD_Status HYD_BSCU_Wait_for_completion(void);
HYD_Status HYD_BSCU_Append_env(HYDU_Env_t * env_list, char **client_arg, int id);
HYD_Status HYD_BSCU_Append_exec(char **exec, char **client_arg);
HYD_Status HYD_BSCU_Append_wdir(char **client_arg);
HYD_Status HYD_BSCU_Set_common_signals(sighandler_t handler);
void HYD_BSCU_Signal_handler(int signal);

#endif /* BSCI_H_INCLUDED */
