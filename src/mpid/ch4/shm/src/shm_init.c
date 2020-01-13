/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpidimpl.h"
#include "shm_noinline.h"
#include "../posix/posix_noinline.h"
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
#include "../xpmem/xpmem_noinline.h"
#endif

int MPIDI_SHMI_mpi_init_hook(int rank, int size, int *n_vcis_provided, int *tag_bits)
{
    int ret;


    ret = MPIDI_POSIX_mpi_init_hook(rank, size, n_vcis_provided, tag_bits);
    MPIR_ERR_CHECK(ret);

#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    if (MPIR_CVAR_CH4_XPMEM_LMT_MSG_SIZE != -1) {
        ret = MPIDI_XPMEM_mpi_init_hook(rank, size, n_vcis_provided, tag_bits);
        MPIR_ERR_CHECK(ret);
    }
#endif

  fn_exit:
    return ret;
  fn_fail:
    goto fn_exit;
}

int MPIDI_SHMI_mpi_finalize_hook(void)
{
    int ret;


#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    if (MPIR_CVAR_CH4_XPMEM_LMT_MSG_SIZE != -1) {
        ret = MPIDI_XPMEM_mpi_finalize_hook();
        MPIR_ERR_CHECK(ret);
    }
#endif

    ret = MPIDI_POSIX_mpi_finalize_hook();
    MPIR_ERR_CHECK(ret);

  fn_exit:
    return ret;
  fn_fail:
    goto fn_exit;
}

int MPIDI_SHMI_get_vci_attr(int vci)
{
    int ret;


    ret = MPIDI_POSIX_get_vci_attr(vci);

    return ret;
}

int MPIDI_SHMI_progress(int vci, int blocking)
{
    int ret;


    ret = MPIDI_POSIX_progress(blocking);

    return ret;
}
