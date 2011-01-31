/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PERSIST_SERVER_H_INCLUDED
#define PERSIST_SERVER_H_INCLUDED

#include "hydra.h"
#include "demux.h"
#include "persist.h"

struct HYDT_persist_handle {
    int port;                   /* port to listen on */
    int debug;                  /* Run in debug mode */
};

extern struct HYDT_persist_handle HYDT_persist_handle;

#endif /* PERSIST_SERVER_H_INCLUDED */
