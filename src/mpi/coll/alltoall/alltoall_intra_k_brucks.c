/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ALLTOALL_BRUCKS_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        radix (k) value for generic transport brucks based alltoall

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* Brucks Pack, UnPack (PUP) utility function
 * This functions packs (unpacks) non-contiguous (contiguous)
 * data from (to) rbuf to (from) pupbuf. It goes to every offset
 * that has the value "digitval" at the "phase"th digit in the
 * base k representation of the offset. The argument pow_k_phase
 * is MPL_ipow(k, phase) where  phase corresponds to the phase in
 * the brucks algorithm. */
static int
brucks_sched_pup(int pack, void *rbuf, void *pupbuf, MPI_Datatype rtype, MPI_Aint count,
                 int pow_k_phase, int k, int digitval, int comm_size, int *pupsize)
{
    MPI_Aint type_extent, type_lb, type_true_extent;
    int offset, nconsecutive_occurrences, delta;
    int mpi_errno = MPI_SUCCESS;

    MPIR_Datatype_get_extent_macro(rtype, type_extent);
    MPIR_Type_get_true_extent_impl(rtype, &type_lb, &type_true_extent);
    type_extent = MPL_MAX(type_extent, type_true_extent);

    /* first offset where the phase'th bit has value digitval */
    offset = pow_k_phase * digitval;
    /* number of consecutive occurrences of digitval */
    nconsecutive_occurrences = pow_k_phase;
    /* distance between non-consecutive occurrences of digitval */
    delta = (k - 1) * pow_k_phase;

    *pupsize = 0;       /* points to the first empty location in pupbuf */
    while (offset < comm_size) {
        if (pack) {
            mpi_errno = MPIR_Localcopy((char *) rbuf + offset * count * type_extent, count, rtype,
                                       (char *) pupbuf + *pupsize, count, rtype);
            if (mpi_errno) {
                MPIR_ERR_POP(mpi_errno);
            }
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "packing rbuf+%ld to pupbuf+%d\n",
                             offset * count * type_extent, *pupsize));
        } else {
            mpi_errno = MPIR_Localcopy((char *) pupbuf + *pupsize, count, rtype,
                                       (char *) rbuf + offset * count * type_extent, count, rtype);
            if (mpi_errno) {
                MPIR_ERR_POP(mpi_errno);
            }
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "unpacking from pupbuf+%d to rbuf+%ld\n", *pupsize,
                             offset * count * type_extent));
        }

        offset += 1;
        nconsecutive_occurrences -= 1;

        if (nconsecutive_occurrences == 0) {    /* consecutive occurrences are over */
            offset += delta;
            nconsecutive_occurrences = pow_k_phase;
        }

        *pupsize += count * type_extent;
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Algorithm: Bruck's algorithm with radix k
 *
 * This algorithm is used for small message sizes. It is a modified version
 * from the IEEE TPDS Nov 97 paper by Jehoshua Bruck et al. It modifies
 * the original bruck algorithm to work with any radix k(k >= 2). It takes
 * logk(p) 'phases' to complete and performs (k - 1) sends and recvs
 * per 'phase'. When working with radix k, it reads the index of each
 * data location in base k. At each substep j, where 0 < j < k, in a phase,
 * it packs and sends (recvs and unpacks) the data whose 'phase'th least
 * significant digit is j.
 * For cases when p is a perfect power of k,
 *       Cost = logk(p) * alpha + (n/p) * (p - 1) * logk(p) * beta
 * where n is the total amount of data a process needs to send to all
 * other processes. The algorithm also works for non power of k cases.
 * */

