/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "ch4r_proc.h"

int MPIDIU_get_node_id(MPIR_Comm * comm, int rank, int *id_p)
{
    int mpi_errno = MPI_SUCCESS;
    int avtid = 0, lpid = 0;




    MPIDIU_comm_rank_to_pid(comm, rank, &lpid, &avtid);
    *id_p = MPIDI_global.node_map[avtid][lpid];


    return mpi_errno;
}

int MPIDIU_get_max_node_id(MPIR_Comm * comm, int *max_id_p)
{
    int mpi_errno = MPI_SUCCESS;




    *max_id_p = MPIDI_global.max_node_id;


    return mpi_errno;
}

int MPIDIU_build_nodemap(int myrank, MPIR_Comm * comm, int sz, int *out_nodemap, int *sz_out)
{
    int mpi_errno = MPI_SUCCESS;




    /* The nodemap is built in MPIR_pmi_init. Runtime rebuilding node_map not supported */
    MPIR_Assert(0);


    return mpi_errno;
}

int MPIDIU_get_n_avts(void)
{
    int ret;




    ret = MPIDI_global.avt_mgr.n_avts;


    return ret;
}

int MPIDIU_get_max_n_avts(void)
{
    int ret;




    ret = MPIDI_global.avt_mgr.max_n_avts;


    return ret;
}

int MPIDIU_get_avt_size(int avtid)
{
    int ret;




    ret = MPIDI_av_table[avtid]->size;


    return ret;
}

int MPIDIU_alloc_globals_for_avtid(int avtid)
{
    int mpi_errno = MPI_SUCCESS;
    int *new_node_map = NULL;



    new_node_map = (int *) MPL_malloc(MPIDI_av_table[avtid]->size * sizeof(int), MPL_MEM_ADDRESS);
    MPIR_ERR_CHKANDJUMP(new_node_map == NULL, mpi_errno, MPI_ERR_NO_MEM, "**nomem");
    MPIDI_global.node_map[avtid] = new_node_map;

  fn_exit:

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIU_free_globals_for_avtid(int avtid)
{


    if (avtid > 0) {
        MPL_free(MPIDI_global.node_map[avtid]);
    }
    MPIDI_global.node_map[avtid] = NULL;

    return MPI_SUCCESS;
}

int MPIDIU_get_next_avtid(int *avtid)
{



    if (MPIDI_global.avt_mgr.next_avtid == -1) {        /* out of free avtids */
        int old_max, new_max, i;
        old_max = MPIDI_global.avt_mgr.max_n_avts;
        new_max = old_max + 1;
        MPIDI_global.avt_mgr.free_avtid =
            (int *) MPL_realloc(MPIDI_global.avt_mgr.free_avtid, new_max * sizeof(int),
                                MPL_MEM_ADDRESS);
        for (i = old_max; i < new_max - 1; i++) {
            MPIDI_global.avt_mgr.free_avtid[i] = i + 1;
        }
        MPIDI_global.avt_mgr.free_avtid[new_max - 1] = -1;
        MPIDI_global.avt_mgr.max_n_avts = new_max;
        MPIDI_global.avt_mgr.next_avtid = old_max;
    }

    *avtid = MPIDI_global.avt_mgr.next_avtid;
    MPIDI_global.avt_mgr.next_avtid = MPIDI_global.avt_mgr.free_avtid[*avtid];
    MPIDI_global.avt_mgr.free_avtid[*avtid] = -1;
    MPIDI_global.avt_mgr.n_avts++;
    MPIR_Assert(MPIDI_global.avt_mgr.n_avts <= MPIDI_global.avt_mgr.max_n_avts);

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, (MPL_DBG_FDEST, " new_avtid=%d", *avtid));


    return *avtid;
}

int MPIDIU_free_avtid(int avtid)
{



    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, (MPL_DBG_FDEST, " free_avtid=%d", avtid));
    MPIR_Assert(MPIDI_global.avt_mgr.n_avts > 0);
    MPIDI_global.avt_mgr.free_avtid[avtid] = MPIDI_global.avt_mgr.next_avtid;
    MPIDI_global.avt_mgr.next_avtid = avtid;
    MPIDI_global.avt_mgr.n_avts--;
    /*
     * TODO:
     * If the allowed number of process groups are very large
     * We need to return unsed pages back to OS.
     * madvise(addr, len, MADV_DONTNEED);
     * We need tracking wich page can be returned
     */

    return 0;
}

