/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_NEM_PKT_HEADER_H
#define MPID_NEM_PKT_HEADER_H

#define MPID_NEM_PKT_HEADER_FIELDS		\
    struct {                                    \
        int source;                             \
        int dest;                               \
        MPIR_Pint datalen;                      \
        unsigned short seqno;                   \
        unsigned short type; /* currently used only with checkpointing */ \
    };

typedef struct MPID_nem_pkt_header
{
    MPID_NEM_PKT_HEADER_FIELDS;
} MPID_nem_pkt_header_t;


#endif /* MPID_NEM_DATATYPES_H */
