/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *   Copyright (C) 1997-2001 University of Chicago.
 *   See COPYRIGHT notice in top-level directory.
 */

#include "adio.h"
#include "adio_extern.h"

#ifdef AGGREGATION_PROFILE
#include "mpe.h"
#endif

#undef AGG_DEBUG

/* This file contains four functions:
 *
 * ADIOI_Calc_aggregator()
 * ADIOI_Calc_file_domains()
 * ADIOI_Calc_my_req()
 * ADIOI_Calc_others_req()
 *
 * The last three of these were originally in ad_read_coll.c, but they are
 * also shared with ad_write_coll.c.  I felt that they were better kept with
 * the rest of the shared aggregation code.
 */

/* Discussion of values available from above:
 *
 * ADIO_Offset st_offsets[0..nprocs-1]
 * ADIO_Offset end_offsets[0..nprocs-1]
 *    These contain a list of start and end offsets for each process in
 *    the communicator.  For example, an access at loc 10, size 10 would
 *    have a start offset of 10 and end offset of 19.
 * int nprocs
 *    number of processors in the collective I/O communicator
 * ADIO_Offset min_st_offset
 * ADIO_Offset fd_start[0..nprocs_for_coll-1]
 *    starting location of "file domain"; region that a given process will
 *    perform aggregation for (i.e. actually do I/O)
 * ADIO_Offset fd_end[0..nprocs_for_coll-1]
 *    start + size - 1 roughly, but it can be less, or 0, in the case of
 *    uneven distributions
 */

/* ADIOI_Calc_aggregator()
 *
 * The intention here is to implement a function which provides basically
 * the same functionality as in Rajeev's original version of
 * ADIOI_Calc_my_req().  He used a ceiling division approach to assign the
 * file domains, and we use the same approach here when calculating the
 * location of an offset/len in a specific file domain.  Further we assume
 * this same distribution when calculating the rank_index, which is later
 *  used to map to a specific process rank in charge of the file domain.
 *
 * A better (i.e. more general) approach would be to use the list of file
 * domains only.  This would be slower in the case where the
 * original ceiling division was used, but it would allow for arbitrary
 * distributions of regions to aggregators.  We'd need to know the
 * nprocs_for_coll in that case though, which we don't have now.
 *
 * Note a significant difference between this function and Rajeev's old code:
 * this code doesn't necessarily return a rank in the range
 * 0..nprocs_for_coll; instead you get something in 0..nprocs.  This is a
 * result of the rank mapping; any set of ranks in the communicator could be
 * used now.
 *
 * Returns an integer representing a rank in the collective I/O communicator.
 *
 * The "len" parameter is also modified to indicate the amount of data
 * actually available in this file domain.
 */
