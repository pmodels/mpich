/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef UCX_INIT_H_INCLUDED
#define UCX_INIT_H_INCLUDED

#include "ucx_impl.h"
#include "mpir_cvars.h"
#include "ucx_types.h"
#include "pmi.h"
#include <ucp/api/ucp.h>
#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_init_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_init_hook(int rank,
                                         int size,
                                         int appnum,
                                         int *tag_ub,
                                         MPIR_Comm * comm_world,
                                         MPIR_Comm * comm_self,
                                         int spawned)
{
    int mpi_errno = MPI_SUCCESS, pmi_errno;
    int str_errno = MPL_STR_SUCCESS;
    ucp_config_t *config;
    ucs_status_t ucx_status;
    uint64_t features = 0;
    int status;
    int val_max_sz, key_max_sz;
    char *valS, *val;
    char *keyS;
    char *remote_addr;
    size_t maxlen;
    int string_addr_len;
    int max_string;
    int i;
    ucp_params_t ucp_params;
    ucp_worker_params_t worker_params;
    ucp_ep_params_t ep_params;

    int avtid = 0, max_n_avts;

    int p;
    int addr_size = 0;
    char *string_addr;
    MPIR_CHKLMEM_DECL(4);


    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_INIT);

    ucx_status = ucp_config_read(NULL, NULL, &config);
    MPIDI_UCX_CHK_STATUS(ucx_status);

    /* For now use only the tag feature */
    features = UCP_FEATURE_TAG | UCP_FEATURE_RMA;
    ucp_params.features = features;
    ucp_params.request_size = sizeof(MPIDI_UCX_ucp_request_t);
    ucp_params.request_init = MPIDI_UCX_Request_init_callback;
    ucp_params.request_cleanup = NULL;
    ucp_params.estimated_num_eps = size;

    ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES|
		            UCP_PARAM_FIELD_REQUEST_SIZE|
			    UCP_PARAM_FIELD_ESTIMATED_NUM_EPS|
			    UCP_PARAM_FIELD_REQUEST_INIT; 

    ucx_status = ucp_init(&ucp_params, config, &MPIDI_UCX_global.context);
    MPIDI_UCX_CHK_STATUS(ucx_status);
    ucp_config_release(config);

    worker_params.field_mask  = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode =  UCS_THREAD_MODE_SERIALIZED;

    ucx_status = ucp_worker_create(MPIDI_UCX_global.context, &worker_params,
                                   &MPIDI_UCX_global.worker);
    MPIDI_UCX_CHK_STATUS(ucx_status);
    ucx_status =
        ucp_worker_get_address(MPIDI_UCX_global.worker, &MPIDI_UCX_global.if_address,
                               &MPIDI_UCX_global.addrname_len);
    MPIDI_UCX_CHK_STATUS(ucx_status);
#ifdef USE_PMI2_API
    val_max_sz = PMI2_MAX_VALLEN;
    key_max_sz = PMI2_MAX_KEYLEN;
#else
    pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    MPIDI_UCX_PMI_ERROR(pmi_errno);

    pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    MPIDI_UCX_PMI_ERROR(pmi_errno);

#endif

    /*we have to reduce the value - the total size of an PMI string is 1024, so command+value+key
     * assume 100 characters for the command to be save */
    val_max_sz = val_max_sz - key_max_sz - 100;


    MPIR_CHKLMEM_MALLOC(valS, char *, val_max_sz, mpi_errno, "valS");
/* In UCX we have the problem that the key size (as a string) van be larger than val_max_sz.
 * We create a string from the key - but we don't know the size that this string will have
 * So we guess the size - based on the worker address size. The decoding uses the hex-representation
 * of the binary. So we need 2 bytes per byte. Add some extra bytes for the "key".
 */

    max_string = MPIDI_UCX_global.addrname_len * 2 + 128;
    MPIR_CHKLMEM_MALLOC(keyS, char *, key_max_sz, mpi_errno, "keyS");
    MPIR_CHKLMEM_MALLOC(string_addr, char *, max_string, mpi_errno, "string_addr");
    MPIR_CHKLMEM_MALLOC(remote_addr, char *, max_string, mpi_errno, "remote_addr");

    maxlen = max_string;
    val = string_addr;

    str_errno =
        MPL_str_add_binary_arg(&val, (int *) &maxlen, "U", (char *) MPIDI_UCX_global.if_address,
                               (int) MPIDI_UCX_global.addrname_len);

    /*todo: fallback if buffer is to small */
    MPIDI_UCX_STR_ERRCHK(str_errno);

    string_addr_len = max_string - maxlen;
    pmi_errno = PMI_KVS_Get_my_name(MPIDI_UCX_global.kvsname, val_max_sz);
    val = valS;
    /* I first commit my worker-address size */
    maxlen = val_max_sz;
    sprintf(keyS, "Ksize-%d", rank);
    MPL_str_add_int_arg(&val, (int *) &maxlen, "K", string_addr_len);
    val = valS;
    pmi_errno = PMI_KVS_Put(MPIDI_UCX_global.kvsname, keyS, val);
    MPIDI_UCX_PMI_ERROR(pmi_errno);
    pmi_errno = PMI_KVS_Commit(MPIDI_UCX_global.kvsname);
    MPIDI_UCX_PMI_ERROR(pmi_errno);
