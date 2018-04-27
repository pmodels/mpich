/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef TSP_GENTRAN_TYPES_H_INCLUDED
#define TSP_GENTRAN_TYPES_H_INCLUDED

#include "utarray.h"

typedef enum {
    MPII_GENUTIL_VTX_KIND__ISEND,
    MPII_GENUTIL_VTX_KIND__IRECV,
    MPII_GENUTIL_VTX_KIND__IMCAST,
} MPII_Genutil_vtx_kind_e;

typedef enum {
    MPII_GENUTIL_VTX_STATE__INIT,
    MPII_GENUTIL_VTX_STATE__ISSUED,
    MPII_GENUTIL_VTX_STATE__COMPLETE,
} MPII_Genutil_vtx_state_e;

typedef struct MPII_Genutil_vtx_t {
    int vtx_kind;
    int vtx_state;
    int vtx_id;

    UT_array *in_vtcs;
    UT_array *out_vtcs;

    int pending_dependencies;

    union {
        struct {
            const void *buf;
            int count;
            MPI_Datatype dt;
            int dest;
            MPIR_Comm *comm;
            MPIR_Request *req;
        } isend;
        struct {
            void *buf;
            int count;
            MPI_Datatype dt;
            int src;
            MPIR_Comm *comm;
            MPIR_Request *req;
        } irecv;
        struct {
            const void *buf;
            int count;
            MPI_Datatype dt;
            UT_array *dests;
            int num_dests;
            MPIR_Comm *comm;
            MPIR_Request **req;
            int last_complete;
        } imcast;
    } u;

    struct MPII_Genutil_vtx_t *next;
} MPII_Genutil_vtx_t;

typedef struct {
    /* communication tag for this schedule */
    int tag;

    UT_array *vtcs;
    int total_vtcs;
    int completed_vtcs;

    /* issued vertices linked list */
    struct MPII_Genutil_vtx_t *issued_head;
    struct MPII_Genutil_vtx_t *issued_tail;
} MPII_Genutil_sched_t;

typedef MPII_Genutil_vtx_t vtx_t;

#endif /* TSP_GENTRAN_TYPES_H_INCLUDED */
