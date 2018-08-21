/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_RMK_H_INCLUDED
#define HYDRA_RMK_H_INCLUDED

/** @file hydra_rmk.h.in */

#include "hydra_base.h"
#include "hydra_node.h"

/*! \addtogroup rmk Resource Management Kernel
 * @{
 */

/**
 * \brief HYD_rmk_detect - Detect any available RMKs
 *
 * \param[out] ret                    RMK string
 *
 * This function is used to check if which RMK is available.
 */
const char *HYD_rmk_detect(void);

/**
 * \brief HYD_rmk_query_node_list - Query for node list information
 *
 * \param[out] node_list       Lists of nodes available
 *
 * This function allows the upper layers to query the available
 * nodes.
 */
HYD_status HYD_rmk_query_node_list(const char *rmk, int *node_count, struct HYD_node **nodes);

/*! @} */

#endif /* HYDRA_RMK_H_INCLUDED */
