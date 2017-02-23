/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef CH4R_COMM_H_INCLUDED
#define CH4R_COMM_H_INCLUDED

#include "ch4_types.h"

#undef FUNCNAME
#define FUNCNAME MPIDIU_upids_to_lupids
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIU_upids_to_lupids(int size,
                                                    size_t *remote_upid_size,
                                                    char *remote_upids,
                                                    int **remote_lupids,
                                                    MPID_Node_id_t *remote_node_ids)
{
    int mpi_errno = MPI_SUCCESS, i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_UPIDS_TO_LUPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_UPIDS_TO_LUPIDS);

    mpi_errno = MPIDI_NM_upids_to_lupids(size, remote_upid_size, remote_upids, remote_lupids);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    /* update node_map */
    for (i = 0; i < size; i++) {
        int _avtid = 0, _lpid = 0;
        /* if this is a new process, update node_map and locality */
        if (MPIDIU_LUPID_IS_NEW_AVT((*remote_lupids)[i])) {
            MPIDIU_LUPID_CLEAR_NEW_AVT_MARK((*remote_lupids)[i]);
            _avtid = MPIDIU_LUPID_GET_AVTID((*remote_lupids)[i]);
            _lpid = MPIDIU_LUPID_GET_LPID((*remote_lupids)[i]);
            if (_avtid != 0) {
                /*
                 * new process groups are always assumed to be remote,
                 * so CH4 don't care what node they are on
                 */
                MPIDI_CH4_Global.node_map[_avtid][_lpid] = remote_node_ids[i];
                if (remote_node_ids[i] >= MPIDI_CH4_Global.max_node_id) {
                    MPIDI_CH4_Global.max_node_id = remote_node_ids[i] + 1;
                }
#ifdef MPIDI_BUILD_CH4_LOCALITY_INFO
                MPIDI_av_table[_avtid]->table[_lpid].is_local = 0;
#endif
            }
        }
    }

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_UPIDS_TO_LUPIDS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_Intercomm_map_bcast_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIU_Intercomm_map_bcast_intra(MPIR_Comm *local_comm,
                                                              int local_leader,
                                                              int *remote_size,
                                                              int *is_low_group,
                                                              int pure_intracomm,
                                                              size_t *remote_upid_size,
                                                              char *remote_upids,
                                                              int **remote_lupids,
                                                              MPID_Node_id_t *remote_node_ids)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int upid_recv_size = 0;
    int map_info[4];
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    size_t *_remote_upid_size = NULL;
    char *_remote_upids = NULL;
    MPID_Node_id_t *_remote_node_ids = NULL;

    MPIR_CHKPMEM_DECL(1);
    MPIR_CHKLMEM_DECL(3);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_INTERCOMM_MAP_BCAST_INTRA);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_INTERCOMM_MAP_BCAST_INTRA);

    if (local_comm->rank == local_leader) {
        if (!pure_intracomm) {
            for (i = 0; i < (*remote_size); i++) {
                upid_recv_size += remote_upid_size[i];
            }
        }
        map_info[0] = *remote_size;
        map_info[1] = upid_recv_size;
        map_info[2] = *is_low_group;
        map_info[3] = pure_intracomm;
        mpi_errno = MPIR_Bcast_intra(map_info, 4, MPI_INT, local_leader, local_comm, &errflag );
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        if (!pure_intracomm) {
            mpi_errno = MPIR_Bcast_intra(remote_upid_size, *remote_size, MPI_UNSIGNED_LONG,
                                         local_leader, local_comm, &errflag );
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            mpi_errno = MPIR_Bcast_intra(remote_upids, upid_recv_size, MPI_BYTE,
                                         local_leader, local_comm, &errflag );
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            mpi_errno =
                MPIR_Bcast_intra(remote_node_ids, (*remote_size) * sizeof(MPID_Node_id_t), MPI_BYTE,
                                 local_leader, local_comm, &errflag);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
        else {
            mpi_errno = MPIR_Bcast_intra(*remote_lupids, *remote_size, MPI_INT,
                                         local_leader, local_comm, &errflag);
        }
    }
    else {
        mpi_errno = MPIR_Bcast_intra(map_info, 4, MPI_INT, local_leader, local_comm, &errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        *remote_size = map_info[0];
        upid_recv_size = map_info[1];
        *is_low_group = map_info[2];
        pure_intracomm = map_info[3];

        MPIR_CHKPMEM_MALLOC((*remote_lupids), int*, (*remote_size) * sizeof(int),
                            mpi_errno, "remote_lupids");
        if (!pure_intracomm) {
            MPIR_CHKLMEM_MALLOC(_remote_upid_size, size_t*, (*remote_size) * sizeof(size_t),
                                mpi_errno, "_remote_upid_size");
            mpi_errno = MPIR_Bcast_intra(_remote_upid_size, *remote_size, MPI_UNSIGNED_LONG,
                                         local_leader, local_comm, &errflag);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            MPIR_CHKLMEM_MALLOC(_remote_upids, char*, upid_recv_size * sizeof(char),
                                mpi_errno, "_remote_upids");
            mpi_errno = MPIR_Bcast_intra(_remote_upids, upid_recv_size, MPI_BYTE,
                                         local_leader, local_comm, &errflag);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            MPIR_CHKLMEM_MALLOC(_remote_node_ids, MPID_Node_id_t*,
                                (*remote_size) * sizeof(MPID_Node_id_t),
                                mpi_errno, "_remote_node_ids");
            mpi_errno =
                MPIR_Bcast_intra(_remote_node_ids, (*remote_size) * sizeof(MPID_Node_id_t), MPI_BYTE,
                                 local_leader, local_comm, &errflag);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

            MPIDIU_upids_to_lupids(*remote_size, _remote_upid_size, _remote_upids,
                                   remote_lupids, _remote_node_ids);
        }
        else {
            mpi_errno = MPIR_Bcast_intra(*remote_lupids, *remote_size, MPI_INT,
                                         local_leader, local_comm, &errflag);
        }
    }

    MPIR_CHKPMEM_COMMIT();
  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_INTERCOMM_MAP_BCAST_INTRA);
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_alloc_lut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDIU_alloc_lut(MPIDI_rank_map_lut_t ** lut, int size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_rank_map_lut_t *new_lut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ALLOC_LUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ALLOC_LUT);

    new_lut = (MPIDI_rank_map_lut_t *) MPL_malloc(sizeof(MPIDI_rank_map_lut_t)
                                                  + size * sizeof(MPIDI_lpid_t));
    if (new_lut == NULL) {
        *lut = NULL;
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    MPIR_Object_set_ref(new_lut, 1);
    *lut = new_lut;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE,
                    (MPL_DBG_FDEST, "alloc lut %p, size %ld, refcount=%d",
                     new_lut, size * sizeof(MPIDI_lpid_t), MPIR_Object_get_ref(new_lut)));
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ALLOC_LUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_release_lut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDIU_release_lut(MPIDI_rank_map_lut_t * lut)
{
    int mpi_errno = MPI_SUCCESS;
    int count = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_RELEASE_LUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_RELEASE_LUT);

    MPIR_Object_release_ref(lut, &count);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE, (MPL_DBG_FDEST, "dec ref to lut %p", lut));
    if (count == 0) {
        MPL_free(lut);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE, (MPL_DBG_FDEST, "free lut %p", lut));
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_RELEASE_LUT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_alloc_mlut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDIU_alloc_mlut(MPIDI_rank_map_mlut_t ** mlut, int size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_rank_map_mlut_t *new_mlut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_ALLOC_MLUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_ALLOC_MLUT);

    new_mlut = (MPIDI_rank_map_mlut_t *) MPL_malloc(sizeof(MPIDI_rank_map_mlut_t)
                                                    + size * sizeof(MPIDI_gpid_t));
    if (new_mlut == NULL) {
        *mlut = NULL;
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    MPIR_Object_set_ref(new_mlut, 1);
    *mlut = new_mlut;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE,
                    (MPL_DBG_FDEST, "alloc mlut %p, size %ld, refcount=%d",
                     new_mlut, size * sizeof(MPIDI_gpid_t), MPIR_Object_get_ref(new_mlut)));
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_ALLOC_MLUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_release_mlut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDIU_release_mlut(MPIDI_rank_map_mlut_t * mlut)
{
    int mpi_errno = MPI_SUCCESS;
    int count = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_RELEASE_MLUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_RELEASE_MLUT);

    MPIR_Object_release_ref(mlut, &count);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE, (MPL_DBG_FDEST, "dec ref to mlut %p", mlut));
    if (count == 0) {
        MPL_free(mlut);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE, (MPL_DBG_FDEST, "free mlut %p", mlut));
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_RELEASE_MLUT);
    return mpi_errno;
}

#endif /* ifndef CH4R_COMM_H_INCLUDED */