int ADIOI_Calc_aggregator(ADIO_File fd,
                          ADIO_Offset off,
                          ADIO_Offset min_off,
                          ADIO_Offset * len,
                          ADIO_Offset fd_size, ADIO_Offset * fd_start, ADIO_Offset * fd_end)
{
    int rank_index, rank;
    ADIO_Offset avail_bytes;

    MPL_UNREFERENCED_ARG(fd_start);

    /* get an index into our array of aggregators */
    rank_index = (int) ((off - min_off + fd_size) / fd_size - 1);

    if (fd->hints->striping_unit > 0) {
        /* wkliao: implementation for file domain alignment
         * fd_start[] and fd_end[] have been aligned with file lock
         * boundaries when returned from ADIOI_Calc_file_domains() so cannot
         * just use simple arithmatic as above */
        rank_index = 0;
        while (off > fd_end[rank_index])
            rank_index++;
    }

    /* we index into fd_end with rank_index, and fd_end was allocated to be no
     * bigger than fd->hins->cb_nodes.   If we ever violate that, we're
     * overrunning arrays.  Obviously, we should never ever hit this abort */
    if (rank_index >= fd->hints->cb_nodes || rank_index < 0) {
        FPRINTF(stderr,
                "Error in ADIOI_Calc_aggregator(): rank_index(%d) >= fd->hints->cb_nodes (%d) fd_size=%lld off=%lld\n",
                rank_index, fd->hints->cb_nodes, (long long) fd_size, (long long) off);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* remember here that even in Rajeev's original code it was the case that
     * different aggregators could end up with different amounts of data to
     * aggregate.  here we use fd_end[] to make sure that we know how much
     * data this aggregator is working with.
     *
     * the +1 is to take into account the end vs. length issue.
     */
    avail_bytes = fd_end[rank_index] + 1 - off;
    if (avail_bytes < *len) {
        /* this file domain only has part of the requested contig. region */
        *len = avail_bytes;
    }

    /* map our index to a rank */
    /* NOTE: FOR NOW WE DON'T HAVE A MAPPING...JUST DO 0..NPROCS_FOR_COLL */
    rank = fd->hints->ranklist[rank_index];

    return rank;
}

void ADIOI_Calc_file_domains(ADIO_Offset * st_offsets, ADIO_Offset
                             * end_offsets, int nprocs, int nprocs_for_coll,
                             ADIO_Offset * min_st_offset_ptr,
                             ADIO_Offset ** fd_start_ptr, ADIO_Offset
                             ** fd_end_ptr, int min_fd_size,
                             ADIO_Offset * fd_size_ptr, int striping_unit)
{
/* Divide the I/O workload among "nprocs_for_coll" processes. This is
   done by (logically) dividing the file into file domains (FDs); each
   process may directly access only its own file domain. */

    ADIO_Offset min_st_offset, max_end_offset, *fd_start, *fd_end, fd_size;
    int i;

#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5004, 0, NULL);
#endif

#ifdef AGG_DEBUG
    FPRINTF(stderr, "ADIOI_Calc_file_domains: %d aggregator(s)\n", nprocs_for_coll);
#endif

/* find min of start offsets and max of end offsets of all processes */

    min_st_offset = st_offsets[0];
    max_end_offset = end_offsets[0];

    for (i = 1; i < nprocs; i++) {
        min_st_offset = MPL_MIN(min_st_offset, st_offsets[i]);
        max_end_offset = MPL_MAX(max_end_offset, end_offsets[i]);
    }

/* determine the "file domain (FD)" of each process, i.e., the portion of
   the file that will be "owned" by each process */

/* partition the total file access range equally among nprocs_for_coll
   processes */
    fd_size = ((max_end_offset - min_st_offset + 1) + nprocs_for_coll - 1) / nprocs_for_coll;
    /* ceiling division as in HPF block distribution */

    /* Tweak the file domains so that no fd is smaller than a threshold.  We
     * have to strike a balance between efficency and parallelism: somewhere
     * between 10k processes sending 32-byte requests and one process sending a
     * 320k request is a (system-dependent) sweet spot */

    if (fd_size < min_fd_size)
        fd_size = min_fd_size;

    *fd_start_ptr = (ADIO_Offset *) ADIOI_Malloc(nprocs_for_coll * 2 * sizeof(ADIO_Offset));
    *fd_end_ptr = *fd_start_ptr + nprocs_for_coll;

    fd_start = *fd_start_ptr;
    fd_end = *fd_end_ptr;

    /* Wei-keng Liao: implementation for fild domain alignment to nearest file
     * lock boundary (as specified by striping_unit hint).  Could also
     * experiment with other alignment strategies here */
    if (striping_unit > 0) {
        ADIO_Offset end_off;
        int rem_front, rem_back;

        /* align fd_end[0] to the nearest file lock boundary */
        fd_start[0] = min_st_offset;
        end_off = fd_start[0] + fd_size;
        rem_front = end_off % striping_unit;
        rem_back = striping_unit - rem_front;
        if (rem_front < rem_back)
            end_off -= rem_front;
        else
            end_off += rem_back;
        fd_end[0] = end_off - 1;

        /* align fd_end[i] to the nearest file lock boundary */
        for (i = 1; i < nprocs_for_coll; i++) {
            fd_start[i] = fd_end[i - 1] + 1;
            end_off = min_st_offset + fd_size * (i + 1);
            rem_front = end_off % striping_unit;
            rem_back = striping_unit - rem_front;
            if (rem_front < rem_back)
                end_off -= rem_front;
            else
                end_off += rem_back;
            fd_end[i] = end_off - 1;
        }
        fd_end[nprocs_for_coll - 1] = max_end_offset;
    } else {    /* no hints set: do things the 'old' way */
        fd_start[0] = min_st_offset;
        fd_end[0] = min_st_offset + fd_size - 1;

        for (i = 1; i < nprocs_for_coll; i++) {
            fd_start[i] = fd_end[i - 1] + 1;
            fd_end[i] = fd_start[i] + fd_size - 1;
        }
    }

/* take care of cases in which the total file access range is not
   divisible by the number of processes. In such cases, the last
   process, or the last few processes, may have unequal load (even 0).
   For example, a range of 97 divided among 16 processes.
   Note that the division is ceiling division. */

    for (i = 0; i < nprocs_for_coll; i++) {
        if (fd_start[i] > max_end_offset)
            fd_start[i] = fd_end[i] = -1;
        if (fd_end[i] > max_end_offset)
            fd_end[i] = max_end_offset;
    }

    *fd_size_ptr = fd_size;
    *min_st_offset_ptr = min_st_offset;

#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5005, 0, NULL);
#endif
}


/* ADIOI_Calc_my_req() - calculate what portions of the access requests
 * of this process are located in the file domains of various processes
 * (including this one)
 */
void ADIOI_Calc_my_req(ADIO_File fd, ADIO_Offset * offset_list, ADIO_Offset * len_list,
                       int contig_access_count, ADIO_Offset
                       min_st_offset, ADIO_Offset * fd_start,
                       ADIO_Offset * fd_end, ADIO_Offset fd_size,
                       int nprocs,
                       int *count_my_req_procs_ptr,
                       int **count_my_req_per_proc_ptr,
                       ADIOI_Access ** my_req_ptr, MPI_Aint ** buf_idx_ptr)
/* Possibly reconsider if buf_idx's are ok as int's, or should they be aints/offsets?
   They are used as memory buffer indices so it seems like the 2G limit is in effect */
{
    int *count_my_req_per_proc, count_my_req_procs;
    MPI_Aint *buf_idx;
    int i, l, proc;
    size_t memLen;
    ADIO_Offset fd_len, rem_len, curr_idx, off, *ptr;
    ADIOI_Access *my_req;

#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5024, 0, NULL);
#endif

    *count_my_req_per_proc_ptr = (int *) ADIOI_Calloc(nprocs, sizeof(int));
    count_my_req_per_proc = *count_my_req_per_proc_ptr;
/* count_my_req_per_proc[i] gives the no. of contig. requests of this
   process in process i's file domain. calloc initializes to zero.
   I'm allocating memory of size nprocs, so that I can do an
   MPI_Alltoall later on.*/

    buf_idx = (MPI_Aint *) ADIOI_Malloc(nprocs * sizeof(MPI_Aint));
/* buf_idx is relevant only if buftype_is_contig.
   buf_idx[i] gives the index into user_buf where data received
   from proc. i should be placed. This allows receives to be done
   without extra buffer. This can't be done if buftype is not contig. */

    /* initialize buf_idx to -1 */
    for (i = 0; i < nprocs; i++)
        buf_idx[i] = -1;

    /* one pass just to calculate how much space to allocate for my_req;
     * contig_access_count was calculated way back in ADIOI_Calc_my_off_len()
     */
    for (i = 0; i < contig_access_count; i++) {
        /* short circuit offset/len processing if len == 0
         *      (zero-byte  read/write */
        if (len_list[i] == 0)
            continue;
        off = offset_list[i];
        fd_len = len_list[i];
        /* note: we set fd_len to be the total size of the access.  then
         * ADIOI_Calc_aggregator() will modify the value to return the
         * amount that was available from the file domain that holds the
         * first part of the access.
         */
        proc = ADIOI_Calc_aggregator(fd, off, min_st_offset, &fd_len, fd_size, fd_start, fd_end);
        count_my_req_per_proc[proc]++;

        /* figure out how much data is remaining in the access (i.e. wasn't
         * part of the file domain that had the starting byte); we'll take
         * care of this data (if there is any) in the while loop below.
         */
        rem_len = len_list[i] - fd_len;

        while (rem_len != 0) {
            off += fd_len;      /* point to first remaining byte */
            fd_len = rem_len;   /* save remaining size, pass to calc */
            proc = ADIOI_Calc_aggregator(fd, off, min_st_offset, &fd_len,
                                         fd_size, fd_start, fd_end);

            count_my_req_per_proc[proc]++;
            rem_len -= fd_len;  /* reduce remaining length by amount from fd */
        }
    }

/* now allocate space for my_req, offset, and len */

    *my_req_ptr = (ADIOI_Access *) ADIOI_Malloc(nprocs * sizeof(ADIOI_Access));
    my_req = *my_req_ptr;

    /* combine offsets and lens into a single regions so we can make one
     * exchange instead of two later on.  Over-allocate the 'offsets' array and
     * make 'lens' point to the over-allocated part
     */
    memLen = 0;
    for (i = 0; i < nprocs; i++)
        memLen += count_my_req_per_proc[i];
    ptr = (ADIO_Offset *) ADIOI_Malloc(memLen * 2 * sizeof(ADIO_Offset));
    my_req[0].offsets = ptr;

    count_my_req_procs = 0;
    for (i = 0; i < nprocs; i++) {
        if (count_my_req_per_proc[i]) {
            my_req[i].offsets = ptr;
            ptr += count_my_req_per_proc[i];
            my_req[i].lens = ptr;
            ptr += count_my_req_per_proc[i];
            count_my_req_procs++;
        }
        my_req[i].count = 0;    /* will be incremented where needed
                                 * later */
    }

/* now fill in my_req */
    curr_idx = 0;
    for (i = 0; i < contig_access_count; i++) {
        /* short circuit offset/len processing if len == 0
         *      (zero-byte  read/write */
        if (len_list[i] == 0)
            continue;
        off = offset_list[i];
        fd_len = len_list[i];
        proc = ADIOI_Calc_aggregator(fd, off, min_st_offset, &fd_len, fd_size, fd_start, fd_end);

        /* for each separate contiguous access from this process */
        if (buf_idx[proc] == -1) {
            ADIOI_Assert(curr_idx == (MPI_Aint) curr_idx);
            buf_idx[proc] = (MPI_Aint) curr_idx;
        }

        l = my_req[proc].count;
        curr_idx += fd_len;

        rem_len = len_list[i] - fd_len;

        /* store the proc, offset, and len information in an array
         * of structures, my_req. Each structure contains the
         * offsets and lengths located in that process's FD,
         * and the associated count.
         */
        my_req[proc].offsets[l] = off;
        my_req[proc].lens[l] = fd_len;
        my_req[proc].count++;

        while (rem_len != 0) {
            off += fd_len;
            fd_len = rem_len;
            proc = ADIOI_Calc_aggregator(fd, off, min_st_offset, &fd_len,
                                         fd_size, fd_start, fd_end);

            if (buf_idx[proc] == -1) {
                ADIOI_Assert(curr_idx == (MPI_Aint) curr_idx);
                buf_idx[proc] = (MPI_Aint) curr_idx;
            }

            l = my_req[proc].count;
            curr_idx += fd_len;
            rem_len -= fd_len;

            my_req[proc].offsets[l] = off;
            my_req[proc].lens[l] = fd_len;
            my_req[proc].count++;
        }
    }

#ifdef AGG_DEBUG
    for (i = 0; i < nprocs; i++) {
        if (count_my_req_per_proc[i] > 0) {
            FPRINTF(stdout, "data needed from %d (count = %d):\n", i, my_req[i].count);
            for (l = 0; l < my_req[i].count; l++) {
                FPRINTF(stdout, "   off[%d] = %lld, len[%d] = %d\n", l,
                        (long long) my_req[i].offsets[l], l, (long long) my_req[i].lens[l]);
            }
            FPRINTF(stdout, "buf_idx[%d] = 0x%x\n", i, buf_idx[i]);
        }
    }
#endif

    *count_my_req_procs_ptr = count_my_req_procs;
    *buf_idx_ptr = buf_idx;
#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5025, 0, NULL);
#endif
}



void ADIOI_Calc_others_req(ADIO_File fd, int count_my_req_procs,
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
    int i, j;
    MPI_Request *requests;
    ADIOI_Access *others_req;
    size_t memLen;
    ADIO_Offset *ptr;
    MPI_Aint *mem_ptrs;

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
    others_req[0].offsets = ptr;
    others_req[0].mem_ptrs = mem_ptrs;

    count_others_req_procs = 0;
    for (i = 0; i < nprocs; i++) {
        if (count_others_req_per_proc[i]) {
            others_req[i].count = count_others_req_per_proc[i];
            others_req[i].offsets = ptr;
            ptr += count_others_req_per_proc[i];
            others_req[i].lens = ptr;
            ptr += count_others_req_per_proc[i];
            others_req[i].mem_ptrs = mem_ptrs;
            mem_ptrs += count_others_req_per_proc[i];
            count_others_req_procs++;
        } else
            others_req[i].count = 0;
    }
    ADIOI_Free(count_others_req_per_proc);

/* now send the calculated offsets and lengths to respective processes */

    requests = (MPI_Request *)
        ADIOI_Malloc(1 + (count_my_req_procs + count_others_req_procs) * sizeof(MPI_Request));
/* +1 to avoid a 0-size malloc */

    j = 0;
    for (i = 0; i < nprocs; i++) {
        if (others_req[i].count == 0)
            continue;
        if (i == myrank)
            /* send to self uses memcpy()C, here others_req[i].count == my_req[i].count */
            memcpy(others_req[i].offsets, my_req[i].offsets,
                   2 * my_req[i].count * sizeof(ADIO_Offset));
        else
            MPI_Irecv(others_req[i].offsets, 2 * others_req[i].count,
                      ADIO_OFFSET, i, i + myrank, fd->comm, &requests[j++]);
    }

    for (i = 0; i < nprocs; i++) {
        if (my_req[i].count && i != myrank) {
            MPI_Isend(my_req[i].offsets, 2 * my_req[i].count,
                      ADIO_OFFSET, i, i + myrank, fd->comm, &requests[j++]);
        }
    }

    if (j) {
#ifdef MPI_STATUSES_IGNORE
        MPI_Waitall(j, requests, MPI_STATUSES_IGNORE);
#else
        MPI_Status *statuses = (MPI_Status *) ADIOI_Malloc(j * sizeof(MPI_Status));
        MPI_Waitall(j, requests, statuses);
        ADIOI_Free(statuses);
#endif
    }

    ADIOI_Free(requests);

    *count_others_req_procs_ptr = count_others_req_procs;
#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5027, 0, NULL);
#endif
}

/* ADIOI_Calc_my_req() - calculate what portions of the access requests
 * of this process are located in the file domains of various processes
 * (including this one)
 */
void ADIOI_TAM_Calc_my_req(ADIO_File fd, ADIO_Offset * offset_list, ADIO_Offset * len_list,
                       int contig_access_count, ADIO_Offset
                       min_st_offset, ADIO_Offset * fd_start,
                       ADIO_Offset * fd_end, ADIO_Offset fd_size,
                       int nprocs,
                       int *count_my_req_procs_ptr,
                       int **count_my_req_per_proc_ptr,
                       ADIOI_Access ** my_req_ptr, MPI_Aint ** buf_idx_ptr)
/* Possibly reconsider if buf_idx's are ok as int's, or should they be aints/offsets?
   They are used as memory buffer indices so it seems like the 2G limit is in effect */
{
    int *count_my_req_per_proc, count_my_req_procs;
    MPI_Aint *buf_idx;
    int i, l, proc;
    size_t memLen;
    ADIO_Offset fd_len, rem_len, curr_idx, off, *ptr;
    ADIOI_Access *my_req;
    int myrank;

    MPI_Comm_rank(fd->comm, &myrank);

#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5024, 0, NULL);
#endif

    *count_my_req_per_proc_ptr = (int *) ADIOI_Calloc(nprocs, sizeof(int));
    count_my_req_per_proc = *count_my_req_per_proc_ptr;
/* count_my_req_per_proc[i] gives the no. of contig. requests of this
   process in process i's file domain. calloc initializes to zero.
   I'm allocating memory of size nprocs, so that I can do an
   MPI_Alltoall later on.*/

    buf_idx = (MPI_Aint *) ADIOI_Malloc(nprocs * sizeof(MPI_Aint));
/* buf_idx is relevant only if buftype_is_contig.
   buf_idx[i] gives the index into user_buf where data received
   from proc. i should be placed. This allows receives to be done
   without extra buffer. This can't be done if buftype is not contig. */

    /* initialize buf_idx to -1 */
    for (i = 0; i < nprocs; i++)
        buf_idx[i] = -1;

    /* one pass just to calculate how much space to allocate for my_req;
     * contig_access_count was calculated way back in ADIOI_Calc_my_off_len()
     */
    for (i = 0; i < contig_access_count; i++) {
        /* short circuit offset/len processing if len == 0
         *      (zero-byte  read/write */
        if (len_list[i] == 0)
            continue;
        off = offset_list[i];
        fd_len = len_list[i];
        /* note: we set fd_len to be the total size of the access.  then
         * ADIOI_Calc_aggregator() will modify the value to return the
         * amount that was available from the file domain that holds the
         * first part of the access.
         */
        proc = ADIOI_Calc_aggregator(fd, off, min_st_offset, &fd_len, fd_size, fd_start, fd_end);
        count_my_req_per_proc[proc]++;

        /* figure out how much data is remaining in the access (i.e. wasn't
         * part of the file domain that had the starting byte); we'll take
         * care of this data (if there is any) in the while loop below.
         */
        rem_len = len_list[i] - fd_len;

        while (rem_len != 0) {
            off += fd_len;      /* point to first remaining byte */
            fd_len = rem_len;   /* save remaining size, pass to calc */
            proc = ADIOI_Calc_aggregator(fd, off, min_st_offset, &fd_len,
                                         fd_size, fd_start, fd_end);

            count_my_req_per_proc[proc]++;
            rem_len -= fd_len;  /* reduce remaining length by amount from fd */
        }
    }

/* now allocate space for my_req, offset, and len */

    *my_req_ptr = (ADIOI_Access *) ADIOI_Malloc(nprocs * sizeof(ADIOI_Access));
    my_req = *my_req_ptr;

    /* combine offsets and lens into a single regions so we can make one
     * exchange instead of two later on.  Over-allocate the 'offsets' array and
     * make 'lens' point to the over-allocated part
     */
    memLen = 0;
    for (i = 0; i < nprocs; i++)
        memLen += count_my_req_per_proc[i];
    ptr = (ADIO_Offset *) ADIOI_Malloc((memLen * 2 + 1) * sizeof(ADIO_Offset));
    fd->my_req_buf = (char*) ptr;

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
        my_req[i].count = 0;    /* will be incremented where needed
                                 * later */
    }
#endif
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
        fd_len = len_list[i];
        proc = ADIOI_Calc_aggregator(fd, off, min_st_offset, &fd_len, fd_size, fd_start, fd_end);

        /* for each separate contiguous access from this process */
        if (buf_idx[proc] == -1) {
            ADIOI_Assert(curr_idx == (MPI_Aint) curr_idx);
            buf_idx[proc] = (MPI_Aint) curr_idx;
        }

        l = my_req[proc].count;
        curr_idx += fd_len;

        rem_len = len_list[i] - fd_len;

        /* store the proc, offset, and len information in an array
         * of structures, my_req. Each structure contains the
         * offsets and lengths located in that process's FD,
         * and the associated count.
         */
        my_req[proc].offsets[l] = off;
        my_req[proc].lens[l] = fd_len;
        my_req[proc].count++;

        while (rem_len != 0) {
            off += fd_len;
            fd_len = rem_len;
            proc = ADIOI_Calc_aggregator(fd, off, min_st_offset, &fd_len,
                                         fd_size, fd_start, fd_end);

            if (buf_idx[proc] == -1) {
                ADIOI_Assert(curr_idx == (MPI_Aint) curr_idx);
                buf_idx[proc] = (MPI_Aint) curr_idx;
            }

            l = my_req[proc].count;
            curr_idx += fd_len;
            rem_len -= fd_len;

            my_req[proc].offsets[l] = off;
            my_req[proc].lens[l] = fd_len;
            my_req[proc].count++;
        }
    }

