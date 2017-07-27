/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_TOPO_H_INCLUDED
#define HYDRA_TOPO_H_INCLUDED

/** @file hydra_topo.h */

#include "hydra_base.h"

/*! \addtogroup topo Process Topology Interface
 * @{
 */

/**
 * \brief HYD_topo_init - Initialize the topology library
 *
 * \param[in]  binding   Binding pattern to use
 * \param[in]  mapping   Mapping pattern to use
 *
 * This function initializes the topology library requested by the
 * user. It also queries for the support provided by the library and
 * stores it for future calls.
 */
HYD_status HYD_topo_init(char *binding, char *mapping, char *membind);


/**
 * \brief HYD_topo_finalize - Finalize the topology library
 *
 * This function cleans up any relevant state that the topology library
 * maintained.
 */
HYD_status HYD_topo_finalize(void);


/**
 * \brief HYD_topo_bind - Bind process to a processing element
 *
 * \param[in] idx   Index of the cpuset to the bind to
 *
 * This function binds a process to an appropriate PU index set. If
 * the cpuset does not contain any set PU index, no binding is done.
 */
HYD_status HYD_topo_bind(int idx);

/*!
 * @}
 */

#endif /* HYDRA_TOPO_H_INCLUDED */