/* now we have to commit the key. However, if the size is larger than the val_max_sz,
 * we have tho spilt it up That's ugly, but badluck */

    if (string_addr_len < val_max_sz) {
        val = string_addr;
        sprintf(keyS, "UCX-%d", rank);
        pmi_errno = PMI_KVS_Put(MPIDI_UCX_global.kvsname, keyS, val);
        MPIDI_UCX_PMI_ERROR(pmi_errno);
        pmi_errno = PMI_KVS_Commit(MPIDI_UCX_global.kvsname);
    }
    else {
        p = 0;
        while (p < string_addr_len) {
            val = valS;
            MPL_snprintf(val, val_max_sz, "%s", string_addr + p);
            val = valS;
            sprintf(keyS, "UCX-%d-%d", rank, p);
            pmi_errno = PMI_KVS_Put(MPIDI_UCX_global.kvsname, keyS, val);
            MPIDI_UCX_PMI_ERROR(pmi_errno);
            pmi_errno = PMI_KVS_Commit(MPIDI_UCX_global.kvsname);
            MPIDI_UCX_PMI_ERROR(pmi_errno);
            p += val_max_sz - 1;
        }
    }

    val = valS;
    MPIDI_UCX_global.max_addr_len = MPIDI_UCX_global.addrname_len;

    pmi_errno = PMI_Barrier();
    MPIDI_UCX_PMI_ERROR(pmi_errno);

    /* Set to NULL now, only created if required in MPI_Intercomm_create */

    MPIDI_UCX_global.pmi_addr_table = NULL;
    maxlen = val_max_sz - 1;

    for (i = 0; i < size; i++) {
        /*first get the size */
        sprintf(keyS, "Ksize-%d", i);
        pmi_errno = PMI_KVS_Get(MPIDI_UCX_global.kvsname, keyS, val, val_max_sz);
        str_errno = MPL_str_get_int_arg(val, "K", &string_addr_len);

        if (string_addr_len < val_max_sz) {
            val = string_addr;
            sprintf(keyS, "UCX-%d", i);
            pmi_errno = PMI_KVS_Get(MPIDI_UCX_global.kvsname, keyS, val, val_max_sz);
            MPIDI_UCX_PMI_ERROR(pmi_errno);
            str_errno = MPL_str_get_binary_arg(string_addr, "U", remote_addr,
                                               (int) max_string, (int *) &addr_size);

            MPIDI_UCX_STR_ERRCHK(str_errno);
        }
        else {
            /* first catch the string together */
            p = 0;
            while (p < string_addr_len) {
                val = string_addr + p;
                sprintf(keyS, "UCX-%d-%d", i, p);
                pmi_errno = PMI_KVS_Get(MPIDI_UCX_global.kvsname, keyS, val, val_max_sz);
                p += val_max_sz - 1;
            }
            str_errno = MPL_str_get_binary_arg(string_addr, "U", remote_addr,
                                               (int) max_string, (int *) &addr_size);
            MPIDI_UCX_STR_ERRCHK(str_errno);

        }

        if (addr_size > MPIDI_UCX_global.max_addr_len)
            MPIDI_UCX_global.max_addr_len = addr_size;

        ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
        ep_params.address    = (ucp_address_t*) remote_addr;
        ucx_status = ucp_ep_create(MPIDI_UCX_global.worker,
                                   &ep_params,
                                   &MPIDI_UCX_AV(&MPIDIU_get_av(0, i)).dest);

        MPIDI_UCX_CHK_STATUS(ucx_status);
    }

    MPIDIG_init(comm_world, comm_self);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_EXIT);
    return mpi_errno;
  fn_fail:
    if (MPIDI_UCX_global.worker != NULL)
        ucp_worker_destroy(MPIDI_UCX_global.worker);

    if (MPIDI_UCX_global.context != NULL)
        ucp_cleanup(MPIDI_UCX_global.context);

    goto fn_exit;

}

