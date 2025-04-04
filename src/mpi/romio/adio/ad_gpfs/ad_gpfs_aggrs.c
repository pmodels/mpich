/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/**
 * \file ad_gpfs_aggrs.c
 * \brief The externally used function from this file is is declared in ad_gpfs_aggrs.h
 */

#include "adio.h"
#include "adio_cb_config_list.h"
#include "ad_gpfs.h"
#include "ad_gpfs_aggrs.h"

#ifdef AGGREGATION_PROFILE
#include "mpe.h"
#endif


#ifdef MPL_USE_DBG_LOGGING
#define AGG_DEBUG 1
#endif

#ifndef TRACE_ERR
#define TRACE_ERR(format...)
#endif


#ifndef HAS_ALLTOALLV_C
static int
MY_Alltoallv(void *sbuf, int *scounts, MPI_Aint * sdisps, MPI_Datatype stype,
             void *rbuf, int *rcounts, MPI_Aint * rdisps, MPI_Datatype rtype, MPI_Comm comm);
#endif

/* Comments copied from common:
 * This file contains four functions:
 *
 * ADIOI_GPFS_Calc_aggregator()
 * ADIOI_GPFS_Calc_file_domains()
 * ADIOI_GPFS_Calc_my_req()
 * ADIOI_GPFS_Free_my_req()
 * ADIOI_GPFS_Calc_others_req()
 * ADIOI_GPFS_Free_others_req()
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

/* Description from common/ad_aggregate.c.  (Does it completely apply to bg?)
 * ADIOI_Calc_aggregator()
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
/*
 * This is more general aggregator search function which does not base on the assumption
 * that each aggregator hosts the file domain with the same size
 */