#ifdef AGG_DEBUG
    for (i = 0; i < nprocs; i++) {
        if (count_my_req_per_proc[i] > 0) {
            FPRINTF(stdout, "data needed from %d (count = %d):\n", i, my_req[i].count);
            for (l = 0; l < my_req[i].count; l++) {
                FPRINTF(stdout, "   off[%d] = %lld, len[%d] = %d\n", l,
                        (long long) my_req[i].offsets[l], l, (long long) my_req[i].lens[l]);
            }
            FPRINTF(stdout, "buf_idx[%d] = 0x%x\n", i, buf_idx[i]);
        }
    }
#endif

    *count_my_req_procs_ptr = count_my_req_procs;
    *buf_idx_ptr = buf_idx;
#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5025, 0, NULL);
#endif
}


/**
 * This function performs the same objective as ADIOI_Calc_others_req, but we implement the communication using TAM.
 * Code is highly optimized to avoid any redundant MPI pt2pt calls.
 * This function is only compatible with ADIOI_Lustre_Calc_my_req and ADIOI_GPFS_TAM_Calc_my_req at the moment, because offsets in my_req must be in a certain pre-defined order for optimization purpose.
*/
void ADIOI_TAM_Calc_others_req(ADIO_File fd, int count_my_req_procs,
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
                        /* When we build stand-alone ROMIO library, MPI_Address could fail depending on the production MPI implementation, we use C casting to avoid this problem. */
                        //MPI_Address(fd->local_buf + fd->local_lens[k * fd->hints->cb_nodes + i - 1], fd->array_of_displacements + k);
                        fd->array_of_displacements[k] = (MPI_Aint) (fd->local_buf + fd->local_lens[k * fd->hints->cb_nodes + i - 1]);
                    } else {
                        fd->array_of_blocklengths[0] = fd->local_lens[0];
                        //MPI_Address(fd->local_buf, fd->array_of_displacements);
                        fd->array_of_displacements[0] = (MPI_Aint) fd->local_buf;
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
#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5027, 0, NULL);
#endif
}


