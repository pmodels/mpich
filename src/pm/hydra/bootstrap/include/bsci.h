/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BSCI_H_INCLUDED
#define BSCI_H_INCLUDED

#include "hydra.h"
#include "csi.h"

HYD_Status HYD_BSCI_Launch_procs();
HYD_Status HYD_BSCI_Cleanup_procs();
HYD_Status HYD_BSCI_Wait_for_completion();
HYD_Status HYD_BSCI_Finalize();

#endif /* BSCI_H_INCLUDED */
