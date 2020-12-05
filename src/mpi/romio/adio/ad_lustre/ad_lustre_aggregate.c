/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *   Copyright (C) 1997 University of Chicago.
 *   See COPYRIGHT notice in top-level directory.
 *
 *   Copyright (C) 2007 Oak Ridge National Laboratory
 *
 *   Copyright (C) 2008 Sun Microsystems, Lustre group
 */

#include "ad_lustre.h"
#include "adio_extern.h"

#undef AGG_DEBUG

void ADIOI_LUSTRE_Get_striping_info(ADIO_File fd, int *striping_info, int mode)
{
    /* get striping information:
     *  striping_info[0]: stripe_size
     *  striping_info[1]: stripe_count
     *  striping_info[2]: avail_cb_nodes
     */
    int stripe_size, stripe_count, CO = 1;
    int avail_cb_nodes, divisor, nprocs_for_coll = fd->hints->cb_nodes;

    /* Get hints value */
    /* stripe size */
    stripe_size = fd->hints->striping_unit;
    /* stripe count */
    /* stripe_size and stripe_count have been validated in ADIOI_LUSTRE_Open() */
    stripe_count = fd->hints->striping_factor;

    /* Calculate the available number of I/O clients */
    if (!mode) {
        /* for collective read,
         * if "CO" clients access the same OST simultaneously,
         * the OST disk seek time would be much. So, to avoid this,
         * it might be better if 1 client only accesses 1 OST.
         * So, we set CO = 1 to meet the above requirement.
         */
        CO = 1;
        /*XXX: maybe there are other better way for collective read */
    } else {
        /* CO also has been validated in ADIOI_LUSTRE_Open(), >0 */
        CO = fd->hints->fs_hints.lustre.co_ratio;
    }
    /* Calculate how many IO clients we need */
    /* Algorithm courtesy Pascal Deveze (pascal.deveze@bull.net) */
    /* To avoid extent lock conflicts,
     * avail_cb_nodes should either
     *  - be a multiple of stripe_count,
     *  - or divide stripe_count exactly
     * so that each OST is accessed by a maximum of CO constant clients. */
    if (nprocs_for_coll >= stripe_count)
        /* avail_cb_nodes should be a multiple of stripe_count and the number
         * of procs per OST should be limited to the minimum between
         * nprocs_for_coll/stripe_count and CO
         *
         * e.g. if stripe_count=20, nprocs_for_coll=42 and CO=3 then
         * avail_cb_nodes should be equal to 40 */
        avail_cb_nodes = stripe_count * MPL_MIN(nprocs_for_coll / stripe_count, CO);
    else {
        /* nprocs_for_coll is less than stripe_count */
        /* avail_cb_nodes should divide stripe_count */
        /* e.g. if stripe_count=60 and nprocs_for_coll=8 then
         * avail_cb_nodes should be egal to 6 */
        /* This could be done with :
         * while (stripe_count % avail_cb_nodes != 0) avail_cb_nodes--;
         * but this can be optimized for large values of nprocs_for_coll and
         * stripe_count */
        divisor = 2;
        avail_cb_nodes = 1;
        /* try to divise */
        while (stripe_count >= divisor * divisor) {
            if ((stripe_count % divisor) == 0) {
                if (stripe_count / divisor <= nprocs_for_coll) {
                    /* The value is found ! */
                    avail_cb_nodes = stripe_count / divisor;
                    break;
                }
                /* if divisor is less than nprocs_for_coll, divisor is a
                 * solution, but it is not sure that it is the best one */
                else if (divisor <= nprocs_for_coll)
                    avail_cb_nodes = divisor;
            }
            divisor++;
        }
    }

    striping_info[0] = stripe_size;
    striping_info[1] = stripe_count;
    striping_info[2] = avail_cb_nodes;
}