static inline int MPIDI_NM_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS, pmi_errno;
    int i, p=0, completed;
    MPIR_Comm *comm;
    ucs_status_ptr_t  ucp_request;
    ucs_status_ptr_t* pending;

    comm = MPIR_Process.comm_world;
    pending = MPL_malloc(sizeof(ucs_status_ptr_t) * comm->local_size);

    for (i = 0;  i< comm->local_size; i++) {
        ucp_request = ucp_disconnect_nb(MPIDI_UCX_AV(&MPIDIU_get_av(0, i)).dest);
        MPIDI_CH4_UCX_REQUEST(ucp_request);
        if(ucp_request != UCS_OK) {
            pending[p] = ucp_request;
            p++;
        }
    }

    /* now complete the outstaning requests! Important: call progress inbetween, otherwise we
     * deadlock! */
    completed = p;
    while (completed != 0) {
        ucp_worker_progress(MPIDI_UCX_global.worker);
        completed = p;
        for(i = 0; i < p ; i++) {
            if(ucp_request_is_completed(pending[i])!=0)
                completed -= 1;
        }
    }

    for(i = 0; i < p ; i++) {
        ucp_request_release(pending[i]);
    }

    pmi_errno = PMI_Barrier();
    MPIDI_UCX_PMI_ERROR(pmi_errno);


    if (MPIDI_UCX_global.worker != NULL)
        ucp_worker_destroy(MPIDI_UCX_global.worker);

    if (MPIDI_UCX_global.context != NULL)
        ucp_cleanup(MPIDI_UCX_global.context);

    MPIR_Comm_release_always(comm);

    comm = MPIR_Process.comm_self;
    MPIR_Comm_release_always(comm);
    if (MPIDI_UCX_global.pmi_addr_table)
        MPL_free(MPIDI_UCX_global.pmi_addr_table);

    MPIDIG_finalize();
    PMI_Finalize();

  fn_exit:
    MPL_free(pending);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

static inline int MPIDI_NM_comm_get_lpid(MPIR_Comm * comm_ptr,
                                         int idx, int *lpid_ptr, MPL_bool is_remote)
{
    int avtid = 0, lpid = 0;
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    }
    else if (is_remote) {
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    }
    else {
        MPIDIU_comm_rank_to_pid_local(comm_ptr, idx, &lpid, &avtid);
    }

    *lpid_ptr = MPIDIU_LUPID_CREATE(avtid, lpid);
    return MPI_SUCCESS;

}

