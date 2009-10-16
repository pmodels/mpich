/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMCI_H_INCLUDED
#define PMCI_H_INCLUDED

HYD_status HYD_pmci_launch_procs(void);
HYD_status HYD_pmci_wait_for_completion(void);
HYD_status HYD_pmci_finalize(void);

#endif /* PMCI_H_INCLUDED */