int ADIOI_LUSTRE_Calc_aggregator(ADIO_File fd, ADIO_Offset off,
                                 ADIO_Offset * len, int *striping_info)
{
    int rank_index, rank;
    ADIO_Offset avail_bytes;
    int stripe_size = striping_info[0];
    int avail_cb_nodes = striping_info[2];

    /* Produce the stripe-contiguous pattern for Lustre */
    rank_index = (int) ((off / stripe_size) % avail_cb_nodes);

    /* we index into fd_end with rank_index, and fd_end was allocated to be no
     * bigger than fd->hins->cb_nodes.   If we ever violate that, we're
     * overrunning arrays.  Obviously, we should never ever hit this abort
     */
    if (rank_index >= fd->hints->cb_nodes)
        MPI_Abort(MPI_COMM_WORLD, 1);

    avail_bytes = (off / (ADIO_Offset) stripe_size + 1) * (ADIO_Offset) stripe_size - off;
    if (avail_bytes < *len) {
        /* this proc only has part of the requested contig. region */
        *len = avail_bytes;
    }
    /* map our index to a rank */
    /* NOTE: FOR NOW WE DON'T HAVE A MAPPING...JUST DO 0..NPROCS_FOR_COLL */
    rank = fd->hints->ranklist[rank_index];

    return rank;
}

/* ADIOI_LUSTRE_Calc_my_req() - calculate what portions of the access requests
 * of this process are located in the file domains of various processes
 * (including this one)
 */


