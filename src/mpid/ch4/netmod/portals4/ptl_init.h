/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef PTL_INIT_H_INCLUDED
#define PTL_INIT_H_INCLUDED

#include "mpidch4r.h"
#include "ptl_types.h"
#include "ptl_impl.h"
#include "portals4.h"

static inline int MPIDI_PTL_append_overflow(size_t i)
{
    ptl_me_t me;
    ptl_process_t id_any;

    id_any.phys.pid = PTL_PID_ANY;
    id_any.phys.nid = PTL_NID_ANY;

    me.start = MPIDI_PTL_global.overflow_bufs[i];
    me.length = MPIDI_PTL_OVERFLOW_BUFFER_SZ;
    me.ct_handle = PTL_CT_NONE;
    me.uid = PTL_UID_ANY;
    me.options = (PTL_ME_OP_PUT | PTL_ME_MANAGE_LOCAL | PTL_ME_NO_TRUNCATE | PTL_ME_MAY_ALIGN |
                  PTL_ME_IS_ACCESSIBLE | PTL_ME_EVENT_LINK_DISABLE);
    me.match_id = id_any;
    me.match_bits = 0;
    me.ignore_bits = ~((ptl_match_bits_t) 0);
    me.min_free = MPIDI_PTL_MAX_AM_EAGER_SZ;

    return PtlMEAppend(MPIDI_PTL_global.ni, MPIDI_PTL_global.pt, &me, PTL_OVERFLOW_LIST, (void *) i,
                       &MPIDI_PTL_global.overflow_me_handles[i]);
}

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
                                         int spawned,
                                         int *n_vnis_provided)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    ptl_md_t md;
    ptl_ni_limits_t desired;
    ptl_process_t my_ptl_id;
    int key_max_sz;
    int val_max_sz, val_sz_left;
    int name_max_sz;
    int len, i;
    char *keyS, *valS, *buscard;

    /* Make sure our IOV is the same as portals4's IOV */
    MPIR_Assert(sizeof(ptl_iovec_t) == sizeof(MPL_IOV));
    MPIR_Assert(((void *) &(((ptl_iovec_t *) 0)->iov_base)) ==
                ((void *) &(((MPL_IOV *) 0)->MPL_IOV_BUF)));
    MPIR_Assert(((void *) &(((ptl_iovec_t *) 0)->iov_len)) ==
                ((void *) &(((MPL_IOV *) 0)->MPL_IOV_LEN)));
    MPIR_Assert(sizeof(((ptl_iovec_t *) 0)->iov_len) == sizeof(((MPL_IOV *) 0)->MPL_IOV_LEN));

    *n_vnis_provided = 1;

    /* init portals */
    ret = PtlInit();
    MPIDI_PTL_CHK_STATUS(ret, PtlInit);

    /* /\* do an interface pre-init to get the default limits struct *\/ */
    /* ret = PtlNIInit(PTL_IFACE_DEFAULT, PTL_NI_MATCHING | PTL_NI_PHYSICAL, */
    /*                 PTL_PID_ANY, NULL, &desired, &MPIDI_PTL_global.ni_handle); */
    /* MPIDI_PTL_CHK_STATUS(ret, PtlNIInit); */

    /* /\* finalize the interface so we can re-init with our desired maximums *\/ */
    /* ret = PtlNIFini(MPIDI_PTL_global.ni_handle); */
    /* MPIDI_PTL_CHK_STATUS(ret, PtlNIFini); */

    /* /\* set higher limits if they are determined to be too low *\/ */
    /* if (desired.max_unexpected_headers < UNEXPECTED_HDR_COUNT && getenv("PTL_LIM_MAX_UNEXPECTED_HEADERS") == NULL) */
    /*     desired.max_unexpected_headers = UNEXPECTED_HDR_COUNT; */
    /* if (desired.max_list_size < LIST_SIZE && getenv("PTL_LIM_MAX_LIST_SIZE") == NULL) */
    /*     desired.max_list_size = LIST_SIZE; */
    /* if (desired.max_entries < ENTRY_COUNT && getenv("PTL_LIM_MAX_ENTRIES") == NULL) */
    /*     desired.max_entries = ENTRY_COUNT; */

    /* do the real init */
    ret = PtlNIInit(PTL_IFACE_DEFAULT, PTL_NI_MATCHING | PTL_NI_PHYSICAL,
                    PTL_PID_ANY, NULL, &MPIDI_PTL_global.ni_limits, &MPIDI_PTL_global.ni);
    MPIDI_PTL_CHK_STATUS(ret, PtlNIInit);

    /* allocate EQs: 0 is origin, 1 is target */
    ret = PtlEQAlloc(MPIDI_PTL_global.ni, MPIDI_PTL_EVENT_COUNT, &MPIDI_PTL_global.eqs[0]);
    MPIDI_PTL_CHK_STATUS(ret, PtlEQAlloc);
    ret = PtlEQAlloc(MPIDI_PTL_global.ni, MPIDI_PTL_EVENT_COUNT, &MPIDI_PTL_global.eqs[1]);
    MPIDI_PTL_CHK_STATUS(ret, PtlEQAlloc);

    /* allocate portal */
    ret =
        PtlPTAlloc(MPIDI_PTL_global.ni,
                   PTL_PT_ONLY_USE_ONCE | PTL_PT_ONLY_TRUNCATE | PTL_PT_FLOWCTRL,
                   MPIDI_PTL_global.eqs[1], PTL_PT_ANY, &MPIDI_PTL_global.pt);
    MPIDI_PTL_CHK_STATUS(ret, PtlPTAlloc);

    /* create an MD that covers all of memory */
    md.start = NULL;
    md.length = PTL_SIZE_MAX;
    md.options = 0x0;
    md.eq_handle = MPIDI_PTL_global.eqs[0];
    md.ct_handle = PTL_CT_NONE;
    ret = PtlMDBind(MPIDI_PTL_global.ni, &md, &MPIDI_PTL_global.md);
    MPIDI_PTL_CHK_STATUS(ret, PtlMDBind);

    /* create business card */
    ret = PMI_KVS_Get_key_length_max(&key_max_sz);
    ret = PMI_KVS_Get_value_length_max(&val_max_sz);
    ret = PMI_KVS_Get_name_length_max(&name_max_sz);
    MPIDI_PTL_global.kvsname = MPL_malloc(name_max_sz);
    ret = PMI_KVS_Get_my_name(MPIDI_PTL_global.kvsname, name_max_sz);

    keyS = MPL_malloc(key_max_sz);
    valS = MPL_malloc(val_max_sz);
    buscard = valS;
    val_sz_left = val_max_sz;

    ret = PtlGetId(MPIDI_PTL_global.ni, &my_ptl_id);
    ret =
        MPL_str_add_binary_arg(&buscard, &val_sz_left, "NID", (char *) &my_ptl_id.phys.nid,
                               sizeof(my_ptl_id.phys.nid));
    ret =
        MPL_str_add_binary_arg(&buscard, &val_sz_left, "PID", (char *) &my_ptl_id.phys.pid,
                               sizeof(my_ptl_id.phys.pid));
    ret =
        MPL_str_add_binary_arg(&buscard, &val_sz_left, "PTI", (char *) &MPIDI_PTL_global.pt,
                               sizeof(MPIDI_PTL_global.pt));

    sprintf(keyS, "PTL-%d", rank);
    buscard = valS;
    ret = PMI_KVS_Put(MPIDI_PTL_global.kvsname, keyS, buscard);
    ret = PMI_KVS_Commit(MPIDI_PTL_global.kvsname);
    ret = PMI_Barrier();

    /* get and store business cards in address table */
    MPIDI_PTL_global.addr_table = MPL_malloc(size * sizeof(MPIDI_PTL_addr_t));
    for (i = 0; i < size; i++) {
        sprintf(keyS, "PTL-%d", i);
        ret = PMI_KVS_Get(MPIDI_PTL_global.kvsname, keyS, valS, val_max_sz);
        MPL_str_get_binary_arg(valS, "NID",
                               (char *) &MPIDI_PTL_global.addr_table[i].process.phys.nid,
                               sizeof(MPIDI_PTL_global.addr_table[i].process.phys.nid), &len);
        MPL_str_get_binary_arg(valS, "PID",
                               (char *) &MPIDI_PTL_global.addr_table[i].process.phys.pid,
                               sizeof(MPIDI_PTL_global.addr_table[i].process.phys.pid), &len);
        MPL_str_get_binary_arg(valS, "PTI", (char *) &MPIDI_PTL_global.addr_table[i].pt,
                               sizeof(MPIDI_PTL_global.addr_table[i].pt), &len);
    }

    /* Setup CH4R Active Messages */
    MPIDIG_init(comm_world, comm_self, *n_vnis_provided);
    for (i = 0; i < MPIDI_PTL_NUM_OVERFLOW_BUFFERS; i++) {
        MPIDI_PTL_global.overflow_bufs[i] = MPL_malloc(MPIDI_PTL_OVERFLOW_BUFFER_SZ);
        MPIDI_PTL_append_overflow(i);
    }

    MPIDI_PTL_global.node_map = MPL_malloc(size * sizeof(*MPIDI_PTL_global.node_map));
    mpi_errno =
        MPIDI_CH4U_build_nodemap(rank, comm_world, size, MPIDI_PTL_global.node_map,
                                 &MPIDI_PTL_global.max_node_id);

  fn_exit:
    MPL_free(keyS);
    MPL_free(valS);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    int ret, i;

    MPIR_Comm_release(MPIR_Process.comm_world);
    MPIR_Comm_release(MPIR_Process.comm_self);

    MPIDIG_finalize();

    for (i = 0; i < MPIDI_PTL_NUM_OVERFLOW_BUFFERS; i++) {
        ret = PtlMEUnlink(MPIDI_PTL_global.overflow_me_handles[i]);
        MPL_free(MPIDI_PTL_global.overflow_bufs[i]);
    }
    ret = PtlMDRelease(MPIDI_PTL_global.md);
    ret = PtlPTFree(MPIDI_PTL_global.ni, MPIDI_PTL_global.pt);
    ret = PtlEQFree(MPIDI_PTL_global.eqs[1]);
    ret = PtlEQFree(MPIDI_PTL_global.eqs[0]);
    ret = PtlNIFini(MPIDI_PTL_global.ni);
    PtlFini();

    MPL_free(MPIDI_PTL_global.node_map);
    MPL_free(MPIDI_PTL_global.addr_table);
    MPL_free(MPIDI_PTL_global.kvsname);

    return mpi_errno;
}

static inline int MPIDI_NM_get_vni_attr(int vni)
{
    MPIR_Assert(0 <= vni && vni < 1);
    return MPIDI_VNI_TX | MPIDI_VNI_RX;
}

static inline int MPIDI_NM_comm_get_lpid(MPIR_Comm * comm_ptr,
                                         int idx, int *lpid_ptr, MPL_bool is_remote)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size,
                                           char **local_upids)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_upids_to_lupids(int size,
                                           size_t * remote_upid_size,
                                           char *remote_upids, int **remote_lupids)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                       int size, const int lpids[])
{
    MPIR_Assert(0);
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


#endif /* PTL_INIT_H_INCLUDED */