static inline int MPIDI_NMI_allocate_address_table()
{

    char *keyS;
    char *valS, *val;
    int mpi_errno = MPI_SUCCESS, pmi_errno, str_errno;
    int len = MPIDI_UCX_global.max_addr_len;
    int i;
    int size, maxlen = 1;
    int key_max_sz, val_max_sz;
    int string_addr_len;
    int max_string;
    char *string_addr;
    int addr_size;
    int p;
    MPIR_CHKLMEM_DECL(3);
#ifdef USE_PMI2_API
    val_max_sz = PMI2_MAX_VALLEN;
    key_max_sz = PMI2_MAX_KEYLEN;
#else
    pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    MPIDI_UCX_PMI_ERROR(pmi_errno);
    pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    MPIDI_UCX_PMI_ERROR(pmi_errno);
#endif

    val_max_sz = val_max_sz - 100 - key_max_sz; /*see comment at init */

    max_string = len * 2 + 128;
    MPIR_CHKLMEM_MALLOC(valS, char *, val_max_sz, mpi_errno, "valS");
    MPIR_CHKLMEM_MALLOC(keyS, char *, key_max_sz, mpi_errno, "keyS");
    MPIR_CHKLMEM_MALLOC(string_addr, char *, max_string, mpi_errno, "string_addr");
    val = valS;

    size = MPIR_Process.comm_world->local_size;
    MPIDI_UCX_global.pmi_addr_table = MPL_malloc(size * len);
    memset(MPIDI_UCX_global.pmi_addr_table, 0x0, len * size);

    for (i = 0; i < size; i++) {
        /*first get the size */
        sprintf(keyS, "Ksize-%d", i);
        pmi_errno = PMI_KVS_Get(MPIDI_UCX_global.kvsname, keyS, val, val_max_sz);
        MPIDI_UCX_PMI_ERROR(pmi_errno);
        str_errno = MPL_str_get_int_arg(val, "K", &string_addr_len);
        if (string_addr_len < val_max_sz) {
            val = string_addr;
            sprintf(keyS, "UCX-%d", i);
            pmi_errno = PMI_KVS_Get(MPIDI_UCX_global.kvsname, keyS, val, val_max_sz);
            MPIDI_UCX_PMI_ERROR(pmi_errno);
            str_errno =
                MPL_str_get_binary_arg(string_addr, "U", &MPIDI_UCX_global.pmi_addr_table[i * len],
                                       (int) max_string, (int *) &addr_size);
            MPIDI_UCX_STR_ERRCHK(str_errno);
        }
        else {
            /* first catch the string together */
            p = 0;
            while (p < string_addr_len) {
                val = string_addr + p;
                sprintf(keyS, "UCX-%d-%d", i, p);
                pmi_errno = PMI_KVS_Get(MPIDI_UCX_global.kvsname, keyS, val, val_max_sz);
                MPIDI_UCX_PMI_ERROR(pmi_errno);
                p += val_max_sz - 1;
            }
            str_errno =
                MPL_str_get_binary_arg(string_addr, "U", &MPIDI_UCX_global.pmi_addr_table[i * len],
                                       (int) max_string, (int *) &addr_size);
        }

    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size,
                                           char **local_upids)
{
    int mpi_errno = MPI_SUCCESS;
    int i, total_size = 0;
    char *temp_buf = NULL, *curr_ptr = NULL;
    char keyS[MPIDI_UCX_KVSAPPSTRLEN];
    char valS[MPIDI_UCX_KVSAPPSTRLEN];
    int len = MPIDI_UCX_global.max_addr_len;

    *local_upid_size = (size_t *) MPL_malloc(comm->local_size * sizeof(size_t));
    temp_buf = (char *) MPL_malloc(comm->local_size * len);

    for (i = 0; i < comm->local_size; i++) {
        sprintf(keyS, "UCX-%d", i);
        PMI_KVS_Get(MPIDI_UCX_global.kvsname, keyS, valS, MPIDI_UCX_KVSAPPSTRLEN);
        // MPIDI_UCX_PMI_ERROR(pmi_errno, pmi_commit);
        MPL_str_get_binary_arg(valS, "UCX", &temp_buf[len * i],
                               (int) len, (int *) &(*local_upid_size)[i]);
        total_size += (*local_upid_size)[i];
    }

    *local_upids = (char *) MPL_malloc(total_size * sizeof(char));
    curr_ptr = (*local_upids);
    for (i = 0; i < comm->local_size; i++) {
        memcpy(curr_ptr, &temp_buf[i * len], (*local_upid_size)[i]);
        curr_ptr += (*local_upid_size)[i];
    }

  fn_exit:
    if (temp_buf)
        MPL_free(temp_buf);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_upids_to_lupids(int size,
                                           size_t * remote_upid_size,
                                           char *remote_upids, int **remote_lupids)
{
    int i, mpi_errno = MPI_SUCCESS;
    size_t sz = MPIDI_UCX_global.max_addr_len;
    int *new_avt_procs;
    char **new_upids;
    int n_new_procs = 0;
    int max_n_avts;
    char *curr_upid;
    new_avt_procs = (int *) MPL_malloc(size * sizeof(int));
    new_upids = (char **) MPL_malloc(size * sizeof(char *));
    max_n_avts = MPIDIU_get_max_n_avts();
    if (MPIDI_UCX_global.pmi_addr_table == NULL) {
        MPIDI_NMI_allocate_address_table();
    }

    curr_upid = remote_upids;
    for (i = 0; i < size; i++) {
        int j, k;
        int found = 0;

        for (k = 0; k < max_n_avts; k++) {
            if (MPIDIU_get_av_table(k) == NULL) {
                continue;
            }
            for (j = 0; j < MPIDIU_get_av_table(k)->size; j++) {
                if (!memcmp(&MPIDI_UCX_global.pmi_addr_table[j * sz],
                            curr_upid, remote_upid_size[i])) {
                    (*remote_lupids)[i] = MPIDIU_LUPID_CREATE(k, j);
                    found = 1;
                    break;
                }
            }
        }

        if (!found) {
            new_avt_procs[n_new_procs] = i;
            new_upids[n_new_procs] = curr_upid;
            n_new_procs++;
        }
        curr_upid += remote_upid_size[i];
    }

    /* FIXME: add support for dynamic processes */
    /* create new av_table, insert processes */
    if (n_new_procs > 0) {
        mpi_errno = -1;
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPL_free(new_avt_procs);
    MPL_free(new_upids);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                       int size, const int lpids[])
{
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_mpi_free_mem(void *ptr)
{
    return MPIDI_CH4U_mpi_free_mem(ptr);
}

static inline void *MPIDI_NM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    return MPIDI_CH4U_mpi_alloc_mem(size, info_ptr);
}

#endif /* UCX_INIT_H_INCLUDED */
