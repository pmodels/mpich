/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PERSIST_H_INCLUDED
#define PERSIST_H_INCLUDED

#define PERSIST_DEFAULT_PORT 9899

typedef struct {
    enum {
        HYDT_PERSIST_STDOUT,
        HYDT_PERSIST_STDERR
    } io_type;
    int buflen;
} HYDT_persist_header;

#endif /* PERSIST_H_INCLUDED */