/* Nonblocking version of ADIOI_Calc_others_req().
   It consists of three functions - ADIOI_Icalc_others_req(),
   ADIOI_Icalc_others_req_main(), and ADIOI_Icalc_others_req_fini(). */
void ADIOI_Icalc_others_req(ADIOI_NBC_Request * nbc_req, int *error_code)
{
    ADIOI_Icalc_others_req_vars *vars = nbc_req->cor_vars;

    /* count_others_req_per_proc[i] indicates how many separate contiguous
     * requests of proc. i lie in this process's file domain. */

    /* first find out how much to send/recv and from/to whom */
#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5026, 0, NULL);
#endif
    vars->count_others_req_per_proc = (int *) ADIOI_Malloc(vars->nprocs * sizeof(int));

    *error_code = MPI_Ialltoall(vars->count_my_req_per_proc, 1, MPI_INT,
                                vars->count_others_req_per_proc, 1, MPI_INT, vars->fd->comm,
                                &vars->req1);

    if (nbc_req->rdwr == ADIOI_READ) {
        nbc_req->data.rd.state = ADIOI_IRC_STATE_ICALC_OTHERS_REQ;
    } else {
        ADIOI_Assert(nbc_req->rdwr == ADIOI_WRITE);
        nbc_req->data.wr.state = ADIOI_IWC_STATE_ICALC_OTHERS_REQ;
    }
}