int MPIR_Alltoall_intra_k_brucks(const void *sendbuf,
                                 MPI_Aint sendcnt,
                                 MPI_Datatype sendtype,
                                 void *recvbuf,
                                 MPI_Aint recvcnt,
                                 MPI_Datatype recvtype, MPIR_Comm * comm, int k,
                                 MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int i, j;
    int rank, size;
    int nphases, max;
    int p_of_k;                 /* largest power of k that is smaller than 'size' */
    int is_inplace;
    MPI_Aint s_extent, s_lb, r_extent, r_lb;
    MPI_Aint s_true_extent, r_true_extent;
    int delta, src, dst;
    void **tmp_sbuf = NULL, **tmp_rbuf = NULL;
    int packsize;
    void *tmp_buf = NULL;
    const void *senddata;
    MPIR_Request **reqs;
    int num_reqs = 0;

    MPIR_CHKLMEM_DECL(4);

    MPIR_CHKLMEM_MALLOC(reqs, MPIR_Request **, (2 * (k - 1) * sizeof(MPIR_Request *)), mpi_errno,
                        "reqs", MPL_MEM_BUFFER);

    is_inplace = (sendbuf == MPI_IN_PLACE);

    rank = MPIR_Comm_rank(comm);
    size = MPIR_Comm_size(comm);

    nphases = 0;
    max = size - 1;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "alltoall_blocking_brucks: num_ranks: %d, k: %d \n", size, k));
    /* calculate the number of bits required to represent a rank in base k */
    while (max) {
        nphases++;
        max /= k;
    }

    /* calculate largest power of k that is smaller than 'size'.
     * This is used for allocating temporary space for sending
     * and receiving data. */
    p_of_k = 1;
    for (i = 0; i < nphases - 1; i++) {
        p_of_k *= k;
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Num phases: %d\n", nphases));

    if (is_inplace) {
        sendcnt = recvcnt;
        sendtype = recvtype;
    }

    MPIR_Datatype_get_extent_macro(sendtype, s_extent);
    MPIR_Type_get_true_extent_impl(sendtype, &s_lb, &s_true_extent);
    s_extent = MPL_MAX(s_extent, s_true_extent);

    MPIR_Datatype_get_extent_macro(recvtype, r_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &r_lb, &r_true_extent);
    r_extent = MPL_MAX(r_extent, r_true_extent);

#ifdef MPL_USE_DBG_LOGGING
    {
        MPI_Aint s_type_size;
        MPIR_Datatype_get_size_macro(sendtype, s_type_size);
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST,
                         "send_type_size: %ld, send_type_extent: %ld, send_count: %ld\n",
                         s_type_size, s_extent, sendcnt));
    }