int ADIOI_GPFS_Calc_aggregator(ADIO_File fd,
                               ADIO_Offset off,
                               ADIO_Offset min_off,
                               ADIO_Offset * len,
                               ADIO_Offset fd_size, ADIO_Offset * fd_start, ADIO_Offset * fd_end)
{
    int rank_index, rank;
    ADIO_Offset avail_bytes;
    TRACE_ERR("Entering ADIOI_GPFS_Calc_aggregator\n");

    ADIOI_Assert((off <= fd_end[fd->hints->cb_nodes - 1] && off >= min_off &&
                  fd_start[0] >= min_off));

    /* binary search --> rank_index is returned */
    int ub = fd->hints->cb_nodes;
    int lb = 0;
    /* get an index into our array of aggregators */
    /* Common code for striping - bg doesn't use it but it's
     * here to make diff'ing easier.
     * rank_index = (int) ((off - min_off + fd_size)/ fd_size - 1);
     *
     * if (fd->hints->striping_unit > 0) {
     * * wkliao: implementation for file domain alignment
     * fd_start[] and fd_end[] have been aligned with file lock
     * boundaries when returned from ADIOI_Calc_file_domains() so cannot
     * just use simple arithmetic as above *
     * rank_index = 0;
     * while (off > fd_end[rank_index]) rank_index++;
     * }
     * bg does it's own striping below
     */
    rank_index = fd->hints->cb_nodes / 2;
    while (off < fd_start[rank_index] || off > fd_end[rank_index]) {
        if (off > fd_end[rank_index]) {
            lb = rank_index;
            rank_index = (rank_index + ub) / 2;
        } else if (off < fd_start[rank_index]) {
            ub = rank_index;
            rank_index = (rank_index + lb) / 2;
        }
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
    /* DBG_FPRINTF ("ADIOI_GPFS_Calc_aggregator: rank_index = %d\n",
     * rank_index); */

    /*
     * remember here that even in Rajeev's original code it was the case that
     * different aggregators could end up with different amounts of data to
     * aggregate.  here we use fd_end[] to make sure that we know how much
     * data this aggregator is working with.
     *
     * the +1 is to take into account the end vs. length issue.
     */
    avail_bytes = fd_end[rank_index] + 1 - off;
    if (avail_bytes < *len && avail_bytes > 0) {
        /* this file domain only has part of the requested contig. region */

        *len = avail_bytes;
    }

    /* map our index to a rank */
    /* NOTE: FOR NOW WE DON'T HAVE A MAPPING...JUST DO 0..NPROCS_FOR_COLL */
    rank = fd->hints->ranklist[rank_index];
    TRACE_ERR("Leaving ADIOI_GPFS_Calc_aggregator\n");

    return rank;
}

/*
 * Compute a dynamic access range based file domain partition among I/O aggregators,
 * which align to the GPFS block size
 * Divide the I/O workload among "nprocs_for_coll" processes. This is
 * done by (logically) dividing the file into file domains (FDs); each
 * process may directly access only its own file domain.
 * Additional effort is to make sure that each I/O aggregator get
 * a file domain that aligns to the GPFS block size.  So, there will
 * not be any false sharing of GPFS file blocks among multiple I/O nodes.
 *
 * The common version of this now accepts a min_fd_size and striping_unit.
 * It doesn't seem necessary here (using GPFS block sizes) but keep it in mind
 * (e.g. we could pass striping unit instead of using fs_ptr->blksize).
 */
void ADIOI_GPFS_Calc_file_domains(ADIO_File fd,
                                  ADIO_Offset * st_offsets,
                                  ADIO_Offset * end_offsets,
                                  int nprocs,
                                  int nprocs_for_coll,
                                  ADIO_Offset * min_st_offset_ptr,
                                  ADIO_Offset ** fd_start_ptr,
                                  ADIO_Offset ** fd_end_ptr,
                                  ADIO_Offset * fd_size_ptr, void *fs_ptr)
{
    ADIO_Offset min_st_offset, max_end_offset, *fd_start, *fd_end, *fd_size;
    int i, aggr;
    TRACE_ERR("Entering ADIOI_GPFS_Calc_file_domains\n");
    blksize_t blksize;

#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5004, 0, NULL);
#endif

#if AGG_DEBUG
    static char myname[] = "ADIOI_GPFS_Calc_file_domains";
    DBG_FPRINTF(stderr, "%s(%d): %d aggregator(s)\n", myname, __LINE__, nprocs_for_coll);
#endif
    if (fd->blksize <= 0)
        /* default to 1M if blksize unset */
        fd->blksize = 1048576;
    blksize = fd->blksize;

#if AGG_DEBUG
    DBG_FPRINTF(stderr, "%s(%d): Blocksize=%ld\n", myname, __LINE__, blksize);
#endif
/* find min of start offsets and max of end offsets of all processes */
    min_st_offset = st_offsets[0];
    max_end_offset = end_offsets[0];
    for (i = 1; i < nprocs; i++) {
        min_st_offset = MPL_MIN(min_st_offset, st_offsets[i]);
        max_end_offset = MPL_MAX(max_end_offset, end_offsets[i]);
    }

    /* DBG_FPRINTF(stderr, "_calc_file_domains, min_st_offset, max_
     * = %qd, %qd\n", min_st_offset, max_end_offset); */

    /* determine the "file domain (FD)" of each process, i.e., the portion of
     * the file that will be "owned" by each process */

    ADIO_Offset gpfs_ub = (max_end_offset + blksize - 1) / blksize * blksize - 1;
    ADIO_Offset gpfs_lb = min_st_offset / blksize * blksize;
    ADIO_Offset gpfs_ub_rdoff =
        (max_end_offset + blksize - 1) / blksize * blksize - 1 - max_end_offset;
    ADIO_Offset gpfs_lb_rdoff = min_st_offset - min_st_offset / blksize * blksize;
    ADIO_Offset fd_gpfs_range = gpfs_ub - gpfs_lb + 1;

    int naggs = nprocs_for_coll;

    /* Tweak the file domains so that no fd is smaller than a threshold.  We
     * have to strike a balance between efficiency and parallelism: somewhere
     * between 10k processes sending 32-byte requests and one process sending a
     * 320k request is a (system-dependent) sweet spot

     This is from the common code - the new min_fd_size parm that we didn't implement.
     (And common code uses a different declaration of fd_size so beware)

     if (fd_size < min_fd_size)
     fd_size = min_fd_size;
     */
    fd_size = ADIOI_Malloc(nprocs_for_coll * sizeof(ADIO_Offset));
    *fd_start_ptr = ADIOI_Malloc(nprocs_for_coll * 2 * sizeof(ADIO_Offset));
    *fd_end_ptr = *fd_start_ptr + nprocs_for_coll;
    fd_start = *fd_start_ptr;
    fd_end = *fd_end_ptr;

    /* each process will have a file domain of some number of gpfs blocks, but
     * the division of blocks is not likely to be even.  Some file domains will
     * be "large" and others "small"
     *
     * Example: consider  17 blocks distributed over 3 aggregators.
     * nb_cn_small = 17/3 = 5
     * naggs_large = 17 - 3*(17/3) = 17 - 15  = 2
     * naggs_small = 3 - 2 = 1
     *
     * and you end up with file domains of {5-blocks, 6-blocks, 6-blocks}
     *
     * what about (relatively) small files?  say, a file of 1000 blocks
     * distributed over 2064 aggregators:
     * nb_cn_small = 1000/2064 = 0
     * naggs_large = 1000 - 2064*(1000/2064) = 1000
     * naggs_small = 2064 - 1000 = 1064
     * and you end up with domains of {0, 0, 0, ... 1, 1, 1 ...}
     *
     * it might be a good idea instead of having all the zeros up front, to
     * "mix" those zeros into the fd_size array.  that way, no pset/bridge-set
     * is left with zero work.  In fact, even if the small file domains aren't
     * zero, it's probably still a good idea to mix the "small" file domains
     * across the fd_size array to keep the io nodes in balance */


    ADIO_Offset n_gpfs_blk = fd_gpfs_range / blksize;
    ADIO_Offset nb_cn_small = n_gpfs_blk / naggs;
    ADIO_Offset naggs_large = n_gpfs_blk - naggs * (n_gpfs_blk / naggs);
    ADIO_Offset naggs_small = naggs - naggs_large;

    for (i = 0; i < naggs; i++) {
        if (i < naggs_large) {
            fd_size[i] = (nb_cn_small + 1) * blksize;
        } else {
            fd_size[i] = nb_cn_small * blksize;
        }
    }

#if AGG_DEBUG
    DBG_FPRINTF(stderr, "%s(%d): "
                "gpfs_ub       %llu, "
                "gpfs_lb       %llu, "
                "gpfs_ub_rdoff %llu, "
                "gpfs_lb_rdoff %llu, "
                "fd_gpfs_range %llu, "
                "n_gpfs_blk    %llu, "
                "nb_cn_small   %llu, "
                "naggs_large   %llu, "
                "naggs_small   %llu, "
                "\n",
                myname, __LINE__,
                gpfs_ub,
                gpfs_lb,
                gpfs_ub_rdoff,
                gpfs_lb_rdoff, fd_gpfs_range, n_gpfs_blk, nb_cn_small, naggs_large, naggs_small);
#endif

    fd_size[0] -= gpfs_lb_rdoff;
    fd_size[naggs - 1] -= gpfs_ub_rdoff;

    /* compute the file domain for each aggr */
    ADIO_Offset offset = min_st_offset;
    for (aggr = 0; aggr < naggs; aggr++) {
        fd_start[aggr] = offset;
        fd_end[aggr] = offset + fd_size[aggr] - 1;
        offset += fd_size[aggr];
    }

    *fd_size_ptr = fd_size[0];
    *min_st_offset_ptr = min_st_offset;

#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5005, 0, NULL);
#endif
    ADIOI_Free(fd_size);
    TRACE_ERR("Leaving ADIOI_GPFS_Calc_file_domains\n");
}

/*
 * ADIOI_GPFS_Calc_my_req() overrides ADIOI_Calc_my_req for the default implementation
 * is specific for static file domain partitioning.
 *
 * ADIOI_Calc_my_req() - calculate what portions of the access requests
 * of this process are located in the file domains of various processes
 * (including this one)
 */
void ADIOI_GPFS_Calc_my_req(ADIO_File fd, ADIO_Offset * offset_list, ADIO_Offset * len_list,
                            MPI_Count contig_access_count, ADIO_Offset
                            min_st_offset, ADIO_Offset * fd_start,
                            ADIO_Offset * fd_end, ADIO_Offset fd_size,
                            int nprocs,
                            MPI_Count * count_my_req_procs_ptr,
                            MPI_Count ** count_my_req_per_proc_ptr,
                            ADIOI_Access ** my_req_ptr, MPI_Aint ** buf_idx_ptr)
{
    MPI_Count *count_my_req_per_proc, count_my_req_procs;
    MPI_Aint *buf_idx;
    MPI_Count l, proc;
    ADIO_Offset fd_len, rem_len, curr_idx, off;
    ADIOI_Access *my_req;
    TRACE_ERR("Entering ADIOI_GPFS_Calc_my_req\n");

#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5024, 0, NULL);
#endif
    *count_my_req_per_proc_ptr = ADIOI_Calloc(nprocs, sizeof(**count_my_req_per_proc_ptr));
    count_my_req_per_proc = *count_my_req_per_proc_ptr;
/* count_my_req_per_proc[i] gives the no. of contig. requests of this
   process in process i's file domain. calloc initializes to zero.
   I'm allocating memory of size nprocs, so that I can do an
   MPI_Alltoall later on.*/

    buf_idx = ADIOI_Malloc(nprocs * sizeof(MPI_Aint));
/* buf_idx is relevant only if buftype_is_contig.
   buf_idx[i] gives the index into user_buf where data received
   from proc. i should be placed. This allows receives to be done
   without extra buffer. This can't be done if buftype is not contig. */

    /* initialize buf_idx to -1 */
    for (int i = 0; i < nprocs; i++)
        buf_idx[i] = -1;

    /* one pass just to calculate how much space to allocate for my_req;
     * contig_access_count was calculated way back in ADIOI_Calc_my_off_len()
     */
    for (MPI_Count i = 0; i < contig_access_count; i++) {
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
        /* BES */
        proc = ADIOI_GPFS_Calc_aggregator(fd, off, min_st_offset, &fd_len, fd_size,
                                          fd_start, fd_end);
        count_my_req_per_proc[proc]++;

        /* figure out how much data is remaining in the access (i.e. wasn't
         * part of the file domain that had the starting byte); we'll take
         * care of this data (if there is any) in the while loop below.
         */
        rem_len = len_list[i] - fd_len;

        while (rem_len > 0) {
            off += fd_len;      /* point to first remaining byte */
            fd_len = rem_len;   /* save remaining size, pass to calc */
            proc = ADIOI_GPFS_Calc_aggregator(fd, off, min_st_offset, &fd_len,
                                              fd_size, fd_start, fd_end);

            count_my_req_per_proc[proc]++;
            rem_len -= fd_len;  /* reduce remaining length by amount from fd */
        }
    }

/* now allocate space for my_req, offset, and len */

    *my_req_ptr = ADIOI_Malloc(nprocs * sizeof(ADIOI_Access));
    my_req = *my_req_ptr;

    count_my_req_procs = 0;
    for (int i = 0; i < nprocs; i++) {
        if (count_my_req_per_proc[i]) {
            my_req[i].offsets = ADIOI_Malloc(count_my_req_per_proc[i] * 2 * sizeof(ADIO_Offset));
            my_req[i].lens = my_req[i].offsets + count_my_req_per_proc[i];
            count_my_req_procs++;
        }
        my_req[i].count = 0;    /* will be incremented where needed
                                 * later */
    }

/* now fill in my_req */
    curr_idx = 0;
    for (MPI_Count i = 0; i < contig_access_count; i++) {
        /* short circuit offset/len processing if len == 0
         *      (zero-byte  read/write */
        if (len_list[i] == 0)
            continue;
        off = offset_list[i];
        fd_len = len_list[i];
        proc = ADIOI_GPFS_Calc_aggregator(fd, off, min_st_offset, &fd_len, fd_size,
                                          fd_start, fd_end);

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

        while (rem_len > 0) {
            off += fd_len;
            fd_len = rem_len;
            proc = ADIOI_GPFS_Calc_aggregator(fd, off, min_st_offset, &fd_len,
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
    for (int i = 0; i < nprocs; i++) {
        if (count_my_req_per_proc[i] > 0) {
            DBG_FPRINTF(stderr, "data needed from %d (count = %lld):\n", i, my_req[i].count);
            for (l = 0; l < my_req[i].count; l++) {
                DBG_FPRINTF(stderr, "   off[%lld] = %lld, len[%lld] = %lld\n", l,
                            my_req[i].offsets[l], l, my_req[i].lens[l]);
            }
        }
        DBG_FPRINTF(stderr, "buf_idx[%d] = 0x%lx\n", i, buf_idx[i]);
    }
#endif

    *count_my_req_procs_ptr = count_my_req_procs;
    *buf_idx_ptr = buf_idx;
#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5025, 0, NULL);
#endif
    TRACE_ERR("Leaving ADIOI_GPFS_Calc_my_req\n");
}

void ADIOI_GPFS_Free_my_req(int nprocs, MPI_Count * count_my_req_per_proc,
                            ADIOI_Access * my_req, MPI_Aint * buf_idx)
{
    for (int i = 0; i < nprocs; i++) {
        if (count_my_req_per_proc[i]) {
            ADIOI_Free(my_req[i].offsets);
        }
    }
    ADIOI_Free(my_req);
    ADIOI_Free(count_my_req_per_proc);
    ADIOI_Free(buf_idx);
}

/*
 * ADIOI_Calc_others_req (copied to bg and switched to all to all for performance)
 *
 * param[in]  count_my_req_procs        Number of processes whose file domain my
 *                                        request touches.
 * param[in]  count_my_req_per_proc     count_my_req_per_proc[i] gives the no. of
 *                                        contig. requests of this process in
 *                                        process i's file domain.
 * param[in]  my_req                    A structure defining my request
 * param[in]  nprocs                    Number of nodes in the block
 * param[in]  myrank                    Rank of this node
 * param[out] count_others_req_proc_ptr Number of processes whose requests lie in
 *                                        my process's file domain (including my
 *                                        process itself)
 * param[out] others_req_ptr            Array of other process' requests that lie
 *                                        in my process's file domain
 */
void ADIOI_GPFS_Calc_others_req(ADIO_File fd, MPI_Count count_my_req_procs,
                                MPI_Count * count_my_req_per_proc,
                                ADIOI_Access * my_req,
                                int nprocs, int myrank,
                                MPI_Count * count_others_req_procs_ptr,
                                MPI_Count ** count_others_req_per_proc_ptr,
                                ADIOI_Access ** others_req_ptr)
{
    TRACE_ERR("Entering ADIOI_GPFS_Calc_others_req\n");
/* determine what requests of other processes lie in this process's
   file domain */

/* count_others_req_procs = number of processes whose requests lie in
   this process's file domain (including this process itself)
   count_others_req_per_proc[i] indicates how many separate contiguous
   requests of proc. i lie in this process's file domain. */

    MPI_Count *count_others_req_per_proc, count_others_req_procs;
    ADIOI_Access *others_req;

#if MPI_VERSION >= 4
#define HAS_ALLTOALLV_C 1
#else
#define HAS_ALLTOALLV_C 0
#endif
    /* Parameters for MPI_Alltoallv */
    MPI_Aint *sdispls, *rdispls;

    /* Parameters for MPI_Alltoallv.  These are the buffers, which
     * are later computed to be the lowest address of all buffers
     * to be sent/received for offsets and lengths.  Initialize to
     * the highest possible address which is the current minimum.
     */
    void *sendBuf = (void *) 0xFFFFFFFFFFFFFFFF, *recvBuf = (void *) 0xFFFFFFFFFFFFFFFF;

/* first find out how much to send/recv and from/to whom */
#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5026, 0, NULL);
#endif
    /* Preliminary "how much work will we do?" data exchange:
     * count_my_req_per_proc[i] is the number of
     * requests that my process will do to the file domain owned by process[i].
     * count_others_req_per_proc[i] is the number of
     * requests that process[i] will do to the file domain owned by my process.
     * Just sending/receiving one value here: a few lines later we'll set up
     * the data structures for the full alltoallv */
    count_others_req_per_proc = ADIOI_Malloc(nprocs * sizeof(*count_others_req_per_proc));
/*     cora2a1=timebase(); */
/*for(i=0;i<nprocs;i++) ?*/
    MPI_Alltoall(count_my_req_per_proc, 1, MPI_COUNT,
                 count_others_req_per_proc, 1, MPI_COUNT, fd->comm);

/*     total_cora2a+=timebase()-cora2a1; */

    /* Allocate storage for an array of other nodes' accesses of our
     * node's file domain.  Also allocate storage for the alltoallv
     * parameters.
     */
    *others_req_ptr = ADIOI_Malloc(nprocs * sizeof(ADIOI_Access));
    others_req = *others_req_ptr;

    sdispls = ADIOI_Malloc(nprocs * sizeof(MPI_Aint));
    rdispls = ADIOI_Malloc(nprocs * sizeof(MPI_Aint));
#if HAS_ALLTOALLV_C
    MPI_Count *scounts = ADIOI_Malloc(nprocs * sizeof(MPI_Count));
    MPI_Count *rcounts = ADIOI_Malloc(nprocs * sizeof(MPI_Count));
#else
    int *scounts = ADIOI_Malloc(nprocs * sizeof(int));
    int *rcounts = ADIOI_Malloc(nprocs * sizeof(int));
#endif

    /* If process[i] has any requests in my file domain,
     *   initialize an ADIOI_Access structure that will describe each request
     *   from process[i].  The offsets, lengths, and buffer pointers still need
     *   to be obtained to complete the setting of this structure.
     */
    count_others_req_procs = 0;
    for (int i = 0; i < nprocs; i++) {
        if (count_others_req_per_proc[i]) {
            others_req[i].count = count_others_req_per_proc[i];

            others_req[i].offsets =
                ADIOI_Malloc(count_others_req_per_proc[i] * 2 * sizeof(ADIO_Offset));
            others_req[i].lens = others_req[i].offsets + count_others_req_per_proc[i];

            if ((uintptr_t) others_req[i].offsets < (uintptr_t) recvBuf)
                recvBuf = others_req[i].offsets;

            others_req[i].mem_ptrs = ADIOI_Malloc(count_others_req_per_proc[i] * sizeof(MPI_Aint));

            count_others_req_procs++;
        } else {
            others_req[i].count = 0;
            others_req[i].offsets = NULL;
            others_req[i].mem_ptrs = NULL;
            others_req[i].lens = NULL;
        }
    }
    /* If no recv buffer was allocated in the loop above, make it NULL */
    if (recvBuf == (void *) 0xFFFFFFFFFFFFFFFF)
        recvBuf = NULL;

    /* Now send the calculated offsets and lengths to respective processes */

    /************************/
    /* Exchange the offsets */
    /************************/

    /* Determine the lowest sendBuf */
    for (int i = 0; i < nprocs; i++) {
        if ((my_req[i].count) && ((uintptr_t) my_req[i].offsets <= (uintptr_t) sendBuf)) {
            sendBuf = my_req[i].offsets;
        }
        /* my_req[i].offsets and my_req[i].lens have been malloc-ed together */
    }

    /* If no send buffer was found in the loop above, make it NULL */
    if (sendBuf == (void *) 0xFFFFFFFFFFFFFFFF)
        sendBuf = NULL;

    /* Calculate the displacements from the sendBuf */
    for (int i = 0; i < nprocs; i++) {
        /* Send these offsets and lengths to process i. */
        scounts[i] = count_my_req_per_proc[i] * 2;
        if (scounts[i] == 0)
            sdispls[i] = 0;
        else
            sdispls[i] = (MPI_Aint)
                (((uintptr_t) my_req[i].offsets -
                  (uintptr_t) sendBuf) / (uintptr_t) sizeof(ADIO_Offset));

        /* Receive these offsets and lengths from process i. */
        rcounts[i] = count_others_req_per_proc[i] * 2;
        if (rcounts[i] == 0)
            rdispls[i] = 0;
        else
            rdispls[i] = (MPI_Aint)
                (((uintptr_t) others_req[i].offsets -
                  (uintptr_t) recvBuf) / (uintptr_t) sizeof(ADIO_Offset));
    }

    /* Exchange the offsets and lengths */
#if HAS_ALLTOALLV_C
    MPI_Alltoallv_c(sendBuf, scounts, sdispls, ADIO_OFFSET,
                    recvBuf, rcounts, rdispls, ADIO_OFFSET, fd->comm);
#else
    MY_Alltoallv(sendBuf, scounts, sdispls, ADIO_OFFSET,
                 recvBuf, rcounts, rdispls, ADIO_OFFSET, fd->comm);
#endif

    /* Clean up */
    ADIOI_Free(scounts);
    ADIOI_Free(sdispls);
    ADIOI_Free(rcounts);
    ADIOI_Free(rdispls);

    *count_others_req_procs_ptr = count_others_req_procs;
    *count_others_req_per_proc_ptr = count_others_req_per_proc;
#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5027, 0, NULL);
#endif
    TRACE_ERR("Leaving ADIOI_GPFS_Calc_others_req\n");
}

void ADIOI_GPFS_Free_others_req(int nprocs, MPI_Count * count_others_req_per_proc,
                                ADIOI_Access * others_req)
{
    for (int i = 0; i < nprocs; i++) {
        if (count_others_req_per_proc[i]) {
            ADIOI_Free(others_req[i].offsets);
            ADIOI_Free(others_req[i].mem_ptrs);
        }
    }
    ADIOI_Free(others_req);
    ADIOI_Free(count_others_req_per_proc);
}

#ifndef HAS_ALLTOALLV_C
/*
 *  Alltoallv with MPI_Aint for sdisps/rdisps
 *
 *  If the disps are actually small enough to fit in an int, it
 *  creates int arrays and calls regular MPI_Alltoallv.
 *  Otherwise it does a bunch of copies to make the data contiguous
 *  so it can fit in int displacements.
 */
static int
MY_Alltoallv(void *sbuf, int *scounts, MPI_Aint * sdisps, MPI_Datatype stype,
             void *rbuf, int *rcounts, MPI_Aint * rdisps, MPI_Datatype rtype, MPI_Comm comm)
{
    int sizeof_stype, sizeof_rtype;
    int rv, i;
    int nranks;
    int disps_are_small_enough;
    int *sdisps_int;
    int *rdisps_int;

    MPI_Comm_size(comm, &nranks);
    MPI_Type_size(stype, &sizeof_stype);
    MPI_Type_size(rtype, &sizeof_rtype);
    sdisps_int = ADIOI_Malloc(2 * nranks * sizeof(int));
    rdisps_int = &sdisps_int[nranks];

    disps_are_small_enough = 1;
    for (i = 0; i < nranks && disps_are_small_enough; ++i) {
        if (sdisps[i] != (int) sdisps[i]) {
            disps_are_small_enough = 0;
        }
    }
    for (i = 0; i < nranks && disps_are_small_enough; ++i) {
        if (rdisps[i] != (int) rdisps[i]) {
            disps_are_small_enough = 0;
        }
    }

    if (disps_are_small_enough) {
        for (i = 0; i < nranks; ++i) {
            /* cast ok: we shouldn't be here if Alltoallv_c available, and
             * displacement sizes checked right above us */
            sdisps_int[i] = (int) sdisps[i];
        }
        for (i = 0; i < nranks; ++i) {
            /* cast ok: we shouldn't be here if Alltoallv_c available, and
             * displacement sizes checked right above us */
            rdisps_int[i] = (int) rdisps[i];
        }
        rv = MPI_Alltoallv(sbuf, scounts, sdisps_int, stype,
                           rbuf, rcounts, rdisps_int, rtype, comm);
        ADIOI_Free(sdisps_int);
        return rv;
    }

    void *sbuf_copy;
    void *rbuf_copy;
    size_t scount_total = 0;
    size_t rcount_total = 0;
    for (i = 0; i < nranks; i++) {
        if ((scount_total != (int) scount_total) || (rcount_total != (int) rcount_total)) {
            FPRINTF(stderr,
                    "Error in %s: integer overflow / scount_total(%zu) != (int)scount_total(%d)"
                    " || rcount_total(%zu) != (int)rcount_total(%d)\n",
                    __func__, scount_total, (int) scount_total, rcount_total, (int) rcount_total);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        sdisps_int[i] = (int) scount_total;
        rdisps_int[i] = (int) rcount_total;
        scount_total += scounts[i];
        rcount_total += rcounts[i];
    }
    sbuf_copy = ADIOI_Malloc(scount_total * sizeof_stype);
    rbuf_copy = ADIOI_Malloc(rcount_total * sizeof_rtype);
    for (i = 0; i < nranks; i++) {
        memcpy((char *) sbuf_copy + sdisps_int[i] * sizeof_stype,
               (char *) sbuf + sdisps[i] * sizeof_stype, scounts[i] * sizeof_stype);
    }
    rv = MPI_Alltoallv(sbuf_copy, scounts, sdisps_int, stype,
                       rbuf_copy, rcounts, rdisps_int, rtype, comm);
    for (i = 0; i < nranks; i++) {
        memcpy((char *) rbuf + rdisps[i] * sizeof_rtype,
               (char *) rbuf_copy + rdisps_int[i] * sizeof_rtype, rcounts[i] * sizeof_rtype);
    }

    ADIOI_Free(sbuf_copy);
    ADIOI_Free(rbuf_copy);
    ADIOI_Free(sdisps_int);
    return rv;
}
#endif
