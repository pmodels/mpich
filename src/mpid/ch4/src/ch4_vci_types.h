/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * (C) 2019 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 *
 */

#ifndef CH4_VCI_TYPES_H_INCLUDED
#define CH4_VCI_TYPES_H_INCLUDED

/* VCI resources */
typedef enum {
    MPIDI_VCI_RESOURCE__TX = 0x1,       /* Can send */
    MPIDI_VCI_RESOURCE__RX = 0x2,       /* Can receive */
} MPIDI_vci_resource_t;

#define MPIDI_VCI_RESOURCE__GENERIC MPIDI_VCI_RESOURCE__TX | \
                                    MPIDI_VCI_RESOURCE__RX

#endif /* CH4_VCI_TYPES_H_INCLUDED */
