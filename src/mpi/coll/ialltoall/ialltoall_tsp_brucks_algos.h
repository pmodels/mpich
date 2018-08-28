/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IALLTOALL_TSP_BRUCKS_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IALLTOALL_BRUCKS_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        radix (k) value for generic transport brucks based ialltoall

    - name        : MPIR_CVAR_IALLTOALL_BRUCKS_BUFFER_PER_NBR
      category    : COLLECTIVE
      type        : boolean
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, the gentran based brucks algorithm will allocate
        dedicated send and receive buffers for every neighbor in the brucks
        algorithm. Otherwise, it would reuse a single buffer for sending
        and receiving data to/from neighbors

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "tsp_namespace_def.h"

/* Brucks Pack, UnPack (PUP) utility function
* This functions packs (unpacks) non-contiguous (contiguous)
* data from (to) rbuf to (from) pupbuf. It goes to every offset
* that has the value "digitval" at the "phase"th digit in the
* base k representation of the offset. The argument phase corresponds
* to the phase in the brucks algorithm. */
static int
brucks_sched_pup(int pack, void *rbuf, void *pupbuf, MPI_Datatype rtype, int count,
                 int phase, int k, int digitval, int comm_size, int *pupsize,
                 MPIR_TSP_sched_t * sched, int ninvtcs, int *invtcs)
{
    MPI_Aint type_extent, type_lb, type_true_extent;
    int pow_k_phase, offset, nconsecutive_occurrences, delta;
    int *dtcopy_id;
    int counter;
    int sink_id;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_BRUCKS_SCHED_PUP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_BRUCKS_SCHED_PUP);

    MPIR_Datatype_get_extent_macro(rtype, type_extent);
    MPIR_Type_get_true_extent_impl(rtype, &type_lb, &type_true_extent);
    type_extent = MPL_MAX(type_extent, type_true_extent);

    pow_k_phase = MPL_ipow(k, phase);
    /* first offset where the phase'th bit has value digitval */
    offset = pow_k_phase * digitval;
    /* number of consecutive occurences of digitval */
    nconsecutive_occurrences = pow_k_phase;
    /* distance between non-consecutive occurences of digitval */
    delta = (k - 1) * pow_k_phase;

    dtcopy_id = MPL_malloc(sizeof(int) * comm_size, MPL_MEM_COLL);      /* NOTE: We do not need this much large array - make it more accurate */
    MPIR_Assert(dtcopy_id != NULL);
    counter = 0;
    *pupsize = 0;       /* points to the first empty location in pupbuf */
    while (offset < comm_size) {
        if (pack) {
            dtcopy_id[counter++] =
                MPIR_TSP_sched_localcopy((char *) rbuf + offset * count * type_extent, count, rtype,
                                         (char *) pupbuf + *pupsize, count, rtype, sched, ninvtcs,
                                         invtcs);
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "packing rbuf+%ld to pupbuf+%d\n",
                             offset * count * type_extent, *pupsize));
        } else {
            dtcopy_id[counter++] =
                MPIR_TSP_sched_localcopy((char *) pupbuf + *pupsize, count, rtype,
                                         (char *) rbuf + offset * count * type_extent, count, rtype,
                                         sched, ninvtcs, invtcs);
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

        *pupsize += count * type_extent;        /* NOTE: This may not be extent, it might be type_size - CHECK THIS */
    }

    sink_id = MPIR_TSP_sched_selective_sink(sched, counter, dtcopy_id);
    MPL_free(dtcopy_id);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_BRUCKS_SCHED_PUP);

    return sink_id;
}

int
MPIR_TSP_Ialltoall_sched_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                      MPIR_Comm * comm, MPIR_TSP_sched_t * sched, int k,
                                      int buffer_per_phase)
{
    int mpi_errno = MPI_SUCCESS;
    int i, j;
    int pack_ninvtcs, recv_ninvtcs, unpack_ninvtcs;
    int *pack_invtcs, *recv_invtcs, *unpack_invtcs;
    int *invtcs, n_invtcs;
    int rank, size;
    int nphases = 0, max;
    int p_of_k;                 /* largest power of k that is (strictly) smaller than 'size' */
    int is_inplace;
    MPI_Aint s_extent, s_lb, r_extent, r_lb;
    MPI_Aint s_true_extent, r_true_extent;
    int delta, src, dst;
    void ***tmp_sbuf = NULL, ***tmp_rbuf = NULL;
    int *packids, *sendids = NULL, *recvids = NULL, *unpackids = NULL;
    int packsize, num_unpacks_in_last_phase;
    void *tmp_buf = NULL;
    const void *senddata;
    int tag;

    MPIR_CHKLMEM_DECL(6);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLTOALL_SCHED_INTRA_BRUCKS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLTOALL_SCHED_INTRA_BRUCKS);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_CHKLMEM_MALLOC(pack_invtcs, int *, sizeof(int) * k, mpi_errno, "pack_invtcs",
                        MPL_MEM_COLL);
    MPIR_CHKLMEM_MALLOC(recv_invtcs, int *, sizeof(int) * k, mpi_errno, "recv_invtcs",
                        MPL_MEM_COLL);
    MPIR_CHKLMEM_MALLOC(unpack_invtcs, int *, sizeof(int) * k, mpi_errno, "unpack_invtcs",
                        MPL_MEM_COLL);
    MPIR_CHKLMEM_MALLOC(invtcs, int *, sizeof(int) * 6 * k, mpi_errno, "invtcs", MPL_MEM_COLL);

    is_inplace = (sendbuf == MPI_IN_PLACE);

    rank = MPIR_Comm_rank(comm);
    size = MPIR_Comm_size(comm);

    max = size - 1;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Ialltoall_brucks: num_ranks: %d, k: %d \n", size, k));
    /* calculate the number of bits required to represent a rank in base k */
    while (max) {
        nphases++;
        max /= k;
    }

    /* calculate largest power of k that is smaller than 'size'.
     * This is used for allocating temporary space for sending
     * and receving data. */
    p_of_k = 1;
    for (i = 0; i < nphases - 1; i++) {
        p_of_k *= k;
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Num phases: %d\n", nphases));

    if (is_inplace) {
        sendcount = recvcount;
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
        size_t s_type_size;
        MPIR_Datatype_get_size_macro(sendtype, s_type_size);
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST,
                         "send_type_size: %ld, send_type_extent: %ld, send_count: %d\n",
                         s_type_size, s_extent, sendcount));
    }
