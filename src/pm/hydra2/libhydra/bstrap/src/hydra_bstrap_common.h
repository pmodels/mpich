/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BSTRAP_COMMON_H_INCLUDED
#define BSTRAP_COMMON_H_INCLUDED

/* this file is internal to the bstrap, but is shared between the
 * server-side and proxy-side of the bstrap */

struct HYDI_bstrap_cmd {
    enum {
        HYDI_BSTRAP_CMD__PROXY_ID = 0,
        HYDI_BSTRAP_CMD__HOSTLIST,
    } type;

    union {
        struct {
            int id;
        } proxy_id;
    } u;
};

#endif /* BSTRAP_COMMON_H_INCLUDED */
