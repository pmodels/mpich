/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BSCU_H_INCLUDED
#define BSCU_H_INCLUDED

#include "hydra_base.h"

HYD_Status HYD_BSCU_finalize(void);
HYD_Status HYD_BSCU_get_usize(int *size);
HYD_Status HYD_BSCU_wait_for_completion(void);

#endif /* BSCU_H_INCLUDED */
