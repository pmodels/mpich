/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* This function assumes that the structure of the key is as follows:
 * typedef struct {
 *     int coll_op;
 *     union {
 *         struct {...} bcst;
 *         struct {...} reduce;
 *         .
 *         .
 *         .
 *     } args;
 * } MPIR_ALGO_args_t;
 * This function returns the number of bytes actually used in the key.
 * Size of the key can be larger than that required by coll_op
 * because key containts a union of args for different collective types.
 * The coll_op values are as per the enum in coll/include/coll_sched_db.h file
 * */

MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_get_key_size (MPIR_ALGO_args_t* key) {
    int key_size=0;
    key_size += sizeof(key->coll_op);

    switch (key->coll_op) {
        case BCAST:
            key_size += sizeof(key->args.bcast);
            break;
        case REDUCE:
            key_size += sizeof(key->args.reduce);
            break;
        case ALLREDUCE:
            key_size += sizeof(key->args.allreduce);
            break;
        case SCATTER:
            key_size += sizeof(key->args.scatter);
            break;
        case GATHER:
            key_size += sizeof(key->args.gather);
            break;
        case ALLGATHER:
            key_size += sizeof(key->args.allgather);
            break;
        case ALLTOALL:
            key_size += sizeof(key->args.alltoall);
            break;
        case ALLTOALLV:
            key_size += sizeof(key->args.alltoallv);
            break;
        case ALLTOALLW:
            key_size += sizeof(key->args.alltoallw);
            break;
        case BARRIER:
            key_size += sizeof(key->args.barrier);
            break;
        case REDUCESCATTER:
            key_size += sizeof(key->args.reducescatter);
            break;
        case SCAN:
            key_size += sizeof(key->args.scan);
            break;
        case EXSCAN:
            key_size += sizeof(key->args.exscan);
            break;
        case GATHERV:
            key_size += sizeof(key->args.gatherv);
            break;
        case ALLGATHERV:
            key_size += sizeof(key->args.allgatherv);
            break;
        case SCATTERV:
            key_size += sizeof(key->args.scatterv);
            break;
        default:
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"coll_type not recognized\n"));
            MPIR_Assert(0);
            break;
    }

    return key_size;
}
