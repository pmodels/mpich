/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BSCU_H_INCLUDED
#define BSCU_H_INCLUDED

#include "hydra.h"
#include "hydra_sig.h"
#include "bsci.h"

HYD_Status HYD_BSCU_Init_exit_status(void);
HYD_Status HYD_BSCU_Finalize_exit_status(void);
HYD_Status HYD_BSCU_Init_io_fds(void);
HYD_Status HYD_BSCU_Finalize_io_fds(void);
HYD_Status HYD_BSCU_Wait_for_completion(void);
HYD_Status HYD_BSCU_Set_common_signals(void (*handler) (int));
void HYD_BSCU_Signal_handler(int signal);

#endif /* BSCI_H_INCLUDED */
