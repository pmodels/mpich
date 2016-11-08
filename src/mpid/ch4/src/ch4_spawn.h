/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4_SPAWN_H_INCLUDED
#define CH4_SPAWN_H_INCLUDED

#include "ch4_impl.h"

static inline int MPIDI_mpi_to_pmi_keyvals(MPIR_Info * info_ptr,
                                           PMI_keyval_t ** kv_ptr, int *nkeys_ptr)
{
    char key[MPI_MAX_INFO_KEY];
    PMI_keyval_t *kv = 0;
    int i, nkeys = 0, vallen, flag, mpi_errno = MPI_SUCCESS;

    if (!info_ptr || info_ptr->handle == MPI_INFO_NULL)
        goto fn_exit;

    MPIR_Info_get_nkeys_impl(info_ptr, &nkeys);

    if (nkeys == 0)
        goto fn_exit;

    kv = (PMI_keyval_t *) MPL_malloc(nkeys * sizeof(PMI_keyval_t));

    for (i = 0; i < nkeys; i++) {
        mpi_errno = MPIR_Info_get_nthkey_impl(info_ptr, i, key);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_Info_get_valuelen_impl(info_ptr, key, &vallen, &flag);
        kv[i].key = (const char *) MPL_strdup(key);
        kv[i].val = (char *) MPL_malloc(vallen + 1);
        MPIR_Info_get_impl(info_ptr, key, vallen + 1, kv[i].val, &flag);
    }

  fn_exit:
    *kv_ptr = kv;
    *nkeys_ptr = nkeys;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static inline void MPIDI_free_pmi_keyvals(PMI_keyval_t ** kv, int size, int *counts)
{
    int i, j;

    for (i = 0; i < size; i++) {
        for (j = 0; j < counts[i]; j++) {
            if (kv[i][j].key != NULL)
                MPL_free((char *) kv[i][j].key);

            if (kv[i][j].val != NULL)
                MPL_free(kv[i][j].val);
        }

        if (kv[i] != NULL)
            MPL_free(kv[i]);
    }

}

#undef FUNCNAME
#define FUNCNAME MPIDI_Comm_spawn_multiple
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Comm_spawn_multiple(int count,
                                                       char *commands[],
                                                       char **argvs[],
                                                       const int maxprocs[],
                                                       MPIR_Info * info_ptrs[],
                                                       int root,
                                                       MPIR_Comm * comm_ptr,
                                                       MPIR_Comm ** intercomm, int errcodes[])
{
    char port_name[MPI_MAX_PORT_NAME];
    int *info_keyval_sizes = 0, i, mpi_errno = MPI_SUCCESS;
    PMI_keyval_t **info_keyval_vectors = 0, preput_keyval_vector;
    int *pmi_errcodes = 0, pmi_errno = 0;
    int total_num_processes, should_accept = 1;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_COMM_SPAWN_MULTIPLE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_COMM_SPAWN_MULTIPLE);

    memset(port_name, 0, sizeof(port_name));

    if (comm_ptr->rank == root) {
        total_num_processes = 0;

        for (i = 0; i < count; i++)
            total_num_processes += maxprocs[i];

        pmi_errcodes = (int *) MPL_malloc(sizeof(int) * total_num_processes);
        MPIR_ERR_CHKANDJUMP(!pmi_errcodes, mpi_errno, MPI_ERR_OTHER, "**nomem");

        for (i = 0; i < total_num_processes; i++)
            pmi_errcodes[i] = 0;

        mpi_errno = MPIDI_Open_port(NULL, port_name);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        info_keyval_sizes = (int *) MPL_malloc(count * sizeof(int));
        MPIR_ERR_CHKANDJUMP(!info_keyval_sizes, mpi_errno, MPI_ERR_OTHER, "**nomem");
        info_keyval_vectors = (PMI_keyval_t **) MPL_malloc(count * sizeof(PMI_keyval_t *));
        MPIR_ERR_CHKANDJUMP(!info_keyval_vectors, mpi_errno, MPI_ERR_OTHER, "**nomem");

        if (!info_ptrs)
            for (i = 0; i < count; i++) {
                info_keyval_vectors[i] = 0;
                info_keyval_sizes[i] = 0;
            }
        else
            for (i = 0; i < count; i++) {
                mpi_errno = MPIDI_mpi_to_pmi_keyvals(info_ptrs[i],
                                                     &info_keyval_vectors[i],
                                                     &info_keyval_sizes[i]);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            }

        preput_keyval_vector.key = MPIDI_PARENT_PORT_KVSKEY;
        preput_keyval_vector.val = port_name;
        pmi_errno = PMI_Spawn_multiple(count, (const char **)
                                       commands,
                                       (const char ***) argvs,
                                       maxprocs, info_keyval_sizes, (const PMI_keyval_t **)
                                       info_keyval_vectors, 1, &preput_keyval_vector, pmi_errcodes);

        if (pmi_errno != PMI_SUCCESS)
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER,
                                 "**pmi_spawn_multiple", "**pmi_spawn_multiple %d", pmi_errno);

        if (errcodes != MPI_ERRCODES_IGNORE) {
            for (i = 0; i < total_num_processes; i++) {
                errcodes[i] = pmi_errcodes[0];
                should_accept = should_accept && errcodes[i];
            }

            should_accept = !should_accept;
        }
    }

    if (errcodes != MPI_ERRCODES_IGNORE) {
        MPIR_Errflag_t errflag = MPIR_ERR_NONE;
        mpi_errno = MPIR_Bcast_impl(&should_accept, 1, MPI_INT, root, comm_ptr, &errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Bcast_impl(&pmi_errno, 1, MPI_INT, root, comm_ptr, &errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Bcast_impl(&total_num_processes, 1, MPI_INT, root, comm_ptr, &errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Bcast_impl(errcodes, total_num_processes, MPI_INT,
                                    root, comm_ptr, &errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    if (should_accept) {
        mpi_errno = MPIDI_Comm_accept(port_name, NULL, root, comm_ptr, intercomm);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    else {
        if ((pmi_errno == PMI_SUCCESS) && (errcodes[0] != 0))
            MPIR_Comm_create(intercomm);
    }

    if (comm_ptr->rank == root) {
        mpi_errno = MPIDI_Close_port(port_name);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:

    if (info_keyval_vectors) {
        MPIDI_free_pmi_keyvals(info_keyval_vectors, count, info_keyval_sizes);
        MPL_free(info_keyval_vectors);
    }

    if (info_keyval_sizes)
        MPL_free(info_keyval_sizes);

    if (pmi_errcodes)
        MPL_free(pmi_errcodes);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_COMM_SPAWN_MULTIPLE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Comm_connect
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Comm_connect(const char *port_name,
                                                MPIR_Info * info,
                                                int root, MPIR_Comm * comm,
                                                MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_COMM_CONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_COMM_CONNECT);
    mpi_errno = MPIDI_NM_mpi_comm_connect(port_name, info, root, comm, newcomm_ptr);

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_COMM_CONNECT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Comm_disconnect
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Comm_disconnect(MPIR_Comm * comm_ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_COMM_DISCONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_COMM_DISCONNECT);
    mpi_errno = MPIDI_NM_mpi_comm_disconnect(comm_ptr);

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_COMM_DISCONNECT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Open_port
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Open_port(MPIR_Info * info_ptr, char *port_name)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_OPEN_PORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_OPEN_PORT);
    mpi_errno = MPIDI_NM_mpi_open_port(info_ptr, port_name);

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_OPEN_PORT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Close_port
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Close_port(const char *port_name)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_CLOSE_PORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_CLOSE_PORT);
    mpi_errno = MPIDI_NM_mpi_close_port(port_name);

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_CLOSE_PORT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Comm_accept
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Comm_accept(const char *port_name,
                                               MPIR_Info * info,
                                               int root, MPIR_Comm * comm, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_COMM_ACCEPT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_COMM_ACCEPT);
    mpi_errno = MPIDI_NM_mpi_comm_accept(port_name, info, root, comm, newcomm_ptr);

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_COMM_ACCEPT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_SPAWN_H_INCLUDED */
