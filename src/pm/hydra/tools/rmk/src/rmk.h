/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef RMK_H_INCLUDED
#define RMK_H_INCLUDED

/** @file rmk.h.in */

/*! \addtogroup rmk Resource Management Kernel
 * @{
 */

/**
 * \brief HYDT_rmk_detect - Detect any available RMKs
 *
 * \param[out] ret                    RMK string
 *
 * This function is used to check if which RMK is available.
 */
const char *HYDT_rmk_detect(void);

/**
 * \brief HYDT_rmk_query_node_list - Query for node list information
 *
 * \param[out] node_list       Lists of nodes available
 *
 * This function allows the upper layers to query the available
 * nodes.
 */
HYD_status HYDT_rmk_query_node_list(const char *rmk, struct HYD_node **node_list);

/*! @} */

#include "slurm_rmk.h"
#include "ll_rmk.h"
#include "lsf_rmk.h"
#include "sge_rmk.h"
#include "pbs_rmk.h"
#include "cobalt_rmk.h"

#endif /* RMK_H_INCLUDED */