int MPIDIU_new_avt(int size, int *avtid)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_table_t *new_av_table;



    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, (MPL_DBG_FDEST, " new_avt: size=%d", size));

    MPIDIU_get_next_avtid(avtid);

    new_av_table = (MPIDI_av_table_t *) MPL_malloc(size * sizeof(MPIDI_av_entry_t)
                                                   + sizeof(MPIDI_av_table_t), MPL_MEM_ADDRESS);
    new_av_table->size = size;
    MPIDI_av_table[*avtid] = new_av_table;

    MPIR_Object_set_ref(MPIDI_av_table[*avtid], 0);

    MPIDIU_alloc_globals_for_avtid(*avtid);


    return mpi_errno;
}

int MPIDIU_free_avt(int avtid)
{
    int mpi_errno = MPI_SUCCESS;




    MPIDIU_free_globals_for_avtid(avtid);
    MPL_free(MPIDI_av_table[avtid]);
    MPIDI_av_table[avtid] = NULL;
    MPIDIU_free_avtid(avtid);


    return mpi_errno;
}

int MPIDIU_avt_add_ref(int avtid)
{



    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, (MPL_DBG_FDEST, " incr avtid=%d", avtid));
    MPIR_Object_add_ref(MPIDI_av_table[avtid]);


    return MPI_SUCCESS;
}

int MPIDIU_avt_release_ref(int avtid)
{
    int in_use;




    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, (MPL_DBG_FDEST, " decr avtid=%d", avtid));
    MPIR_Object_release_ref(MPIDIU_get_av_table(avtid), &in_use);
    if (!in_use) {
        MPIDIU_free_avt(avtid);
    }


    return MPI_SUCCESS;
}

int MPIDIU_avt_init(void)
{
    int i, mpi_errno = MPI_SUCCESS;




    MPIDI_global.avt_mgr.mmapped_size = 8 * 4 * 1024;
    MPIDI_global.avt_mgr.max_n_avts = 1;
    MPIDI_global.avt_mgr.next_avtid = 0;
    MPIDI_global.avt_mgr.n_avts = 0;

    MPIDI_av_table = (MPIDI_av_table_t **)
        MPL_mmap(NULL, MPIDI_global.avt_mgr.mmapped_size,
                 PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0, MPL_MEM_ADDRESS);
    MPIR_ERR_CHKANDSTMT(MPIDI_av_table == MAP_FAILED, mpi_errno, MPI_ERR_NO_MEM,
                        goto fn_fail, "**nomem");

    MPIDI_global.node_map = (int **)
        MPL_mmap(NULL, MPIDI_global.avt_mgr.mmapped_size,
                 PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0, MPL_MEM_ADDRESS);
    MPIR_ERR_CHKANDSTMT(MPIDI_global.node_map == MAP_FAILED, mpi_errno,
                        MPI_ERR_NO_MEM, goto fn_fail, "**nomem");

    MPIDI_global.avt_mgr.free_avtid =
        (int *) MPL_malloc(MPIDI_global.avt_mgr.max_n_avts * sizeof(int), MPL_MEM_ADDRESS);
    MPIR_ERR_CHKANDSTMT(MPIDI_global.avt_mgr.free_avtid == NULL, mpi_errno,
                        MPI_ERR_NO_MEM, goto fn_fail, "**nomem");

    for (i = 0; i < MPIDI_global.avt_mgr.max_n_avts - 1; i++) {
        MPIDI_global.avt_mgr.free_avtid[i] = i + 1;
    }
    MPIDI_global.avt_mgr.free_avtid[MPIDI_global.avt_mgr.max_n_avts - 1] = -1;

  fn_exit:

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIU_avt_destroy(void)
{



    MPL_munmap((void *) MPIDI_global.node_map, MPIDI_global.avt_mgr.mmapped_size, MPL_MEM_ADDRESS);
    MPL_munmap((void *) MPIDI_av_table, MPIDI_global.avt_mgr.mmapped_size, MPL_MEM_ADDRESS);
    MPL_free(MPIDI_global.avt_mgr.free_avtid);


    return MPI_SUCCESS;
}

int MPIDIU_build_nodemap_avtid(int myrank, MPIR_Comm * comm, int sz, int avtid)
{
    int ret;



    ret = MPIDIU_build_nodemap(myrank, comm, sz, MPIDI_global.node_map[avtid],
                               &MPIDI_global.max_node_id);


    return ret;
}
