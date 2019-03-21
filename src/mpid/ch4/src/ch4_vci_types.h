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

/* VCI properties */
typedef enum {
    MPIDI_VCI_PROPERTY__TAGGED_ORDERED = 0x1,
    MPIDI_VCI_PROPERTY__TAGGED_UNORDERED = 0x2,
    MPIDI_VCI_PROPERTY__RAR = 0x4,
    MPIDI_VCI_PROPERTY__WAR = 0x8,
    MPIDI_VCI_PROPERTY__RAW = 0x10,
    MPIDI_VCI_PROPERTY__WAW = 0x20,
} MPIDI_vci_property_t;

#define MPIDI_VCI_PROPERTY__GENERIC MPIDI_VCI_PROPERTY__TAGGED_ORDERED | \
                                    MPIDI_VCI_PROPERTY__TAGGED_UNORDERED | \
                                    MPIDI_VCI_PROPERTY__RAR | \
                                    MPIDI_VCI_PROPERTY__WAR | \
                                    MPIDI_VCI_PROPERTY__RAW | \
                                    MPIDI_VCI_PROPERTY__WAW

#endif /* CH4_VCI_TYPES_H_INCLUDED */