void ADIOI_LUSTRE_Calc_my_req(ADIO_File fd, ADIO_Offset * offset_list,
                              ADIO_Offset * len_list, int contig_access_count,
                              int *striping_info, int nprocs,
                              int *count_my_req_procs_ptr,
                              int **count_my_req_per_proc_ptr,
                              ADIOI_Access ** my_req_ptr, ADIO_Offset *** buf_idx_ptr)
{
    /* Nothing different from ADIOI_Calc_my_req(), except calling
     * ADIOI_Lustre_Calc_aggregator() instead of the old one */
    int *count_my_req_per_proc, count_my_req_procs;
    int i, l, proc;
    size_t memLen;
    ADIO_Offset avail_len, rem_len, curr_idx, off, **buf_idx, *ptr;
    ADIOI_Access *my_req;
    int myrank;

    MPI_Comm_rank(fd->comm, &myrank);

    *count_my_req_per_proc_ptr = (int *) ADIOI_Calloc(nprocs, sizeof(int));
    count_my_req_per_proc = *count_my_req_per_proc_ptr;
    /* count_my_req_per_proc[i] gives the no. of contig. requests of this
     * process in process i's file domain. calloc initializes to zero.
     * I'm allocating memory of size nprocs, so that I can do an
     * MPI_Alltoall later on.
     */

    /* one pass just to calculate how much space to allocate for my_req;
     * contig_access_count was calculated way back in ADIOI_Calc_my_off_len()
     */
    for (i = 0; i < contig_access_count; i++) {
        /* short circuit offset/len processing if len == 0
         * (zero-byte  read/write
         */
        if (len_list[i] == 0)
            continue;
        off = offset_list[i];
        avail_len = len_list[i];
        /* note: we set avail_len to be the total size of the access.
         * then ADIOI_LUSTRE_Calc_aggregator() will modify the value to return
         * the amount that was available.
         */
        proc = ADIOI_LUSTRE_Calc_aggregator(fd, off, &avail_len, striping_info);
        count_my_req_per_proc[proc]++;

        /* figure out how many data is remaining in the access
         * we'll take care of this data (if there is any)
         * in the while loop below.
         */
        rem_len = len_list[i] - avail_len;

        while (rem_len != 0) {
            off += avail_len;   /* point to first remaining byte */
            avail_len = rem_len;        /* save remaining size, pass to calc */
            proc = ADIOI_LUSTRE_Calc_aggregator(fd, off, &avail_len, striping_info);
            count_my_req_per_proc[proc]++;
            rem_len -= avail_len;       /* reduce remaining length by amount from fd */
        }
    }

    /* buf_idx is relevant only if buftype_is_contig.
     * buf_idx[i] gives the index into user_buf where data received
     * from proc 'i' should be placed. This allows receives to be done
     * without extra buffer. This can't be done if buftype is not contig.
     */

    memLen = 0;
    for (i = 0; i < nprocs; i++)
        memLen += count_my_req_per_proc[i];
    ptr = (ADIO_Offset *) ADIOI_Malloc((memLen * 3 + nprocs) * sizeof(ADIO_Offset));

    /* initialize buf_idx vectors */
    buf_idx = (ADIO_Offset **) ADIOI_Malloc(nprocs * sizeof(ADIO_Offset *));
    buf_idx[0] = ptr;
    for (i = 1; i < nprocs; i++)
        buf_idx[i] = buf_idx[i - 1] + count_my_req_per_proc[i - 1] + 1;
    ptr += memLen + nprocs;     /* "+ nprocs" puts a terminal index at the end */

    /* now allocate space for my_req, offset, and len */
    *my_req_ptr = (ADIOI_Access *) ADIOI_Malloc(nprocs * sizeof(ADIOI_Access));
    my_req = *my_req_ptr;
    count_my_req_procs = 0;
#if 1==2
    for (i = 0; i < nprocs; i++) {
        if (count_my_req_per_proc[i]) {
            my_req[i].offsets = ptr;
            ptr += count_my_req_per_proc[i];
            my_req[i].lens = ptr;
            ptr += count_my_req_per_proc[i];
            count_my_req_procs++;
        }
        my_req[i].count = 0;    /* will be incremented where needed later */
    }
#endif
    fd->my_req_buf = (char*) ptr;
    for ( i = 0; i < nprocs; ++i ) {
        my_req[i].count = 0;
    }
    if (fd->is_agg) {
        for (i = 0; i < fd->hints->cb_nodes; i++) {
            if (fd->hints->ranklist[i] != myrank && count_my_req_per_proc[fd->hints->ranklist[i]]) {
                my_req[fd->hints->ranklist[i]].offsets = ptr;
                ptr += count_my_req_per_proc[fd->hints->ranklist[i]];
                my_req[fd->hints->ranklist[i]].lens = ptr;
                ptr += count_my_req_per_proc[fd->hints->ranklist[i]];
                count_my_req_procs++;
            }
        }
        if (count_my_req_per_proc[myrank]) {
            my_req[myrank].offsets = ptr;
            ptr += count_my_req_per_proc[myrank];
            my_req[myrank].lens = ptr;
            ptr += count_my_req_per_proc[myrank];
            count_my_req_procs++;
        }
    } else {
        for (i = 0; i < fd->hints->cb_nodes; i++) {
            if (count_my_req_per_proc[fd->hints->ranklist[i]]) {
                my_req[fd->hints->ranklist[i]].offsets = ptr;
                ptr += count_my_req_per_proc[fd->hints->ranklist[i]];
                my_req[fd->hints->ranklist[i]].lens = ptr;
                ptr += count_my_req_per_proc[fd->hints->ranklist[i]];
                count_my_req_procs++;
            }
        }
    }

    /* now fill in my_req */
    curr_idx = 0;
    for (i = 0; i < contig_access_count; i++) {
        /* short circuit offset/len processing if len == 0
         *      (zero-byte  read/write */
        if (len_list[i] == 0)
            continue;
        off = offset_list[i];
        avail_len = len_list[i];
        proc = ADIOI_LUSTRE_Calc_aggregator(fd, off, &avail_len, striping_info);

        l = my_req[proc].count;

        ADIOI_Assert(l < count_my_req_per_proc[proc]);
        buf_idx[proc][l] = curr_idx;
        curr_idx += avail_len;

        rem_len = len_list[i] - avail_len;

        /* store the proc, offset, and len information in an array
         * of structures, my_req. Each structure contains the
         * offsets and lengths located in that process's FD,
         * and the associated count.
         */
        my_req[proc].offsets[l] = off;
        ADIOI_Assert(avail_len == (int) avail_len);
        my_req[proc].lens[l] = (int) avail_len;
        my_req[proc].count++;

        while (rem_len != 0) {
            off += avail_len;
            avail_len = rem_len;
            proc = ADIOI_LUSTRE_Calc_aggregator(fd, off, &avail_len, striping_info);

            l = my_req[proc].count;
            ADIOI_Assert(l < count_my_req_per_proc[proc]);
            buf_idx[proc][l] = curr_idx;

            curr_idx += avail_len;
            rem_len -= avail_len;

            my_req[proc].offsets[l] = off;
            ADIOI_Assert(avail_len == (int) avail_len);
            my_req[proc].lens[l] = (int) avail_len;
            my_req[proc].count++;
        }
    }

#ifdef AGG_DEBUG
    for (i = 0; i < nprocs; i++) {
        if (count_my_req_per_proc[i] > 0) {
            FPRINTF(stdout, "data needed from %d (count = %d):\n", i, my_req[i].count);
            for (l = 0; l < my_req[i].count; l++) {
                FPRINTF(stdout, "   off[%d] = %lld, len[%d] = %d\n",
                        l, (long long) my_req[i].offsets[l], l, (long long) my_req[i].lens[l]);
            }
        }
    }
#endif

    *count_my_req_procs_ptr = count_my_req_procs;
    *buf_idx_ptr = buf_idx;
}

