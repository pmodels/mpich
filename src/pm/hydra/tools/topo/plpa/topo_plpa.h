/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef TOPO_PLPA_H_INCLUDED
#define TOPO_PLPA_H_INCLUDED

HYD_status HYDT_topo_plpa_init(HYDT_topo_support_level_t * support_level);
HYD_status HYDT_topo_plpa_bind(struct HYDT_topo_cpuset_t cpuset);
HYD_status HYDT_topo_plpa_finalize(void);

#endif /* TOPO_PLPA_H_INCLUDED */
