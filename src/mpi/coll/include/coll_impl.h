/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "allreduce_group.h"

#ifndef COLL_IMPL_H_INCLUDED
#define COLL_IMPL_H_INCLUDED

#include "tsp_impl.h"

#include "../algorithms/stubalgo/stubalgo.h"
#include "../algorithms/treealgo/treealgo.h"
#include "../algorithms/recexchalgo/recexchalgo.h"

#define MPII_COLLECTIVE_FALLBACK_CHECK(check, mpi_errno)                \
    do {                                                                \
        if ((check) == 0) {                                             \
            if (MPIR_CVAR_COLLECTIVE_FALLBACK == MPIR_CVAR_COLLECTIVE_FALLBACK_error) { \
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**collalgo"); \
            } else {                                                    \
                goto fallback;                                          \
            }                                                           \
        }                                                               \
    } while (0)

extern int MPIR_Nbc_progress_hook_id;

extern MPIR_Csel_node_s *MPIR_Csel_selection;
extern char MPII_coll_selection_json[];

/* Function to initialize communicators for collectives */
int MPIR_Coll_comm_init(MPIR_Comm * comm);

/* Function to cleanup any communicators for collectives */
int MPII_Coll_comm_cleanup(MPIR_Comm * comm);

/* Hook for any collective algorithms related initialization */
int MPII_Coll_init(void);

int MPIR_Coll_safe_to_block(void);

int MPII_Coll_finalize(void);

int MPIR_Coll_json(MPIR_Csel_coll_sig_s * coll_sig);
int MPIR_Coll_nb(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt);
int MPIR_Coll_sig_hold_handles(MPIR_Csel_coll_sig_s * coll_sig);
void MPIR_Coll_sig_prepare_gpu(MPIR_Csel_coll_sig_s * coll_sig);
int MPIR_Coll_buffer_swap(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt);
int MPIR_Coll_gpu_swap(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt,
                       MPIR_Coll_algo_fn fn);
int MPIR_Coll_persist_request_save(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt,
                                   MPIR_Coll_algo_fn fn);
int MPIR_Coll_persist_request_start(void *sched, MPIR_Request ** req_out);
void MPIR_Coll_persist_request_free(void *sched);

/* generated routines by gen_coll.py from coll_algorithms.txt */
void MPII_Coll_type_init(void);
void MPII_Coll_algo_init(void);
void MPII_Csel_init_condition_names(void);
void MPIR_Coll_cvar_init(void);
bool MPIR_Coll_check_algo_restriction(MPIR_Csel_coll_sig_s * coll_sig,
                                      MPII_Csel_container_type_e algo_id);
void MPIR_Coll_init_algo_container(MPIR_Csel_coll_sig_s * coll_sig,
                                   MPII_Csel_container_type_e algo_id,
                                   MPII_Csel_container_s * algo_cnt);

#define MPII_GENTRAN_CREATE_SCHED_P() \
    do { \
        *sched_type_p = MPIR_SCHED_GENTRAN; \
        MPIR_TSP_sched_create(sched_p, is_persistent); \
    } while (0)

#define MPII_SCHED_CREATE_SCHED_P() \
    do { \
        MPIR_Sched_t s = MPIR_SCHED_NULL; \
        enum MPIR_Sched_kind sched_kind = MPIR_SCHED_KIND_REGULAR; \
        if (is_persistent) { \
            sched_kind = MPIR_SCHED_KIND_PERSISTENT; \
        } \
        mpi_errno = MPIR_Sched_create(&s, sched_kind); \
        MPIR_ERR_CHECK(mpi_errno); \
        if (!tag) { \
            mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag); \
            MPIR_ERR_CHECK(mpi_errno); \
        } \
        MPIR_Sched_set_tag(s, tag); \
        *sched_type_p = MPIR_SCHED_NORMAL; \
        *sched_p = s; \
    } while (0)

#define MPII_CSEL_CREATE_TSP_SCHED(coll_sig) \
    do { \
        if (coll_sig->sched == NULL) { \
            coll_sig->sched_type = MPIR_SCHED_GENTRAN; \
            bool is_persistent = (coll_sig)->flags & MPIR_COLL_SIG_FLAG__PERSISTENT; \
            MPIR_TSP_sched_create(&coll_sig->sched, is_persistent); \
        } else { \
            MPIR_Assert(coll_sig->sched_type = MPIR_SCHED_GENTRAN); \
        } \
    } while (0)

#define MPII_CSEL_CREATE_SCHED(coll_sig) \
    do { \
        if (coll_sig->sched == NULL) { \
            MPIR_Sched_t s = MPIR_SCHED_NULL; \
            enum MPIR_Sched_kind sched_kind = MPIR_SCHED_KIND_REGULAR; \
            bool is_persistent = (coll_sig)->flags & MPIR_COLL_SIG_FLAG__PERSISTENT; \
            if (is_persistent) { \
                sched_kind = MPIR_SCHED_KIND_PERSISTENT; \
            } \
            mpi_errno = MPIR_Sched_create(&s, sched_kind); \
            MPIR_ERR_CHECK(mpi_errno); \
            if (!coll_sig->tag) { \
                mpi_errno = MPIR_Sched_next_tag(coll_sig->comm_ptr, &coll_sig->tag); \
                MPIR_ERR_CHECK(mpi_errno); \
            } \
            MPIR_Sched_set_tag(s, coll_sig->tag); \
            coll_sig->sched_type = MPIR_SCHED_NORMAL; \
            coll_sig->sched = s; \
        } else { \
            MPIR_Assert(coll_sig->sched_type = MPIR_SCHED_NORMAL); \
        } \
    } while (0)

#define MPII_SCHED_START(sched_type, sched, comm_ptr, request) \
    do { \
        if (sched_type == MPIR_SCHED_NORMAL) { \
            mpi_errno = MPIR_Sched_start(sched, comm_ptr, request); \
            MPIR_ERR_CHECK(mpi_errno); \
        } else if (sched_type == MPIR_SCHED_GENTRAN) { \
            mpi_errno = MPIR_TSP_sched_start(sched, comm_ptr, request); \
            MPIR_ERR_CHECK(mpi_errno); \
        } else if (sched_type == MPIR_SCHED_REQUEST) { \
            *request = sched; \
        } else { \
            MPIR_Assert(0); \
        } \
    } while (0)

#endif /* COLL_IMPL_H_INCLUDED */