/**
 * This function performs the same objective as ADIOI_Calc_others_req, but we implement the communication using TAM.
 * Code is highly optimized to avoid any redundant MPI pt2pt calls.
 * This function is only compatible with ADIOI_LUSTRE_Calc_my_req at the moment, because offsets in my_req must be in a certain pre-defined order for optimization purpose.
*/
void ADIOI_LUSTRE_Calc_others_req(ADIO_File fd, int count_my_req_procs,
                           int *count_my_req_per_proc,
                           ADIOI_Access * my_req,
                           int nprocs, int myrank,
                           int *count_others_req_procs_ptr, ADIOI_Access ** others_req_ptr)
{
/* determine what requests of other processes lie in this process's
   file domain */

/* count_others_req_procs = number of processes whose requests lie in
   this process's file domain (including this process itself)
   count_others_req_per_proc[i] indicates how many separate contiguous
   requests of proc. i lie in this process's file domain. */

    int *count_others_req_per_proc, count_others_req_procs;
    int i, j, k, temp;
    MPI_Request *requests;
    ADIOI_Access *others_req;
    size_t memLen;
    ADIO_Offset *ptr;
    MPI_Aint *mem_ptrs;
    MPI_Request *req = fd->req;
    MPI_Status *sts = fd->sts;
    MPI_Aint local_data_size;
    size_t send_total_size;
    char* buf_ptr;

/* first find out how much to send/recv and from/to whom */
#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5026, 0, NULL);
#endif
    count_others_req_per_proc = (int *) ADIOI_Malloc(nprocs * sizeof(int));

    MPI_Alltoall(count_my_req_per_proc, 1, MPI_INT,
                 count_others_req_per_proc, 1, MPI_INT, fd->comm);

    *others_req_ptr = (ADIOI_Access *) ADIOI_Malloc(nprocs * sizeof(ADIOI_Access));
    others_req = *others_req_ptr;

    memLen = 0;
    for (i = 0; i < nprocs; i++)
        memLen += count_others_req_per_proc[i];
    ptr = (ADIO_Offset *) ADIOI_Malloc(memLen * 2 * sizeof(ADIO_Offset));
    mem_ptrs = (MPI_Aint *) ADIOI_Malloc(memLen * sizeof(MPI_Aint));
    count_others_req_procs = 0;
    /* Need to store the beginning of contiguous buf, because it is no longer neccessarily zero any more.
     * These two pointers are going to be freed at the end of collective write. */
    fd->other_req_buf = (char*) ptr;
    fd->other_req_mem = (char*) mem_ptrs;
    /* A special ordering of others request to fit into TAM context. */
    if (fd->is_agg) {
        for ( i = 0; i < fd->local_aggregator_size; ++i ) {
            /* Local aggregate messages are unpacked separately.
             * local_aggregator_domain record which processes a local aggregator represents for. */
            if (fd->local_aggregators[i] != myrank){
                for ( k = 0; k < fd->local_aggregator_domain_size[i]; ++k ) {
                    /* Local metadata has been unpacked at the beginning, nothing should be received from its local aggregator. */
                    if (fd->local_aggregator_domain[i][k] != myrank) {
                        if (count_others_req_per_proc[fd->local_aggregator_domain[i][k]]) {
                            others_req[fd->local_aggregator_domain[i][k]].count = count_others_req_per_proc[fd->local_aggregator_domain[i][k]];
                            others_req[fd->local_aggregator_domain[i][k]].offsets = ptr;
                            ptr += count_others_req_per_proc[fd->local_aggregator_domain[i][k]];
                            others_req[fd->local_aggregator_domain[i][k]].lens = ptr;
                            ptr += count_others_req_per_proc[fd->local_aggregator_domain[i][k]];
                            others_req[fd->local_aggregator_domain[i][k]].mem_ptrs = mem_ptrs;
                            mem_ptrs += count_others_req_per_proc[fd->local_aggregator_domain[i][k]];
                            count_others_req_procs++;
                        } else {
                            others_req[fd->local_aggregator_domain[i][k]].count = 0;
                        }
                    }
                }
            } else {
                temp = i;
            }
        }
        /* I am both global and local aggregators, so proxy messages from myself is placed after messages to be received by MPI_Irev*/
        if (fd->is_local_aggregator) {
            i = temp;
            for ( k = 0; k < fd->local_aggregator_domain_size[i]; ++k ) {
                /* Local metadata has been unpacked at the beginning, nothing should be received from its local aggregator. */
                if (fd->local_aggregator_domain[i][k] != myrank) {
                    if (count_others_req_per_proc[fd->local_aggregator_domain[i][k]]) {
                        others_req[fd->local_aggregator_domain[i][k]].count = count_others_req_per_proc[fd->local_aggregator_domain[i][k]];
                        others_req[fd->local_aggregator_domain[i][k]].offsets = ptr;
                        ptr += count_others_req_per_proc[fd->local_aggregator_domain[i][k]];
                        others_req[fd->local_aggregator_domain[i][k]].lens = ptr;
                        ptr += count_others_req_per_proc[fd->local_aggregator_domain[i][k]];
                        others_req[fd->local_aggregator_domain[i][k]].mem_ptrs = mem_ptrs;
                        mem_ptrs += count_others_req_per_proc[fd->local_aggregator_domain[i][k]];
                        count_others_req_procs++;
                    } else {
                        others_req[fd->local_aggregator_domain[i][k]].count = 0;
                    }
                }
            }
        }
        /*Self receiving offsets are placed in the end, so remote receive from local aggregators is a contiguous buffer. */
        if (count_others_req_per_proc[myrank]) {
            others_req[myrank].count = count_others_req_per_proc[myrank];
            others_req[myrank].offsets = ptr;
            ptr += count_others_req_per_proc[myrank];
            others_req[myrank].lens = ptr;
            ptr += count_others_req_per_proc[myrank];
            others_req[myrank].mem_ptrs = mem_ptrs;
            mem_ptrs += count_others_req_per_proc[myrank];
            count_others_req_procs++;
        } else {
            others_req[myrank].count = 0;
        }
    } else {
        /* non-aggregators do not receive anything. */
        for (i = 0; i < nprocs; i++) {
            others_req[i].count = 0;
        }
    }

    ADIOI_Free(count_others_req_per_proc);
    /* 0. Let local aggregators know metadata size first. */
    send_total_size = 0;
    for (i = 0; i < fd->hints->cb_nodes; ++i) {
        /* Skip self-send. */
        if (fd->hints->ranklist[i] != myrank) {
            send_total_size += 2 * my_req[fd->hints->ranklist[i]].count * sizeof(ADIO_Offset);
            fd->cb_send_size[i] = 2 * my_req[fd->hints->ranklist[i]].count * sizeof(ADIO_Offset);
        } else {
            fd->cb_send_size[i] = 0;
        }
    }
    /* Unpack local request here.*/
    if (my_req[myrank].count) {
        memcpy(others_req[myrank].offsets, my_req[myrank].offsets, 2 * my_req[myrank].count * sizeof(ADIO_Offset));
    }

    /* 1. Local aggregators receive the message size from non-local aggregators */
    j = 0;
    if (fd->is_local_aggregator) {
        /* Array for local message size of size fd->nprocs_aggregator * cb_nodes */
        for ( i = 0; i < fd->nprocs_aggregator; ++i ) {
            if (myrank != fd->aggregator_local_ranks[i]) {
                MPI_Irecv(fd->local_send_size + fd->hints->cb_nodes * i, fd->hints->cb_nodes, MPI_INT, fd->aggregator_local_ranks[i], fd->aggregator_local_ranks[i] + myrank, fd->comm, &req[j++]);
            } else {
                memcpy(fd->local_send_size + fd->hints->cb_nodes * i, fd->cb_send_size, sizeof(int) * fd->hints->cb_nodes);
            }
        }
    }
    if (myrank != fd->my_local_aggregator) {
        MPI_Issend(fd->cb_send_size, fd->hints->cb_nodes, MPI_INT, fd->my_local_aggregator, myrank + fd->my_local_aggregator, fd->comm, &req[j++]);
    }
    if (j) {
#ifdef MPI_STATUSES_IGNORE
        MPI_Waitall(j, req, MPI_STATUSES_IGNORE);
#else
        MPI_Waitall(j, req, sts);
#endif
    }

    /* 2. Intra-node aggregator of data from nonaggregators to local aggregators */
    j = 0;
    if (fd->is_local_aggregator) {
        /* We figure out the total data size a local aggregator is going to receive */
        local_data_size = 0;
        for ( i = 0; i < fd->hints->cb_nodes * fd->nprocs_aggregator; ++i ){
            local_data_size += (MPI_Aint) fd->local_send_size[i];
            /* local_lens is converted into inclusive-prefix sum because prefix-sum allows us to compute the sum within a interval without extra looping.
             * to avoid very large message size, we use MPI_Aint */
            fd->local_lens[i] = local_data_size;
        }

        /* Update memory size when necessary, can also be done with realloc */

        if ( fd->local_buf_size < local_data_size ){
            if (fd->local_buf_size) {
                ADIOI_Free(fd->local_buf);
            }
            fd->local_buf = (char *) ADIOI_Malloc(local_data_size * sizeof(char));
            fd->local_buf_size = local_data_size;
        }

        /* Local aggregators gather data from non-local aggregators */
        /* First local process as a special case*/
        if (fd->local_lens[fd->hints->cb_nodes - 1]) {
            if ( fd->aggregator_local_ranks[0] != myrank) {
                MPI_Irecv(fd->local_buf, fd->local_lens[fd->hints->cb_nodes - 1], MPI_BYTE, fd->aggregator_local_ranks[0], fd->aggregator_local_ranks[0] + myrank, fd->comm, &req[j++]);
            } else {
                //memcpy(fd->local_buf, my_req[0].offsets, send_total_size * sizeof(char));
                memcpy(fd->local_buf, fd->my_req_buf, send_total_size * sizeof(char));
            }
        }
        /* The rest of local processes*/
        for ( i = 1; i < fd->nprocs_aggregator; ++i ) {
            if (fd->local_lens[(i+1)*fd->hints->cb_nodes-1] == fd->local_lens[i*fd->hints->cb_nodes-1]) {
                /* No data for this local aggregator from this local process, just jump to the next one. */
                continue;
            }
            /* Shift the buffer with prefix-sum and do receive*/
            if ( fd->aggregator_local_ranks[i] != myrank ) {
                MPI_Irecv(fd->local_buf + fd->local_lens[i * fd->hints->cb_nodes - 1], fd->local_lens[(i+1)*fd->hints->cb_nodes-1] - fd->local_lens[i*fd->hints->cb_nodes-1], MPI_BYTE, fd->aggregator_local_ranks[i], fd->aggregator_local_ranks[i] + myrank, fd->comm, &req[j++]);
            } else {
                //memcpy(fd->local_buf + fd->local_lens[i * fd->hints->cb_nodes - 1], my_req[0].offsets, send_total_size * sizeof(char));
                memcpy(fd->local_buf + fd->local_lens[i * fd->hints->cb_nodes - 1], fd->my_req_buf, send_total_size * sizeof(char));
            }
        }
    }
    /* Send a large contiguous array of all data at local process to my local aggregator. */
    if (send_total_size && myrank != fd->my_local_aggregator) {
        MPI_Issend(fd->my_req_buf, send_total_size, MPI_BYTE, fd->my_local_aggregator, myrank + fd->my_local_aggregator, fd->comm, &req[j++]);
    }
    if (j) {
#ifdef MPI_STATUSES_IGNORE
        MPI_Waitall(j, req, MPI_STATUSES_IGNORE);
#else
        MPI_Waitall(j, req, sts);
#endif
    }

    /* 3. Inter-node aggregation phase of data from local aggregators to global aggregators.
     * Global aggregators know the data size from all processes in recv_size, so there is no need to exchange data size, this can boost performance. */
    j = 0;
    if (fd->is_agg) {
        /* global_recv_size is an array that indicate the sum of data size to be received from local aggregators. */
        for ( i = 0; i < fd->local_aggregator_size; ++i ) {
            /* We need to count data size from local aggregators 
             * global_recv_size is an array of local aggregator size. */
            fd->global_recv_size[i] = 0;
            for ( k = 0; k < fd->local_aggregator_domain_size[i]; ++k ) {
                /* Remeber a global aggregator does not receive anything from itself (so does its domain handled by any local aggregators)*/
                if (myrank != fd->local_aggregator_domain[i][k]) {
                    fd->global_recv_size[i] += 2 * others_req[fd->local_aggregator_domain[i][k]].count * sizeof(ADIO_Offset);
                }
            }
        }
        /* Now we can do the Irecv post from local aggregators to global aggregators
         * receive messages into meta data buffer. 
         * offset/length in other_req is pre-ordered, so no memory copy is needed. */
        buf_ptr = (char*) fd->other_req_buf;
        for ( i = 0; i < fd->local_aggregator_size; ++i ) {
            if (fd->local_aggregators[i] != myrank && fd->global_recv_size[i]) {
                MPI_Irecv(buf_ptr, fd->global_recv_size[i], MPI_BYTE, fd->local_aggregators[i], fd->local_aggregators[i] + myrank, fd->comm, &req[j++]);
                buf_ptr += fd->global_recv_size[i];
            }
        }
    }
    /* Local aggregators post send requests to global aggregators. 
     * We use derived datatype to wrap the buffer */
    if (fd->is_local_aggregator) {
        for ( i = 0; i < fd->hints->cb_nodes; ++i ) {
            fd->new_types[i] = MPI_BYTE;
            /* Do not do self-send */
            if (fd->hints->ranklist[i] != myrank) {
                local_data_size = 0;
                /* Interleave through local buffer to wrap messages to the same destination with derived dataset. */
                for ( k = 0; k < fd->nprocs_aggregator; ++k ) {
                    if (k * fd->hints->cb_nodes + i) {
                        fd->array_of_blocklengths[k] = fd->local_lens[k * fd->hints->cb_nodes + i] - fd->local_lens[k * fd->hints->cb_nodes + i - 1];
                        MPI_Address(fd->local_buf + fd->local_lens[k * fd->hints->cb_nodes + i - 1], fd->array_of_displacements + k);
                    } else {
                        fd->array_of_blocklengths[0] = fd->local_lens[0];
                        MPI_Address(fd->local_buf, fd->array_of_displacements);
                    }
                    local_data_size += fd->array_of_blocklengths[k];
                }
                /* Send derived datatype if it is not zero-sized. */
                if (local_data_size) {
                    MPI_Type_create_hindexed(fd->nprocs_aggregator, fd->array_of_blocklengths, fd->array_of_displacements, MPI_BYTE, fd->new_types + i);
                    MPI_Type_commit(fd->new_types + i);
                    MPI_Issend(MPI_BOTTOM, 1, fd->new_types[i], fd->hints->ranklist[i], myrank + fd->hints->ranklist[i], fd->comm, &req[j++]);
                }
            } else {
                /* A global aggregator that is also a local aggregator directly unpacks the buffer here. */
                for ( k = 0; k < fd->nprocs_aggregator; ++k ) {
                    /* Need to be careful about the lower bound of prefix sum, avoid negative index 
                     * Also need to avoid unpack my own message, because we have done it before and it is not in fd->local_buf. */
                    if (fd->aggregator_local_ranks[k] == myrank) {
                        continue;
                    }
                    if ( k * fd->hints->cb_nodes + i ) {
                        memcpy(others_req[fd->aggregator_local_ranks[k]].offsets, fd->local_buf + fd->local_lens[k * fd->hints->cb_nodes + i - 1], sizeof(char) * (fd->local_lens[k * fd->hints->cb_nodes + i] - fd->local_lens[k * fd->hints->cb_nodes + i - 1]));
                    } else {
                        memcpy(others_req[fd->aggregator_local_ranks[0]].offsets, fd->local_buf, sizeof(char) * fd->local_lens[0]);
                    }
                }
            }
        }
    }
    if (j) {
#ifdef MPI_STATUSES_IGNORE
        MPI_Waitall(j, req, MPI_STATUSES_IGNORE);
#else
        MPI_Waitall(j, req, sts);
#endif
    }

    /* local aggregators free derived datatypes */
    if ( fd->is_local_aggregator ){
        for ( i = 0; i < fd->hints->cb_nodes; ++i){
            /* A simple check for if we have actually created the type */
            if (fd->new_types[i] != MPI_BYTE) {
                MPI_Type_free(fd->new_types + i);
            }
        }
    }
    *count_others_req_procs_ptr = count_others_req_procs;
}