void ADIOI_Icalc_others_req_main(ADIOI_NBC_Request * nbc_req, int *error_code)
{
    ADIOI_Icalc_others_req_vars *vars = nbc_req->cor_vars;
    ADIO_File fd = vars->fd;
    int count_my_req_procs = vars->count_my_req_procs;
    ADIOI_Access *my_req = vars->my_req;
    int nprocs = vars->nprocs;
    int myrank = vars->myrank;
    ADIOI_Access **others_req_ptr = vars->others_req_ptr;

    /* determine what requests of other processes lie in this process's
     * file domain */

    /* count_others_req_procs = number of processes whose requests lie in
     * this process's file domain (including this process itself)
     * count_others_req_per_proc[i] indicates how many separate contiguous
     * requests of proc. i lie in this process's file domain. */

    int *count_others_req_per_proc = vars->count_others_req_per_proc;
    int count_others_req_procs;
    int i, j;
    ADIOI_Access *others_req;
    size_t memLen;
    ADIO_Offset *ptr;
    MPI_Aint *mem_ptrs;

    *others_req_ptr = (ADIOI_Access *) ADIOI_Malloc(nprocs * sizeof(ADIOI_Access));
    others_req = *others_req_ptr;

    memLen = 0;
    for (i = 0; i < nprocs; i++)
        memLen += count_others_req_per_proc[i];
    ptr = (ADIO_Offset *) ADIOI_Malloc(memLen * 2 * sizeof(ADIO_Offset));
    mem_ptrs = (MPI_Aint *) ADIOI_Malloc(memLen * sizeof(MPI_Aint));
    others_req[0].offsets = ptr;
    others_req[0].mem_ptrs = mem_ptrs;

    count_others_req_procs = 0;
    for (i = 0; i < nprocs; i++) {
        if (count_others_req_per_proc[i]) {
            others_req[i].count = count_others_req_per_proc[i];
            others_req[i].offsets = ptr;
            ptr += count_others_req_per_proc[i];
            others_req[i].lens = ptr;
            ptr += count_others_req_per_proc[i];
            others_req[i].mem_ptrs = mem_ptrs;
            mem_ptrs += count_others_req_per_proc[i];
            count_others_req_procs++;
        } else
            others_req[i].count = 0;
    }
    vars->count_others_req_procs = count_others_req_procs;

    /* now send the calculated offsets and lengths to respective processes */

    vars->req2 = (MPI_Request *)
        ADIOI_Malloc(1 + 2 * (count_my_req_procs + count_others_req_procs)
                     * sizeof(MPI_Request));
    /* +1 to avoid a 0-size malloc */

    j = 0;
    for (i = 0; i < nprocs; i++) {
        if (others_req[i].count) {
            MPI_Irecv(others_req[i].offsets, 2 * others_req[i].count,
                      ADIO_OFFSET, i, i + myrank, fd->comm, &vars->req2[j++]);
        }
    }

    for (i = 0; i < nprocs; i++) {
        if (my_req[i].count) {
            MPI_Isend(my_req[i].offsets, 2 * my_req[i].count,
                      ADIO_OFFSET, i, i + myrank, fd->comm, &vars->req2[j++]);
        }
    }

    /* keep the number of requests */
    vars->num_req2 = j;

    if (nbc_req->rdwr == ADIOI_READ) {
        nbc_req->data.rd.state = ADIOI_IRC_STATE_ICALC_OTHERS_REQ_MAIN;
    } else {
        ADIOI_Assert(nbc_req->rdwr == ADIOI_WRITE);
        nbc_req->data.wr.state = ADIOI_IWC_STATE_ICALC_OTHERS_REQ_MAIN;
    }
}



void ADIOI_Icalc_others_req_fini(ADIOI_NBC_Request * nbc_req, int *error_code)
{
    ADIOI_Icalc_others_req_vars *vars = nbc_req->cor_vars;
    void (*next_fn) (ADIOI_NBC_Request *, int *);

    ADIOI_Free(vars->req2);
    ADIOI_Free(vars->count_others_req_per_proc);

    *vars->count_others_req_procs_ptr = vars->count_others_req_procs;
#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5027, 0, NULL);
#endif
    /* end of the calculation */

    next_fn = vars->next_fn;

    /* free the struct for parameters and variables */
    ADIOI_Free(vars);
    nbc_req->cor_vars = NULL;

    /* move to the next function */
    next_fn(nbc_req, error_code);
}
