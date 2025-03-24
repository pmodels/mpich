/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidch4r.h"
#include "ch4i_comm.h"

/* number of leading zeros, from Hacker's Delight */
static int nlz(uint32_t x)
{
    uint32_t y;
    int n = 32;
/* *INDENT-OFF* */
    y = x >> 16; if (y != 0) { n = n - 16; x = y; }
    y = x >> 8; if (y != 0) { n = n - 8; x = y; }
    y = x >> 4; if (y != 0) { n = n - 4; x = y; }
    y = x >> 2; if (y != 0) { n = n - 2; x = y; }
    y = x >> 1; if (y != 0) { return n - 2; }
/* *INDENT-ON* */
    return n - x;
}

/* 0xab00000cde -> 0xabcde if num_low_bits is 12 */
static uint64_t shrink(uint64_t x, int num_low_bits)
{
    return ((x >> 32) << num_low_bits) + (x & 0xffffffff);
}

int MPIDI_check_disjoint_lpids(MPIR_Lpid lpids1[], int n1, MPIR_Lpid lpids2[], int n2)
{
    int mpi_errno = MPI_SUCCESS;
    uint32_t gpidmaskPrealloc[128];
    uint32_t *gpidmask;
    MPIR_CHKLMEM_DECL();

    MPIR_FUNC_ENTER;

    /* Taking the knowledge that gpid are two 32-bit avtid + lpid, both are
     * often in small range. If we shrink the middle gaps between avtid and
     * lpid, the number shouldn't be too large. */

    /* Find the max low-32-bit gpid */
    uint64_t max_lpid = 0;
    for (int i = 0; i < n1; i++) {
        uint64_t n = lpids1[i] & 0xffffffff;
        if (n > max_lpid)
            max_lpid = n;
    }
    for (int i = 0; i < n2; i++) {
        uint64_t n = lpids2[i] & 0xffffffff;
        if (n > max_lpid)
            max_lpid = n;
    }

    int num_low_bits = 32 - nlz((uint32_t) max_lpid);

    uint64_t max_gpid = 0;
    for (int i = 0; i < n1; i++) {
        uint64_t n = shrink(lpids1[i], num_low_bits);
        if (n > max_gpid)
            max_gpid = n;
    }
    for (int i = 0; i < n2; i++) {
        uint64_t n = shrink(lpids2[i], num_low_bits);
        if (n > max_gpid)
            max_gpid = n;
    }

    uint64_t mask_size = (max_gpid / 32) + 1;
    if (mask_size > 128) {
        MPIR_CHKLMEM_MALLOC(gpidmask, mask_size * sizeof(uint32_t));
    } else {
        gpidmask = gpidmaskPrealloc;
    }

    /* zero the bitvector array */
    memset(gpidmask, 0x00, mask_size * sizeof(*gpidmask));

    /* Set the bits for the first array */
    for (int i = 0; i < n1; i++) {
        uint64_t n = shrink(lpids1[i], num_low_bits);
        int idx = n / 32;
        int bit = n % 32;
        gpidmask[idx] = gpidmask[idx] | (1 << bit);
        MPIR_Assert(idx < mask_size);
    }

    /* Look for any duplicates in the second array */
    for (int i = 0; i < n2; i++) {
        uint64_t n = shrink(lpids2[i], num_low_bits);
        int idx = n / 32;
        int bit = n % 32;
        if (gpidmask[idx] & (1ULL << bit)) {
            MPIR_ERR_SET1(mpi_errno, MPI_ERR_COMM,
                          "**dupprocesses", "**dupprocesses %d", (int) lpids2[i]);
            goto fn_fail;
        }
        /* Add a check on duplicates *within* group 2 */
        gpidmask[idx] = gpidmask[idx] | (1ULL << bit);
        MPIR_Assert(idx < mask_size);
    }

    /* Also fall through for normal return */
  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
