/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "recexchalgo.h"

int MPII_Recexchalgo_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}


int MPII_Recexchalgo_comm_init(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}


int MPII_Recexchalgo_comm_cleanup(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}


/* This function calculates the ranks to/from which the
 * data is sent/recvd in various steps/phases of recursive exchange
 * algorithm. Recursive exchange is divided into three steps - Step 1, 2 and 3.
 * Step 2 is the main step that does the recursive exchange algorithm.
 * Step 1 and Step 3 are done to handle non-power-of-k number of ranks.
 * Only p_of_k (largest power of k that is less than n) ranks participate in Step 2.
 * In Step 1, the non partcipating ranks send their data to ranks
 * participating in Step 2. In Step 3, the ranks that participated in Step 2 send
 * the final data to non-partcipating ranks.
*/
int MPII_Recexchalgo_start(int rank, int nranks, int k, MPII_Recexchalgo_t *recexch)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    recexch->step1_recvfrom = NULL;
    recexch->nbr_bufs = NULL;
    recexch->is_commutative = 0;
    recexch->myidxes = NULL;

    if (nranks <= k) {
        /* trivial case, only step 1 and step 3 */
        recexch->k = nranks;
        recexch->in_step2 = 0;
        recexch->step2_nphases = 0;

        if (rank < nranks - 1) {
            recexch->step1_sendto = nranks - 1;
        } else {
            int n = nranks - 1;
            recexch->step1_nrecvs = n;
            if (n > 0) {
                recexch->step1_recvfrom = (int *) MPL_malloc(sizeof(int) * n, MPL_MEM_COLL);
                MPIR_ERR_CHKANDJUMP(!recexch->step1_recvfrom, mpi_errno, MPI_ERR_OTHER, "**nomem");
                for (int i = 0; i < n; i++) {
                    recexch->step1_recvfrom[i] = nranks - 1;
                }
            }
        }

        goto fn_exit;
    }

    /* Calculate p_of_k, p_of_k is the largest power of k that is less than nranks */
    int p_of_k = 1;
    int log_p_of_k = 0;
    while (p_of_k <= nranks) {
        p_of_k *= k;
        log_p_of_k++;
    }
    p_of_k /= k;
    log_p_of_k--;

    recexch->p_of_k = p_of_k;
    recexch->step2_nphases = log_p_of_k;

    int rem, T, newrank;
    /* rem is the number of ranks that do not particpate in Step 2
     * We need to identify these non-participating ranks. This is done in the following way.
     * The first T ranks are divided into sets of k consecutive ranks each.
     * In each of these sets, the first k-1 ranks are the non-participating
     * ranks while the last rank is the participating rank.
     * The non-participating ranks send their data to the participating rank
     * in their corresponding set.
     */
    rem = nranks - p_of_k;
    int a = rem / (k - 1);
    int b = rem % (k - 1);
    T = a * k + b;

    recexch->T = T;

    recexch->step1_nrecvs = 0;

    /* Step 1 */
    if (rank < T) {
        if (rank % k != (k - 1)) {
            /* I am a non-participating rank */
            recexch->step1_nrecvs = 0;
            recexch->step1_sendto = MPL_MIN(rank + (k - 1 - rank % k), T);
            recexch->in_step2 = false;
            newrank = -1;
        } else {
            recexch->step1_nrecvs = k - 1;
            recexch->step1_recvfrom = (int *) MPL_malloc(sizeof(int) * (k - 1), MPL_MEM_COLL);
            MPIR_ERR_CHKANDJUMP(!recexch->step1_recvfrom, mpi_errno, MPI_ERR_OTHER, "**nomem");

            for (int i = 0; i < k - 1; i++) {
                recexch->step1_recvfrom[i] = rank - i - 1;
            }

            recexch->in_step2 = true;
            newrank = rank / k;
        }
    } else if (rank == T) {
        recexch->step1_nrecvs = T % k;
        for (int i = 0; recexch->step1_nrecvs; i++) {
            recexch->step1_recvfrom[i] = T - 1 - i;
        }

        recexch->in_step2 = true;
        newrank = rank - rem;
    } else {
        recexch->in_step2 = true;
        newrank = rank - rem;
    }

    /* Step 2 */
    recexch->step2_nbrs = (int **) MPL_malloc(sizeof(int *) * log_p_of_k, MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!recexch->step2_nbrs, mpi_errno, MPI_ERR_OTHER, "**nomem");

    for (int i = 0; i < log_p_of_k; i++) {
        recexch->step2_nbrs[i] = (int *) MPL_malloc(sizeof(int) * (k - 1), MPL_MEM_COLL);
        MPIR_ERR_CHKANDJUMP(!recexch->step2_nbrs[i], mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    if (recexch->in_step2) {
        /* calculate step2_nbrs only for participating ranks */
        int *digit = (int *) MPL_malloc(sizeof(int) * log_p_of_k, MPL_MEM_COLL);
        MPIR_Assert(digit != NULL);
        int temprank = newrank;
        int mask = 0x1;
        int phase = 0, cbit, cnt, nbr, power;

        /* calculate the digits in base k representation of newrank */
        for (int i = 0; i < log_p_of_k; i++)
            digit[i] = 0;

        int remainder, i_digit = 0;
        while (temprank != 0) {
            remainder = temprank % k;
            temprank = temprank / k;
            digit[i_digit] = remainder;
            i_digit++;
        }

        while (mask < p_of_k) {
            cbit = digit[phase];        /* phase_th digit changes in this phase, obtain its original value */
            cnt = 0;
            for (int i = 0; i < k; i++) {   /* there are k-1 neighbors */
                if (i != cbit) {        /* do not generate yourself as your nieighbor */
                    digit[phase] = i;   /* this gets us the base k representation of the neighbor */

                    /* calculate the base 10 value of the neighbor rank */
                    nbr = 0;
                    power = 1;
                    for (int j = 0; j < log_p_of_k; j++) {
                        nbr += digit[j] * power;
                        power *= k;
                    }

                    /* calculate its real rank and store it */
                    recexch->step2_nbrs[phase][cnt] =
                        (nbr < rem / (k - 1)) ? (nbr * k) + (k - 1) : nbr + rem;
                    cnt++;
                }
            }
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "step 2, phase %d nbr calculation completed", phase));
            digit[phase] = cbit;        /* reset the digit to original value */
            phase++;
            mask *= k;
        }

        MPL_free(digit);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void MPII_Recexchalgo_finish(MPII_Recexchalgo_t *recexch)
{
    if (recexch->in_step2) {
        if (recexch->step1_recvfrom) {
            MPL_free(recexch->step1_recvfrom);
        }

        for (int i = 0; i < recexch->step2_nphases; i++) {
            MPL_free(recexch->step2_nbrs[i]);
        }
        MPL_free(recexch->step2_nbrs);

        if (recexch->nbr_bufs) {
            MPL_free(recexch->nbr_bufs);
        }
    }
}

int MPII_Recexchalgo_origrank_to_step2rank(int rank, int rem, int T, int k)
{
    int step2rank;

    MPIR_FUNC_ENTER;

    step2rank = (rank < T) ? rank / k : rank - rem;

    MPIR_FUNC_EXIT;

    return step2rank;
}


int MPII_Recexchalgo_step2rank_to_origrank(int rank, int rem, int T, int k)
{
    int orig_rank;

    MPIR_FUNC_ENTER;

    orig_rank = (rank < rem / (k - 1)) ? (rank * k) + (k - 1) : rank + rem;

    MPIR_FUNC_EXIT;

    return orig_rank;
}


/* This function calculates the offset and count for send and receive for a given
 * phase in recursive exchange algorithms in collective operations like Allgather,
 * Allgatherv, Reducescatter.
*/
int MPII_Recexchalgo_get_count_and_offset(int rank, int phase, int k, int nranks, int *count,
                                          int *offset)
{
    int mpi_errno = MPI_SUCCESS;
    int step2rank, min, max, orig_max, orig_min;
    int k_power_phase = 1;
    int p_of_k = 1, rem, T;

    MPIR_FUNC_ENTER;

    /* p_of_k is the largest power of k that is less than nranks */
    while (p_of_k <= nranks) {
        p_of_k *= k;
    }
    p_of_k /= k;

    rem = nranks - p_of_k;
    T = (rem * k) / (k - 1);

    /* k_power_phase is k^phase */
    while (phase > 0) {
        k_power_phase *= k;
        phase--;
    }
    /* Calculate rank in step2 */
    step2rank = MPII_Recexchalgo_origrank_to_step2rank(rank, rem, T, k);
    /* Calculate min and max ranks of the range of ranks that 'rank'
     * represents in phase 'phase' */
    min = ((step2rank / k_power_phase) * k_power_phase) - 1;
    max = min + k_power_phase;
    /* convert (min,max] to their original ranks */
    orig_min = (min >= 0) ? MPII_Recexchalgo_step2rank_to_origrank(min, rem, T, k) : min;
    orig_max = MPII_Recexchalgo_step2rank_to_origrank(max, rem, T, k);
    *count = orig_max - orig_min;
    *offset = orig_min + 1;

    MPIR_FUNC_EXIT;

    return mpi_errno;
}


/* This function calculates the digit reversed (in base 'k' representation) rank of a given rank participating in Step 2. It does so in the following steps:
 * 1. Converts the given rank to its Step2 rank
 * 2. Calculates the digit reversed (in base 'k' representation) rank of the Step 2 rank
 * 3. Convert the digit reversed rank in the previous Step to the original rank.
*/
int MPII_Recexchalgo_reverse_digits_step2(int rank, int comm_size, int k)
{
    int i, T, rem, power, step2rank, step2_reverse_rank = 0;
    int pofk = 1, log_pofk = 0;
    int *digit, *digit_reverse;
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL(2);

    MPIR_FUNC_ENTER;

    while (pofk <= comm_size) {
        pofk *= k;
        log_pofk++;
    }
    MPIR_Assert(log_pofk > 0);
    pofk /= k;
    log_pofk--;

    rem = comm_size - pofk;
    T = (rem * k) / (k - 1);

    /* step2rank is the rank in the particiapting ranks group of recursive exchange step2 */
    step2rank = MPII_Recexchalgo_origrank_to_step2rank(rank, rem, T, k);

    /* calculate the digits in base k representation of step2rank */
    MPIR_CHKLMEM_MALLOC(digit, int *, sizeof(int) * log_pofk,
                        mpi_errno, "digit buffer", MPL_MEM_COLL);
    MPIR_CHKLMEM_MALLOC(digit_reverse, int *, sizeof(int) * log_pofk,
                        mpi_errno, "digit_reverse buffer", MPL_MEM_COLL);
    for (i = 0; i < log_pofk; i++)
        digit[i] = 0;

    int remainder, i_digit = 0;
    while (step2rank != 0) {
        remainder = step2rank % k;
        step2rank = step2rank / k;
        digit[i_digit] = remainder;
        i_digit++;
    }

    /* reverse the number in base k representation to get the step2_reverse_rank
     * which is the reversed rank in the participating ranks group of recursive exchange step2
     */
    for (i = 0; i < log_pofk; i++)
        digit_reverse[i] = digit[log_pofk - 1 - i];
    /*calculate the base 10 value of the reverse rank */
    step2_reverse_rank = 0;
    power = 1;
    for (i = 0; i < log_pofk; i++) {
        step2_reverse_rank += digit_reverse[i] * power;
        power *= k;
    }

    /* calculate the actual rank from logical rank */
    step2_reverse_rank = MPII_Recexchalgo_step2rank_to_origrank(step2_reverse_rank, rem, T, k);
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "reverse_rank is %d", step2_reverse_rank));

    MPIR_FUNC_EXIT;

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return step2_reverse_rank;
  fn_fail:
    /* TODO - Replace this with real error handling */
    MPIR_Assert(MPI_SUCCESS == mpi_errno);
    goto fn_exit;
}