#endif
    /* temporary buffer used for rotation, so used as sendbuf when inplace is true */
    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, recvcnt * size * r_extent, mpi_errno, "tmp_buf",
                        MPL_MEM_COLL);

    if (is_inplace) {
        mpi_errno =
            MPIR_Localcopy(recvbuf, size * recvcnt, recvtype, tmp_buf, size * recvcnt, recvtype);
        if (mpi_errno) {
            MPIR_ERR_POP(mpi_errno);
        }
        senddata = tmp_buf;
    } else {
        senddata = sendbuf;
    }

    /* Step 1: rotate the data locally */
    mpi_errno = MPIR_Localcopy((void *) ((char *) senddata + rank * sendcnt * s_extent),
                               (size - rank) * sendcnt, sendtype, recvbuf,
                               (size - rank) * recvcnt, recvtype);
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }
    mpi_errno = MPIR_Localcopy(senddata, rank * sendcnt, sendtype,
                               (void *) ((char *) recvbuf + (size - rank) * recvcnt * r_extent),
                               rank * recvcnt, recvtype);
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Step 1 data rotation scheduled\n"));

    /* Step 2: Allocate buffer space for packing/receiving data for every phase */
    delta = 1;

    MPIR_CHKLMEM_MALLOC(tmp_sbuf, void **, sizeof(void *) * (k - 1), mpi_errno, "tmp_sbuf",
                        MPL_MEM_COLL);
    MPIR_CHKLMEM_MALLOC(tmp_rbuf, void **, sizeof(void *) * (k - 1), mpi_errno, "tmp_rbuf",
                        MPL_MEM_COLL);

    for (j = 0; j < k - 1; j++) {
        tmp_sbuf[j] = (void *) MPL_malloc(r_extent * recvcnt * p_of_k, MPL_MEM_COLL);
        MPIR_ERR_CHKANDJUMP(!tmp_sbuf[j], mpi_errno, MPI_ERR_OTHER, "**nomem");
        tmp_rbuf[j] = (void *) MPL_malloc(r_extent * recvcnt * p_of_k, MPL_MEM_COLL);
        MPIR_ERR_CHKANDJUMP(!tmp_rbuf[j], mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Allocated temporary buffer space for packing\n"));

    packsize = 0;

    /* Post (k - 1) sends/recvs for each of the nphases */
    for (i = 0; i < nphases; i++) {
        num_reqs = 0;
        for (j = 1; j < k; j++) {       /* for every non-zero value of digitval */
            if (delta * j >= size)      /* if the first location exceeds comm size, nothing is to be sent */
                break;

            src = (rank - delta * j + size) % size;
            dst = (rank + delta * j) % size;

            /* pack data to be sent */
            mpi_errno =
                brucks_sched_pup(1, recvbuf, tmp_sbuf[j - 1], recvtype, recvcnt, delta, k, j,
                                 size, &packsize);
            if (mpi_errno) {
                MPIR_ERR_POP(mpi_errno);
            }

            mpi_errno =
                MPIC_Irecv(tmp_rbuf[j - 1], packsize, MPI_BYTE, src, MPIR_ALLTOALL_TAG, comm,
                           &reqs[num_reqs++]);
            if (mpi_errno) {
                MPIR_ERR_POP(mpi_errno);
            }

            mpi_errno =
                MPIC_Isend(tmp_sbuf[j - 1], packsize, MPI_BYTE, dst, MPIR_ALLTOALL_TAG, comm,
                           &reqs[num_reqs++], errflag);
            if (mpi_errno) {
                MPIR_ERR_POP(mpi_errno);
            }
        }

        MPIC_Waitall(num_reqs, reqs, MPI_STATUSES_IGNORE, errflag);

        for (j = 1; j < k; j++) {
            if (delta * j >= size)      /* if the first location exceeds comm size, nothing is to be sent */
                break;

            /* Unpack received data */
            mpi_errno =
                brucks_sched_pup(0, recvbuf, tmp_rbuf[j - 1], recvtype, recvcnt, delta, k, j,
                                 size, &packsize);
            if (mpi_errno) {
                MPIR_ERR_POP(mpi_errno);
            }
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "phase %d, digit %d unpacking scheduled\n", i, j));
        }
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "phase %d scheduled\n", i));

        delta *= k;
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Step 2 scheduled\n"));

    /* Step 3: rotate the buffer */
    /* TODO: MPICH implementation does some lower_bound adjustment here for
     * derived datatypes, skipping that for now, will come back
     * to it later on - will require adding API for getting true_lb */

    mpi_errno = MPIR_Localcopy((void *) ((char *) recvbuf + (rank + 1) * recvcnt * r_extent),
                               (size - rank - 1) * recvcnt, recvtype, tmp_buf,
                               (size - rank - 1) * recvcnt, recvtype);
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }
    mpi_errno = MPIR_Localcopy(recvbuf, (rank + 1) * recvcnt, recvtype,
                               (void *) ((char *) tmp_buf +
                                         (size - rank - 1) * recvcnt * r_extent),
                               (rank + 1) * recvcnt, recvtype);
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }

    /* invert the buffer now to get the result in desired order */
    for (i = 0; i < size; i++) {
        mpi_errno = MPIR_Localcopy((char *) tmp_buf + i * recvcnt * r_extent, recvcnt, recvtype,
                                   (void *) ((char *) recvbuf +
                                             (size - i - 1) * recvcnt * r_extent), recvcnt,
                                   recvtype);
        if (mpi_errno) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Step 3: data rearrangement scheduled\n"));

    for (i = 0; i < k - 1; i++) {
        MPL_free(tmp_sbuf[i]);
        MPL_free(tmp_rbuf[i]);
    }

    MPIR_CHKLMEM_FREEALL();

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
