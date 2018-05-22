/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpiimpl.h"
#include "recexchalgo.h"

int MPII_Recexchalgo_init()
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
#undef FUNCNAME
#define FUNCNAME MPII_Recexchalgo_get_neighbors
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Recexchalgo_get_neighbors(int rank, int nranks, int *k_,
                                   int *step1_sendto, int **step1_recvfrom_, int *step1_nrecvs,
                                   int ***step2_nbrs_, int *step2_nphases, int *p_of_k_, int *T_)
{
    int mpi_errno = MPI_SUCCESS;
    int i, j, k;
    int p_of_k = 1, log_p_of_k = 0, rem, T, newrank;
    int **step2_nbrs;
    int *step1_recvfrom;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPII_RECEXCHALGO_GET_NEIGHBORS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPII_RECEXCHALGO_GET_NEIGHBORS);

    k = *k_;
    if (nranks < k)     /* If size of the communicator is less than k, reduce the value of k */
        k = (nranks > 2) ? nranks : 2;
    *k_ = k;

    /* Calculate p_of_k, p_of_k is the largest power of k that is less than nranks */
    while (p_of_k <= nranks) {
        p_of_k *= k;
        log_p_of_k++;
    }
    p_of_k /= k;
    log_p_of_k--;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "allocate memory for storing communication pattern\n"));
    step1_recvfrom = *step1_recvfrom_ = (int *) MPL_malloc(sizeof(int) * (k - 1), MPL_MEM_COLL);
    step2_nbrs = *step2_nbrs_ = (int **) MPL_malloc(sizeof(int *) * log_p_of_k, MPL_MEM_COLL);
    for (i = 0; i < log_p_of_k; i++) {
        (*step2_nbrs_)[i] = (int *) MPL_malloc(sizeof(int) * (k - 1), MPL_MEM_COLL);
    }

    *step2_nphases = log_p_of_k;

    rem = nranks - p_of_k;
    /* rem is the number of ranks that do not particpate in Step 2
     * We need to identify these non-participating ranks. This is done in the following way.
     * The first T ranks are divided into sets of k consecutive ranks each.
     * In each of these sets, the first k-1 ranks are the non-participating
     * ranks while the last rank is the participating rank.
     * The non-participating ranks send their data to the participating rank
     * in their corresponding set.
     */
    T = (rem * k) / (k - 1);
    *T_ = T;
    *p_of_k_ = p_of_k;


    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "step 1 nbr calculation started. T is %d \n", T));
    *step1_nrecvs = 0;
    *step1_sendto = -1;

    /* Step 1 */
    if (rank < T) {
        if (rank % k != (k - 1)) {      /* I am a non-participating rank */
            *step1_sendto = rank + (k - 1 - rank % k);  /* partipating rank to send the data to */
            /* if the corresponding participating rank is not in T,
             * then send to the Tth rank to preserve non-commutativity */
            if (*step1_sendto > T - 1)
                *step1_sendto = T;
            newrank = -1;       /* tag this rank as non-participating */
        } else {        /* participating rank */
            for (i = 0; i < k - 1; i++) {
                step1_recvfrom[i] = rank - i - 1;
            }
            *step1_nrecvs = k - 1;
            newrank = rank / k; /* this is the new rank amongst the set of participating ranks */
        }
    } else {    /* rank >= T */
        newrank = rank - rem;

        if (rank == T && (T - 1) % k != k - 1 && T >= 1) {
            int nsenders = (T - 1) % k + 1;     /* number of ranks sending their data to me in Step 1 */

            for (j = nsenders - 1; j >= 0; j--) {
                step1_recvfrom[nsenders - 1 - j] = T - nsenders + j;
            }
            *step1_nrecvs = nsenders;
        }
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "step 1 nbr computation completed\n"));

    /* Step 2 */
    if (*step1_sendto == -1) {  /* calulate step2_nbrs only for participating ranks */
        int *digit = (int *) MPL_malloc(sizeof(int) * log_p_of_k, MPL_MEM_COLL);
        int temprank = newrank, index = 0, remainder;
        int mask = 0x1;
        int phase = 0, cbit, cnt, nbr, power;

        /* calculate the digits in base k representation of newrank */
        for (i = 0; i < log_p_of_k; i++)
            digit[i] = 0;
        while (temprank != 0) {
            remainder = temprank % k;
            temprank = temprank / k;
            digit[index] = remainder;
            index++;
        }

        while (mask < p_of_k) {
            cbit = digit[phase];        /* phase_th digit changes in this phase, obtain its original value */
            cnt = 0;
            for (i = 0; i < k; i++) {   /* there are k-1 neighbors */
                if (i != cbit) {        /* do not generate yourself as your nieighbor */
                    digit[phase] = i;   /* this gets us the base k representation of the neighbor */

                    /* calculate the base 10 value of the neighbor rank */
                    nbr = 0;
                    power = 1;
                    for (j = 0; j < log_p_of_k; j++) {
                        nbr += digit[j] * power;
                        power *= k;
                    }

                    /* calculate its real rank and store it */
                    step2_nbrs[phase][cnt] =
                        (nbr < rem / (k - 1)) ? (nbr * k) + (k - 1) : nbr + rem;
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST, "step2_nbrs[%d][%d] is %d \n", phase, cnt,
                                     step2_nbrs[phase][cnt]));
                    cnt++;
                }
            }
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "step 2, phase %d nbr calculation completed\n", phase));
            digit[phase] = cbit;        /* reset the digit to original value */
            phase++;
            mask *= k;
        }

        MPL_free(digit);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPII_RECEXCHALGO_GET_NEIGHBORS);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
