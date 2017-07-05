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

#ifndef COLLRECEXCH_UTIL_H_INCLUDED
#define COLLRECEXCH_UTIL_H_INCLUDED

/*
This is a simple function to compare two integers.
It is used for sorting list of ranks.
*/
MPL_STATIC_INLINE_PREFIX int intcmpfn(const void *a, const void *b)
{
    return (*(int *) a - *(int *) b);
}

/* Converts original rank to rank in step 2 of recursive koupling */
MPL_STATIC_INLINE_PREFIX int orgrank_to_step2rank(int rank, int rem, int T, int k)
{
    int step2rank;
    step2rank = (rank < T) ? rank / k : rank - rem;
    return step2rank;
}

/* Converts rank in step 2 of recursive koupling to original rank */
MPL_STATIC_INLINE_PREFIX int step2rank_to_orgrank(int rank, int rem, int T, int k)
{
    int orig_rank;
    orig_rank = (rank < rem / (k - 1)) ? (rank * k) + (k - 1) : rank + rem;
    return orig_rank;
}

/* This function calculates the offset and count for allgather send and recv */
MPL_STATIC_INLINE_PREFIX
    int allgather_get_offset_and_count(int rank, int phase, int k, int nranks, int *count,
                                       int *offset)
{
    int step2rank, min, max, orig_max, orig_min;
    /* k_power_phase is k^p */
    int k_power_phase = 1;

    /*p_of_k is the largest power of k that is less than nranks */
    int p_of_k = 1, rem, T;
    while (p_of_k <= nranks) {
        p_of_k *= k;
    }
    p_of_k /= k;
    rem = nranks - p_of_k;
    T = (rem * k) / (k - 1);
    while (phase > 0) {
        k_power_phase *= k;
        phase--;
    }
    /* Calculate rank in step2 (participating rank) */
    step2rank = orgrank_to_step2rank(rank, rem, T, k);
    /* Calculate min and max ranks of the range of ranks that 'rank'
     * represents in phase 'phase' */
    min = ((step2rank / k_power_phase) * k_power_phase) - 1;
    max = min + k_power_phase;
    /* convert the (min,max] to their original ranks */
    orig_min = (min >= 0) ? step2rank_to_orgrank(min, rem, T, k) : min;
    orig_max = step2rank_to_orgrank(max, rem, T, k);
    /* count is max-(min-1) */
    *count = orig_max - orig_min;
    *offset = orig_min + 1;
    return 0;
}

/* This function calculates the digit reversed rank for a given rank participating in step2 of
 * recursive kouplingin base 'k' representation. This function converts the given rank into its
 * step2 participating rank and calculates the digit reversed rank and converts the reversed rank
 * into the origianl rank. It returns the reversed rank in base '10' representation */
MPL_STATIC_INLINE_PREFIX int reverse_digits(int rank, int comm_size, int k, int pofk, int log_pofk, int T)
{
    int i, rem, step2rank, step2_reverse_rank = 0;
    int digit[log_pofk];
    int digit_reverse[log_pofk];

    rem = comm_size - pofk;
    /* step2rank is the rank in the particiapting ranks group of recursive koupling step2 */
    step2rank = orgrank_to_step2rank(rank, rem, T, k);
    /*calculate the digits in base k representation of step2rank */
    for (i = 0; i < log_pofk; i++)
        digit[i] = 0;
    int index = 0, remainder;
    while (step2rank != 0) {
        remainder = step2rank % k;
        step2rank = step2rank / k;
        digit[index] = remainder;
        index++;
    }
    /* reverse the number in base k representation to get the step2_reverse_rank
     * which is the reversed rank in the participating ranks group of recursive koupling step2 */
    for (i = 0; i < log_pofk; i++)
        digit_reverse[i] = digit[log_pofk - 1 - i];
    /*calculate the base 10 value of the reverse rank */
    step2_reverse_rank = 0;
    int power = 1;
    for (i = 0; i < log_pofk; i++) {
        step2_reverse_rank += digit_reverse[i] * power;
        power *= k;
    }
    /* calculate the actual rank from logical rank */
    step2_reverse_rank = step2rank_to_orgrank(step2_reverse_rank, rem, T, k);
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"reverse_rank is %d\n", step2_reverse_rank));
    return step2_reverse_rank;
}
#endif /* COLLRECEXCH_UTIL_H_INCLUDED */
