/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PERSIST_SERVER_H_INCLUDED
#define PERSIST_SERVER_H_INCLUDED

#include "hydra.h"
#include "demux.h"
#include "persist.h"

struct HYDT_persist_handle_s {
    int port;                   /* port to listen on */
    int debug;                  /* Run in debug mode */
};

extern struct HYDT_persist_handle_s HYDT_persist_handle;

#endif /* PERSIST_SERVER_H_INCLUDED */
