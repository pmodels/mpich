/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef TOPO_H_INCLUDED
#define TOPO_H_INCLUDED

/** @file topo.h */

#include "hydra.h"

/*! \addtogroup topo Process Topology Interface
 * @{
 */

/**
 * \brief Topology information
 *
 * Contains private persistent information stored by the topology
 * library.
 */
struct HYDT_topo_info {
    /** \brief Topology library to use */
    char *topolib;
    /** \brief Enable debugging output */
    int debug;
};

/*! \cond */
extern struct HYDT_topo_info HYDT_topo_info;
/*! \endcond */

/**
 * \brief HYDT_topo_init - Initialize the topology library
 *
 * \param[in]  topolib   Topology library to use
 * \param[in]  binding   Binding pattern to use
 * \param[in]  mapping   Mapping pattern to use
 *
 * This function initializes the topology library requested by the
 * user. It also queries for the support provided by the library and
 * stores it for future calls.
 */
HYD_status HYDT_topo_init(char *topolib, char *binding, char *mapping, char *membind);


/**
 * \brief HYDT_topo_finalize - Finalize the topology library
 *
 * This function cleans up any relevant state that the topology library
 * maintained.
 */
HYD_status HYDT_topo_finalize(void);


/**
 * \brief HYDT_topo_bind - Bind process to a processing element
 *
 * \param[in] idx   Index of the cpuset to the bind to
 *
 * This function binds a process to an appropriate PU index set. If
 * the cpuset does not contain any set PU index, no binding is done.
 */
HYD_status HYDT_topo_bind(int idx);

/*!
 * @}
 */

#endif /* TOPO_H_INCLUDED */