#endif
    tmp_buf = (void *) MPIR_TSP_sched_malloc(recvcount * size * r_extent, sched);       /* temporary buffer used for rotation
                                                                                         * also used as sendbuf when inplace is true */
    MPIR_Assert(tmp_buf != NULL);

    if (is_inplace) {
        invtcs[0] = MPIR_TSP_sched_localcopy(recvbuf, size * recvcount, recvtype,
                                             tmp_buf, size * recvcount, recvtype, sched, 0, NULL);
        n_invtcs = 1;
        senddata = tmp_buf;
    } else {
        senddata = sendbuf;
        n_invtcs = 0;
    }

    /* Step 1: rotate the data locally */
    MPIR_TSP_sched_localcopy((void *) ((char *) senddata + rank * sendcount * s_extent),
                             (size - rank) * sendcount, sendtype,
                             recvbuf, (size - rank) * recvcount, recvtype, sched, n_invtcs, invtcs);
    MPIR_TSP_sched_localcopy(senddata, rank * sendcount, sendtype,
                             (void *) ((char *) recvbuf + (size - rank) * recvcount * r_extent),
                             rank * recvcount, recvtype, sched, n_invtcs, invtcs);
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Step 1 data rotation scheduled\n"));

    MPIR_TSP_sched_fence(sched);

    /* Step 2: Allocate buffer space for packing/receving data for every phase */
    delta = 1;
    MPIR_CHKLMEM_MALLOC(tmp_sbuf, void ***, sizeof(void **) * nphases, mpi_errno, "tmp_sbuf",
                        MPL_MEM_COLL);
    MPIR_CHKLMEM_MALLOC(tmp_rbuf, void ***, sizeof(void **) * nphases, mpi_errno, "tmp_rbuf",
                        MPL_MEM_COLL);

    for (i = 0; i < nphases; i++) {
        tmp_sbuf[i] = (void **) MPL_malloc(sizeof(void *) * (k - 1), MPL_MEM_COLL);
        MPIR_Assert(tmp_sbuf[i] != NULL);
        tmp_rbuf[i] = (void **) MPL_malloc(sizeof(void *) * (k - 1), MPL_MEM_COLL);
        MPIR_Assert(tmp_rbuf[i] != NULL);
        for (j = 0; j < k - 1; j++) {
            if (i == 0 || buffer_per_phase == 1) {      /* allocate new memory if buffer_per_phase is set to true */
                tmp_sbuf[i][j] =
                    (void *) MPIR_TSP_sched_malloc((int) r_extent * recvcount * p_of_k, sched);
                MPIR_Assert(tmp_sbuf[i][j] != NULL);
                tmp_rbuf[i][j] =
                    (void *) MPIR_TSP_sched_malloc((int) r_extent * recvcount * p_of_k, sched);
                MPIR_Assert(tmp_rbuf[i][j] != NULL);
            } else {    /* reuse memory from first phase, make sure task dependencies are set correctly */
                tmp_sbuf[i][j] = tmp_sbuf[0][j];
                tmp_rbuf[i][j] = tmp_rbuf[0][j];
            }
        }
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Allocated temporary buffer space for packing\n"));

    /* use invtcs in the following manner
     * 0..k-2 for pack ids
     * k-1..2k-3 for send ids
     * 2k-2..3k-4 for recv ids
     * 3k-3..4k-5 for unpack ids
     */
    packids = invtcs;
    sendids = invtcs + k - 1;
    recvids = invtcs + 2 * k - 2;
    unpackids = invtcs + 3 * k - 3;
    pack_ninvtcs = recv_ninvtcs = 0;

    packsize = 0;
    num_unpacks_in_last_phase = 0;      /* record number of unpacking tasks in last phase for building dependency graph */

    for (i = 0; i < nphases; i++) {
        num_unpacks_in_last_phase = 0;
        for (j = 1; j < k; j++) {       /* for every non-zero value of digitval */
            if (MPL_ipow(k, i) * j >= size)     /* if the first location exceeds comm size, nothing is to be sent */
                break;

            src = (rank - delta * j + size) % size;
            dst = (rank + delta * j) % size;

            if (i != 0 && buffer_per_phase == 0) {      /* this dependency holds only when we don't have dedicated send buffer per phase */
                pack_invtcs[k - 1] = sendids[j - 1];
                pack_ninvtcs = k;
            }
            packids[j - 1] =
                brucks_sched_pup(1, recvbuf, tmp_sbuf[i][j - 1],
                                 recvtype, recvcount, i, k, j, size,
                                 &packsize, sched, pack_ninvtcs, pack_invtcs);
            *unpack_invtcs = packids[j - 1];
            unpack_ninvtcs = 1;
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "phase %d, digit %d packing scheduled\n", i, j));

            sendids[j - 1] =
                MPIR_TSP_sched_isend(tmp_sbuf[i][j - 1], packsize, MPI_BYTE, dst, tag,
                                     comm, sched, 1, &packids[j - 1]);
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "phase %d, digit %d send scheduled\n", i, j));

            if (i != 0 && buffer_per_phase == 0) {      /* this dependency holds only when we don't have dedicated recv buffer per phase */
                *recv_invtcs = unpackids[j - 1];
                recv_ninvtcs = 1;
            }
            recvids[j - 1] =
                MPIR_TSP_sched_irecv(tmp_rbuf[i][j - 1], packsize, MPI_BYTE,
                                     src, tag, comm, sched, recv_ninvtcs, recv_invtcs);
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "phase %d, digit %d recv scheduled\n", i, j));

            *(unpack_invtcs + 1) = recvids[j - 1];
            unpack_ninvtcs = 2;
            unpackids[j - 1] =
                brucks_sched_pup(0, recvbuf, tmp_rbuf[i][j - 1], recvtype,
                                 recvcount, i, k, j, size,
                                 &packsize, sched, unpack_ninvtcs, unpack_invtcs);
            num_unpacks_in_last_phase++;
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "phase %d, digit %d unpacking scheduled\n", i, j));
        }
        MPIR_Localcopy(unpackids, sizeof(int) * (k - 1), MPI_BYTE,
                       pack_invtcs, sizeof(int) * (k - 1), MPI_BYTE);
        pack_ninvtcs = k - 1;

        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "phase %d scheduled\n", i));

        delta *= k;
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Step 2 scheduled\n"));
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Step 2: num_unpacks_in_last_phase: %d\n",
                     num_unpacks_in_last_phase));

    /* Step 3: rotate the buffer */
    /* TODO: MPICH implementation does some lower_bound adjustment
     * here for derived datatypes, I am skipping that for now,
     * will come back to it later on - will require adding API
     * for getting true_lb */
    invtcs[0] =
        MPIR_TSP_sched_localcopy((void *) ((char *) recvbuf + (rank + 1) * recvcount * r_extent),
                                 (size - rank - 1) * recvcount, recvtype, tmp_buf,
                                 (size - rank - 1) * recvcount, recvtype, sched,
                                 num_unpacks_in_last_phase, unpackids);
    invtcs[1] =
        MPIR_TSP_sched_localcopy(recvbuf, (rank + 1) * recvcount, recvtype,
                                 (void *) ((char *) tmp_buf +
                                           (size - rank - 1) * recvcount * r_extent),
                                 (rank + 1) * recvcount, recvtype, sched, num_unpacks_in_last_phase,
                                 unpackids);

    /* invert the buffer now to get the result in desired order */
    for (i = 0; i < size; i++)
        MPIR_TSP_sched_localcopy((char *) tmp_buf + i * recvcount * r_extent, recvcount, recvtype,
                                 (void *) ((char *) recvbuf +
                                           (size - i - 1) * recvcount * r_extent), recvcount,
                                 recvtype, sched, 2, invtcs);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Step 3: data rearrangement scheduled\n"));

  fn_exit:
    for (i = 0; i < nphases; i++) {
        MPL_free(tmp_sbuf[i]);
        MPL_free(tmp_rbuf[i]);
    }
    MPIR_CHKLMEM_FREEALL();

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLTOALL_SCHED_INTRA_BRUCKS);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Non-blocking brucks based Alltoall */
int MPIR_TSP_Ialltoall_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                    MPIR_Comm * comm_ptr, MPIR_Request ** req, int k,
                                    int buffer_per_phase)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLTOALL_INTRA_BRUCKS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLTOALL_INTRA_BRUCKS);

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_Assert(sched != NULL);
    MPIR_TSP_sched_create(sched, false);

    /* schedule pipelined tree algo */
    mpi_errno = MPIR_TSP_Ialltoall_sched_intra_brucks(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcount, recvtype, comm_ptr, sched,
                                                      k, buffer_per_phase);
    MPIR_ERR_CHECK(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm_ptr, req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLTOALL_INTRA_BRUCKS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
