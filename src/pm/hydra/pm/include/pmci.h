/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMCI_H_INCLUDED
#define PMCI_H_INCLUDED

/** @file pmci.h */

/*! \addtogroup pmci Process Management Control Interface
 * @{
 */

/**
 * \brief HYD_pmci_launch_procs - Launch processes
 *
 * This function appends the appropriate process management interface
 * specific environment and other functionality
 */
HYD_status HYD_pmci_launch_procs(void);

/**
 * \brief HYD_pmci_wait_for_completion - Wait for launched processes to complete
 *
 * Wait for launched processes to complete
 */
HYD_status HYD_pmci_wait_for_completion(int timeout);

/**
 * \brief HYD_pmci_finalize - Finalize process management control device
 *
 * This function cleans up any relevant state that the process
 * management control device maintained.
 */
HYD_status HYD_pmci_finalize(void);

/*!
 * @}
 */

#endif /* PMCI_H_INCLUDED */