int ADIOI_LUSTRE_Docollect(ADIO_File fd, int contig_access_count,
                           ADIO_Offset * len_list, int nprocs)
{
    /* If the processes are non-interleaved, we will check the req_size.
     *   if (avg_req_size > big_req_size) {
     *       docollect = 0;
     *   }
     */

    int i, docollect = 1, big_req_size = 0;
    ADIO_Offset req_size = 0, total_req_size;
    int avg_req_size, total_access_count;

    /* calculate total_req_size and total_access_count */
    for (i = 0; i < contig_access_count; i++)
        req_size += len_list[i];
    MPI_Allreduce(&req_size, &total_req_size, 1, MPI_LONG_LONG_INT, MPI_SUM, fd->comm);
    MPI_Allreduce(&contig_access_count, &total_access_count, 1, MPI_INT, MPI_SUM, fd->comm);
    /* avoid possible divide-by-zero) */
    if (total_access_count != 0) {
        /* estimate average req_size */
        avg_req_size = (int) (total_req_size / total_access_count);
    } else {
        avg_req_size = 0;
    }
    /* get hint of big_req_size */
    big_req_size = fd->hints->fs_hints.lustre.coll_threshold;
    /* Don't perform collective I/O if there are big requests */
    if ((big_req_size > 0) && (avg_req_size > big_req_size))
        docollect = 0;

    return docollect;
}
