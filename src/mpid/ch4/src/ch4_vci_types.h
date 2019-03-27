/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * (C) 2019 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 *
 */

#ifndef CH4_VCI_TYPES_H_INCLUDED
#define CH4_VCI_TYPES_H_INCLUDED

#include "mpid_thread.h"

#define MPIDI_VCI_ROOT 0
#define MPIDI_VCI_INVALID -1

#define MPIDI_VCI_POOL(field) MPIDI_global.vci_pool.field
#define MPIDI_VCI(i) MPIDI_VCI_POOL(vci)[i]

/* VCI resources */
typedef enum {
    MPIDI_VCI_RESOURCE__TX = 0x1,       /* Can send */
    MPIDI_VCI_RESOURCE__RX = 0x2,       /* Can receive */
    MPIDI_VCI_RESOURCE__NM = 0x4,       /* Can perform netmod operations */
    MPIDI_VCI_RESOURCE__SHM = 0x8,      /* Can perform shmmod operations */
} MPIDI_vci_resource_t;

#define MPIDI_VCI_RESOURCE__GENERIC MPIDI_VCI_RESOURCE__TX | \
                                    MPIDI_VCI_RESOURCE__RX | \
                                    MPIDI_VCI_RESOURCE__NM | \
                                    MPIDI_VCI_RESOURCE__SHM

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

/* VCI types */
typedef enum {
    MPIDI_VCI_TYPE__SHARED = 0x1,       /* VCI can be used by multiple objects */
    MPIDI_VCI_TYPE__EXCLUSIVE = 0x2,    /* VCI will be used by only one object */
} MPIDI_vci_type_t;

/* VCI */
typedef struct MPIDI_vci {
    MPID_Thread_mutex_t lock;   /* lock to protect the objects in this VCI */
    int ref_count;              /* number of objects referring to this VCI */
    int is_free;                /* flag to check if this VCI is free or not */
    int vni;                    /* index to the VNI in the netmod's pool */
    int vsi;                    /* index to the VSI in the shmmod's pool */
} MPIDI_vci_t;

/* VCI pool */
typedef struct MPIDI_vci_pool {
    int next_free_vci;
    int max_vcis;
    MPID_Thread_mutex_t lock;   /* lock to protect the VCI pool */
    MPIDI_vci_t *vci;           /* array of VCIs */
} MPIDI_vci_pool_t;

/* VCI hash */
typedef struct MPIDI_vci_hash {
    union {
        struct {
            int vci;
        } single;
        /* TODO: struct multi */
    } u;
} MPIDI_vci_hash_t;

#endif /* CH4_VCI_TYPES_H_INCLUDED */
