/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BIND_PLPA_H_INCLUDED
#define BIND_PLPA_H_INCLUDED

HYD_status HYDT_bind_plpa_init(HYDT_bind_support_level_t * support_level);
HYD_status HYDT_bind_plpa_process(struct HYDT_bind_cpuset_t cpuset);

#endif /* BIND_PLPA_H_INCLUDED */
