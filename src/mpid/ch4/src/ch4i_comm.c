/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidch4r.h"
#include "ch4i_comm.h"

int MPIDI_check_disjoint_lupids(int lupids1[], int n1, int lupids2[], int n2)
{
    int i, mask_size, idx, bit, maxlupid = -1;
    int mpi_errno = MPI_SUCCESS;
    uint32_t lupidmaskPrealloc[128];
    uint32_t *lupidmask;
    MPIR_CHKLMEM_DECL(1);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CHECK_DISJOINT_LUPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CHECK_DISJOINT_LUPIDS);

    /* Find the max lupid */
    for (i = 0; i < n1; i++) {
        if (lupids1[i] > maxlupid)
            maxlupid = lupids1[i];
    }
    for (i = 0; i < n2; i++) {
        if (lupids2[i] > maxlupid)
            maxlupid = lupids2[i];
    }

    mask_size = (maxlupid / 32) + 1;

    if (mask_size > 128) {
        MPIR_CHKLMEM_MALLOC(lupidmask, uint32_t *, mask_size * sizeof(uint32_t),
                            mpi_errno, "lupidmask", MPL_MEM_COMM);
    } else {
        lupidmask = lupidmaskPrealloc;
    }

    /* zero the bitvector array */
    memset(lupidmask, 0x00, mask_size * sizeof(*lupidmask));

    /* Set the bits for the first array */
    for (i = 0; i < n1; i++) {
        idx = lupids1[i] / 32;
        bit = lupids1[i] % 32;
        lupidmask[idx] = lupidmask[idx] | (1 << bit);
        MPIR_Assert(idx < mask_size);
    }

    /* Look for any duplicates in the second array */
    for (i = 0; i < n2; i++) {
        idx = lupids2[i] / 32;
        bit = lupids2[i] % 32;
        if (lupidmask[idx] & (1 << bit)) {
            MPIR_ERR_SET1(mpi_errno, MPI_ERR_COMM,
                          "**dupprocesses", "**dupprocesses %d", lupids2[i]);
            goto fn_fail;
        }
        /* Add a check on duplicates *within* group 2 */
        lupidmask[idx] = lupidmask[idx] | (1 << bit);
        MPIR_Assert(idx < mask_size);
    }

    /* Also fall through for normal return */
  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CHECK_DISJOINT_LUPIDS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
