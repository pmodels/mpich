/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/**
 * \file ad_gpfs_rdcoll.c
 * \brief ???
 */

#include "adio.h"
#include "adio_extern.h"
#include "ad_gpfs.h"
#include "ad_gpfs_aggrs.h"

#ifdef PROFILE
#include "mpe.h"
#endif

#ifdef MPL_USE_DBG_LOGGING
#define RDCOLL_DEBUG 1
#endif
#ifdef AGGREGATION_PROFILE
#include "mpe.h"
#endif

/* prototypes of functions used for collective reads only. */
static void ADIOI_Read_and_exch(ADIO_File fd, void *buf, MPI_Datatype
                                datatype, int nprocs,
                                int myrank, ADIOI_Access
                                * others_req, ADIO_Offset * offset_list,
                                ADIO_Offset * len_list, MPI_Count contig_access_count,
                                ADIO_Offset
                                min_st_offset, ADIO_Offset fd_size,
                                ADIO_Offset * fd_start, ADIO_Offset * fd_end,
                                MPI_Aint * buf_idx, int *error_code);
static void ADIOI_R_Exchange_data(ADIO_File fd, void *buf, ADIOI_Flatlist_node
                                  * flat_buf, ADIO_Offset * offset_list, ADIO_Offset
                                  * len_list, MPI_Count * send_size, MPI_Count * recv_size,
                                  MPI_Count * count, MPI_Count * start_pos,
                                  MPI_Count * partial_send,
                                  MPI_Count * recd_from_proc, int nprocs,
                                  int myrank, int
                                  buftype_is_contig, MPI_Count contig_access_count,
                                  ADIO_Offset min_st_offset,
                                  ADIO_Offset fd_size,
                                  ADIO_Offset * fd_start, ADIO_Offset * fd_end,
                                  ADIOI_Access * others_req,
                                  int iter, MPI_Aint buftype_extent, MPI_Aint * buf_idx);
static void ADIOI_R_Exchange_data_alltoallv(ADIO_File fd, void *buf, ADIOI_Flatlist_node
                                            * flat_buf, ADIO_Offset * offset_list, ADIO_Offset
                                            * len_list, MPI_Count * send_size,
                                            MPI_Count * recv_size, MPI_Count * count,
                                            MPI_Count * start_pos, MPI_Count * partial_send,
                                            MPI_Count * recd_from_proc, int nprocs, int myrank,
                                            int buftype_is_contig, MPI_Count contig_access_count,
                                            ADIO_Offset min_st_offset, ADIO_Offset fd_size,
                                            ADIO_Offset * fd_start, ADIO_Offset * fd_end,
                                            ADIOI_Access * others_req, int iter,
                                            MPI_Aint buftype_extent, MPI_Aint * buf_idx);
static void ADIOI_Fill_user_buffer(ADIO_File fd, void *buf, ADIOI_Flatlist_node * flat_buf,
                                   char **recv_buf, ADIO_Offset * offset_list,
                                   ADIO_Offset * len_list, MPI_Count * recv_size,
                                   MPI_Request * requests, MPI_Status * statuses,
                                   MPI_Count * recd_from_proc, int nprocs,
                                   MPI_Count contig_access_count, ADIO_Offset min_st_offset,
                                   ADIO_Offset fd_size, ADIO_Offset * fd_start,
                                   ADIO_Offset * fd_end, MPI_Aint buftype_extent);

extern void ADIOI_Calc_my_off_len(ADIO_File fd, MPI_Aint bufcount, MPI_Datatype
                                  datatype, int file_ptr_type, ADIO_Offset
                                  offset, ADIO_Offset ** offset_list_ptr, ADIO_Offset
                                  ** len_list_ptr, ADIO_Offset * start_offset_ptr,
                                  ADIO_Offset * end_offset_ptr,
                                  MPI_Count * contig_access_count_ptr);



void ADIOI_GPFS_ReadStridedColl(ADIO_File fd, void *buf, MPI_Aint count,
                                MPI_Datatype datatype, int file_ptr_type,
                                ADIO_Offset offset, ADIO_Status * status, int
                                *error_code)
{
/* Uses a generalized version of the extended two-phase method described
   in "An Extended Two-Phase Method for Accessing Sections of
   Out-of-Core Arrays", Rajeev Thakur and Alok Choudhary,
   Scientific Programming, (5)4:301--317, Winter 1996.
   http://www.mcs.anl.gov/home/thakur/ext2ph.ps */

    ADIOI_Access *my_req;
    /* array of nprocs structures, one for each other process in
     * whose file domain this process's request lies */

    ADIOI_Access *others_req;
    /* array of nprocs structures, one for each other process
     * whose request lies in this process's file domain. */

    int filetype_is_contig, nprocs, nprocs_for_coll, myrank;
    MPI_Count contig_access_count = 0;
    int interleave_count = 0, buftype_is_contig;
    MPI_Count *count_my_req_per_proc, count_my_req_procs;
    MPI_Count *count_others_req_per_proc, count_others_req_procs;
    ADIO_Offset start_offset, end_offset, orig_fp, fd_size, min_st_offset, off;
    ADIO_Offset *offset_list = NULL, *st_offsets = NULL, *fd_start = NULL,
        *fd_end = NULL, *end_offsets = NULL;
    ADIO_Offset *gpfs_offsets0 = NULL, *gpfs_offsets = NULL;
    ADIO_Offset *count_sizes;
    int ii;
    ADIO_Offset *len_list = NULL;
    MPI_Aint *buf_idx = NULL;

    GPFSMPIO_T_CIO_RESET(r);

#ifdef HAVE_STATUS_SET_BYTES
    MPI_Count bufsize, size;
#endif

#if 0
    /* From common code - not implemented for bg. */
    if (fd->hints->cb_pfr != ADIOI_HINT_DISABLE) {
        ADIOI_IOStridedColl(fd, buf, count, ADIOI_READ, datatype,
                            file_ptr_type, offset, status, error_code);
        return;
    }
#endif
#ifdef PROFILE
    MPE_Log_event(13, 0, "start computation");
#endif

    MPI_Comm_size(fd->comm, &nprocs);
    MPI_Comm_rank(fd->comm, &myrank);

    /* number of aggregators, cb_nodes, is stored in the hints */
    nprocs_for_coll = fd->hints->cb_nodes;
    orig_fp = fd->fp_ind;

    GPFSMPIO_T_CIO_SET_GET(r, 1, 0, GPFSMPIO_CIO_T_MPIO_CRW, GPFSMPIO_CIO_LAST);
    GPFSMPIO_T_CIO_SET_GET(r, 1, 0, GPFSMPIO_CIO_T_LCOMP, GPFSMPIO_CIO_LAST);

    /* only check for interleaving if cb_read isn't disabled */
    if (fd->hints->cb_read != ADIOI_HINT_DISABLE) {
        /* For this process's request, calculate the list of offsets and
         * lengths in the file and determine the start and end offsets.
         * Note: end_offset points to the last byte-offset to be accessed.
         * e.g., if start_offset=0 and 100 bytes to be read, end_offset=99
         */
        ADIOI_Calc_my_off_len(fd, count, datatype, file_ptr_type, offset,
                              &offset_list, &len_list, &start_offset,
                              &end_offset, &contig_access_count);

        GPFSMPIO_T_CIO_SET_GET(r, 1, 1, GPFSMPIO_CIO_T_GATHER, GPFSMPIO_CIO_T_LCOMP);
#ifdef RDCOLL_DEBUG
        for (MPI_Count i = 0; i < contig_access_count; i++) {
            DBG_FPRINTF(stderr, "rank %d  off %lld  len %lld\n",
                        myrank, (long long) offset_list[i], (long long) len_list[i]);
        }
#endif

        /* each process communicates its start and end offsets to other
         * processes. The result is an array each of start and end offsets
         * stored in order of process rank. */

        st_offsets = (ADIO_Offset *) ADIOI_Malloc(nprocs * 2 * sizeof(ADIO_Offset));
        end_offsets = st_offsets + nprocs;

        ADIO_Offset my_count_size = 0;
        /* One-sided aggregation needs the amount of data per rank as well
         * because the difference in starting and ending offsets for 1 byte is
         * 0 the same as 0 bytes so it cannot be distinguished.
         */
        if ((romio_read_aggmethod == 1) || (romio_read_aggmethod == 2)) {
            count_sizes = (ADIO_Offset *) ADIOI_Malloc(nprocs * sizeof(ADIO_Offset));
            MPI_Count buftype_size;
            MPI_Type_size_x(datatype, &buftype_size);
            my_count_size = (ADIO_Offset) count *(ADIO_Offset) buftype_size;
        }
        if (romio_tunegather) {
            if ((romio_read_aggmethod == 1) || (romio_read_aggmethod == 2)) {
                gpfs_offsets0 = (ADIO_Offset *) ADIOI_Malloc(6 * nprocs * sizeof(ADIO_Offset));
                gpfs_offsets = gpfs_offsets0 + 3 * nprocs;
                for (ii = 0; ii < nprocs; ii++) {
                    gpfs_offsets0[ii * 3] = 0;
                    gpfs_offsets0[ii * 3 + 1] = 0;
                    gpfs_offsets0[ii * 3 + 2] = 0;
                }
                gpfs_offsets0[myrank * 3] = start_offset;
                gpfs_offsets0[myrank * 3 + 1] = end_offset;
                gpfs_offsets0[myrank * 3 + 2] = my_count_size;
                MPI_Allreduce(gpfs_offsets0, gpfs_offsets, nprocs * 3, ADIO_OFFSET, MPI_MAX,
                              fd->comm);
                for (ii = 0; ii < nprocs; ii++) {
                    st_offsets[ii] = gpfs_offsets[ii * 3];
                    end_offsets[ii] = gpfs_offsets[ii * 3 + 1];
                    count_sizes[ii] = gpfs_offsets[ii * 3 + 2];
                }
            } else {
                gpfs_offsets0 = (ADIO_Offset *) ADIOI_Malloc(4 * nprocs * sizeof(ADIO_Offset));
                gpfs_offsets = gpfs_offsets0 + 2 * nprocs;
                for (ii = 0; ii < nprocs; ii++) {
                    gpfs_offsets0[ii * 2] = 0;
                    gpfs_offsets0[ii * 2 + 1] = 0;
                }
                gpfs_offsets0[myrank * 2] = start_offset;
                gpfs_offsets0[myrank * 2 + 1] = end_offset;

                MPI_Allreduce(gpfs_offsets0, gpfs_offsets, nprocs * 2, ADIO_OFFSET, MPI_MAX,
                              fd->comm);

                for (ii = 0; ii < nprocs; ii++) {
                    st_offsets[ii] = gpfs_offsets[ii * 2];
                    end_offsets[ii] = gpfs_offsets[ii * 2 + 1];
                }
            }
            ADIOI_Free(gpfs_offsets0);
        } else {
            MPI_Allgather(&start_offset, 1, ADIO_OFFSET, st_offsets, 1, ADIO_OFFSET, fd->comm);
            MPI_Allgather(&end_offset, 1, ADIO_OFFSET, end_offsets, 1, ADIO_OFFSET, fd->comm);
            if ((romio_read_aggmethod == 1) || (romio_read_aggmethod == 2)) {
                MPI_Allgather(&count_sizes, 1, ADIO_OFFSET, count_sizes, 1, ADIO_OFFSET, fd->comm);
            }
        }

        GPFSMPIO_T_CIO_SET_GET(r, 1, 1, GPFSMPIO_CIO_T_PATANA, GPFSMPIO_CIO_T_GATHER);

        /* are the accesses of different processes interleaved? */
        for (int i = 1; i < nprocs; i++)
            if ((st_offsets[i] < end_offsets[i - 1]) && (st_offsets[i] <= end_offsets[i]))
                interleave_count++;
        /* This is a rudimentary check for interleaving, but should suffice
         * for the moment. */
    }

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);

    if (fd->hints->cb_read == ADIOI_HINT_DISABLE
        || (!interleave_count && (fd->hints->cb_read == ADIOI_HINT_AUTO))) {
        /* don't do aggregation */
        if (fd->hints->cb_read != ADIOI_HINT_DISABLE) {
            ADIOI_Free(offset_list);
            ADIOI_Free(st_offsets);
        }

        fd->fp_ind = orig_fp;
        ADIOI_Datatype_iscontig(fd->filetype, &filetype_is_contig);

        if (buftype_is_contig && filetype_is_contig) {
            if (file_ptr_type == ADIO_EXPLICIT_OFFSET) {
                off = fd->disp + (ADIO_Offset) (fd->etype_size) * offset;
                ADIO_ReadContig(fd, buf, count, datatype, ADIO_EXPLICIT_OFFSET,
                                off, status, error_code);
            } else
                ADIO_ReadContig(fd, buf, count, datatype, ADIO_INDIVIDUAL, 0, status, error_code);
        } else
            ADIO_ReadStrided(fd, buf, count, datatype, file_ptr_type, offset, status, error_code);

        return;
    }

    GPFSMPIO_T_CIO_SET_GET(r, 1, 1, GPFSMPIO_CIO_T_FD_PART, GPFSMPIO_CIO_T_PATANA);

    /* We're going to perform aggregation of I/O.  Here we call
     * ADIOI_Calc_file_domains() to determine what processes will handle I/O
     * to what regions.  We pass nprocs_for_coll into this function; it is
     * used to determine how many processes will perform I/O, which is also
     * the number of regions into which the range of bytes must be divided.
     * These regions are called "file domains", or FDs.
     *
     * When this function returns, fd_start, fd_end, fd_size, and
     * min_st_offset will be filled in.  fd_start holds the starting byte
     * location for each file domain.  fd_end holds the ending byte location.
     * min_st_offset holds the minimum byte location that will be accessed.
     *
     * Both fd_start[] and fd_end[] are indexed by an aggregator number; this
     * needs to be mapped to an actual rank in the communicator later.
     *
     */
    int currentNonZeroDataIndex = 0;
    if ((romio_read_aggmethod == 1) || (romio_read_aggmethod == 2)) {
        /* Take out the 0-data offsets by shifting the indexes with data to the
         * front and keeping track of the non-zero data index for use as the
         * length.  By doing this we will optimally use all available aggs
         * and spread the actual data across them instead of having offsets
         * with empty data potentially dilute the file domains and create
         * problems for the one-sided aggregation.
         */
        for (int i = 0; i < nprocs; i++) {
            if (count_sizes[i] > 0) {
                st_offsets[currentNonZeroDataIndex] = st_offsets[i];
                end_offsets[currentNonZeroDataIndex] = end_offsets[i];
                currentNonZeroDataIndex++;
            }
        }
    }
    if (gpfsmpio_tuneblocking) {
        if ((romio_read_aggmethod == 1) || (romio_read_aggmethod == 2)) {
            ADIOI_GPFS_Calc_file_domains(fd, st_offsets, end_offsets, currentNonZeroDataIndex,
                                         nprocs_for_coll, &min_st_offset,
                                         &fd_start, &fd_end, &fd_size, fd->fs_ptr);
        } else {
            ADIOI_GPFS_Calc_file_domains(fd, st_offsets, end_offsets, nprocs,
                                         nprocs_for_coll, &min_st_offset,
                                         &fd_start, &fd_end, &fd_size, fd->fs_ptr);
        }
    } else {
        if ((romio_read_aggmethod == 1) || (romio_read_aggmethod == 2)) {
            ADIOI_Calc_file_domains(st_offsets, end_offsets, currentNonZeroDataIndex,
                                    nprocs_for_coll, &min_st_offset,
                                    &fd_start, &fd_end,
                                    fd->hints->min_fdomain_size, &fd_size,
                                    fd->hints->striping_unit);
        } else {
            ADIOI_Calc_file_domains(st_offsets, end_offsets, nprocs,
                                    nprocs_for_coll, &min_st_offset,
                                    &fd_start, &fd_end,
                                    fd->hints->min_fdomain_size, &fd_size,
                                    fd->hints->striping_unit);
        }
    }

    GPFSMPIO_T_CIO_SET_GET(r, 1, 1, GPFSMPIO_CIO_T_MYREQ, GPFSMPIO_CIO_T_FD_PART);
    if ((romio_read_aggmethod == 1) || (romio_read_aggmethod == 2)) {
        /* If the user has specified to use a one-sided aggregation method then
         * do that at this point instead of the two-phase I/O.
         */
        ADIOI_OneSidedReadAggregation(fd, offset_list, len_list, contig_access_count, buf,
                                      datatype, error_code, st_offsets, end_offsets,
                                      currentNonZeroDataIndex, fd_start, fd_end);
        GPFSMPIO_T_CIO_REPORT(0, fd, myrank, nprocs);
        ADIOI_Free(offset_list);
        ADIOI_Free(st_offsets);
        ADIOI_Free(fd_start);
        ADIOI_Free(count_sizes);
        goto fn_exit;
    }
    if (gpfsmpio_p2pcontig == 1) {
        /* For some simple yet common(?) workloads, full-on two-phase I/O is
         * overkill.  We can establish sub-groups of processes and their
         * aggregator, and then these sub-groups will carry out a simplified
         * two-phase over that sub-group.
         *
         * First verify that the filetype is contig and the offsets are
         * increasing in rank order*/
        int x, inOrderAndNoGaps = 1;
        for (x = 0; x < (nprocs - 1); x++) {
            if (end_offsets[x] != (st_offsets[x + 1] - 1))
                inOrderAndNoGaps = 0;
        }
        if (inOrderAndNoGaps && buftype_is_contig) {
            /* if these conditions exist then execute the P2PContig code else
             * execute the original code */
            ADIOI_P2PContigReadAggregation(fd, buf,
                                           error_code, st_offsets, end_offsets, fd_start, fd_end);

            /* NOTE: we are skipping the rest of two-phase in this path */
            GPFSMPIO_T_CIO_REPORT(0, fd, myrank, nprocs);

            ADIOI_Free(offset_list);
            ADIOI_Free(st_offsets);
            ADIOI_Free(fd_start);
            goto fn_exit;
        }
    }

    /* calculate where the portions of the access requests of this process
     * are located in terms of the file domains.  this could be on the same
     * process or on other processes.  this function fills in:
     * count_my_req_procs - number of processes (including this one) for which
     *     this process has requests in their file domain
     * count_my_req_per_proc - count of requests for each process, indexed
     *     by rank of the process
     * my_req[] - array of data structures describing the requests to be
     *     performed by each process (including self).  indexed by rank.
     * buf_idx[] - array of locations into which data can be directly moved;
     *     this is only valid for contiguous buffer case
     */
    if (gpfsmpio_tuneblocking)
        ADIOI_GPFS_Calc_my_req(fd, offset_list, len_list, contig_access_count,
                               min_st_offset, fd_start, fd_end, fd_size,
                               nprocs, &count_my_req_procs,
                               &count_my_req_per_proc, &my_req, &buf_idx);
    else
        ADIOI_Calc_my_req(fd, offset_list, len_list, contig_access_count,
                          min_st_offset, fd_start, fd_end, fd_size,
                          nprocs, &count_my_req_procs, &count_my_req_per_proc, &my_req, &buf_idx);

    GPFSMPIO_T_CIO_SET_GET(r, 1, 1, GPFSMPIO_CIO_T_OTHREQ, GPFSMPIO_CIO_T_MYREQ);

    /* perform a collective communication in order to distribute the
     * data calculated above.  fills in the following:
     * count_others_req_procs - number of processes (including this
     *     one) which have requests in this process's file domain.
     * count_others_req_per_proc[] - number of separate contiguous
     *     requests from proc i lie in this process's file domain.
     */
    if (gpfsmpio_tuneblocking)
        ADIOI_GPFS_Calc_others_req(fd, count_my_req_procs,
                                   count_my_req_per_proc, my_req,
                                   nprocs, myrank, &count_others_req_procs,
                                   &count_others_req_per_proc, &others_req);
    else
        ADIOI_Calc_others_req(fd, count_my_req_procs,
                              count_my_req_per_proc, my_req,
                              nprocs, myrank, &count_others_req_procs,
                              &count_others_req_per_proc, &others_req);

    GPFSMPIO_T_CIO_SET_GET(r, 1, 1, GPFSMPIO_CIO_T_DEXCH, GPFSMPIO_CIO_T_OTHREQ);

    /* read data in sizes of no more than ADIOI_Coll_bufsize,
     * communicate, and fill user buf.
     */
    ADIOI_Read_and_exch(fd, buf, datatype, nprocs, myrank,
                        others_req, offset_list,
                        len_list, contig_access_count, min_st_offset,
                        fd_size, fd_start, fd_end, buf_idx, error_code);

    GPFSMPIO_T_CIO_SET_GET(r, 0, 1, GPFSMPIO_CIO_LAST, GPFSMPIO_CIO_T_DEXCH);
    GPFSMPIO_T_CIO_SET_GET(r, 0, 1, GPFSMPIO_CIO_LAST, GPFSMPIO_CIO_T_MPIO_CRW);
    GPFSMPIO_T_CIO_REPORT(0, fd, myrank, nprocs);

    /* free all memory allocated for collective I/O */
    if (gpfsmpio_tuneblocking) {
        ADIOI_GPFS_Free_my_req(nprocs, count_my_req_per_proc, my_req, buf_idx);
        ADIOI_GPFS_Free_others_req(nprocs, count_others_req_per_proc, others_req);
    } else {
        ADIOI_Free_my_req(nprocs, count_my_req_per_proc, my_req, buf_idx);
        ADIOI_Free_others_req(nprocs, count_others_req_per_proc, others_req);
    }

    ADIOI_Free(offset_list);
    ADIOI_Free(st_offsets);
    ADIOI_Free(fd_start);

  fn_exit:
#ifdef HAVE_STATUS_SET_BYTES
    MPI_Type_size_x(datatype, &size);
    bufsize = size * count;
    MPIR_Status_set_bytes(status, datatype, bufsize);
/* This is a temporary way of filling in status. The right way is to
   keep track of how much data was actually read and placed in buf
   during collective I/O. */
#endif

    fd->fp_sys_posn = -1;       /* set it to null. */
}

static void ADIOI_Read_and_exch(ADIO_File fd, void *buf, MPI_Datatype
                                datatype, int nprocs,
                                int myrank, ADIOI_Access
                                * others_req, ADIO_Offset * offset_list,
                                ADIO_Offset * len_list, MPI_Count contig_access_count,
                                ADIO_Offset min_st_offset, ADIO_Offset fd_size,
                                ADIO_Offset * fd_start, ADIO_Offset * fd_end,
                                MPI_Aint * buf_idx, int *error_code)
{
/* Read in sizes of no more than coll_bufsize, an info parameter.
   Send data to appropriate processes.
   Place recd. data in user buf.
   The idea is to reduce the amount of extra memory required for
   collective I/O. If all data were read all at once, which is much
   easier, it would require temp space more than the size of user_buf,
   which is often unacceptable. For example, to read a distributed
   array from a file, where each local array is 8Mbytes, requiring
   at least another 8Mbytes of temp space is unacceptable. */

    int i, m, ntimes, max_ntimes, buftype_is_contig;
    ADIO_Offset st_loc = -1, end_loc = -1, off, done, real_off, req_off;
    char *read_buf = NULL, *tmp_buf;
    MPI_Count *curr_offlen_ptr, *count, *send_size, *recv_size;
    MPI_Count *partial_send, *recd_from_proc, *start_pos;
    /* Not convinced end_loc-st_loc couldn't be > int, so make these offsets */
    ADIO_Offset real_size, size, for_curr_iter, for_next_iter;
    MPI_Count req_len;
    int flag, rank;
    MPI_Status status;
    ADIOI_Flatlist_node *flat_buf = NULL;
    MPI_Aint lb, buftype_extent;
    MPI_Aint coll_bufsize;
#ifdef RDCOLL_DEBUG
    int iii;
#endif
    *error_code = MPI_SUCCESS;  /* changed below if error */
    /* only I/O errors are currently reported */

/* calculate the number of reads of size coll_bufsize
   to be done by each process and the max among all processes.
   That gives the no. of communication phases as well.
   coll_bufsize is obtained from the hints object. */

    coll_bufsize = fd->hints->cb_buffer_size;

    /* grab some initial values for st_loc and end_loc */
    for (i = 0; i < nprocs; i++) {
        if (others_req[i].count) {
            st_loc = others_req[i].offsets[0];
            end_loc = others_req[i].offsets[0];
            break;
        }
    }

    /* now find the real values */
    for (i = 0; i < nprocs; i++)
        for (MPI_Count j = 0; j < others_req[i].count; j++) {
            st_loc = MPL_MIN(st_loc, others_req[i].offsets[j]);
            end_loc = MPL_MAX(end_loc, (others_req[i].offsets[j]
                                        + others_req[i].lens[j] - 1));
        }

    /* calculate ntimes, the number of times this process must perform I/O
     * operations in order to complete all the requests it has received.
     * the need for multiple I/O operations comes from the restriction that
     * we only use coll_bufsize bytes of memory for internal buffering.
     */
    if ((st_loc == -1) && (end_loc == -1)) {
        /* this process does no I/O. */
        ntimes = 0;
    } else {
        /* ntimes=ceiling_div(end_loc - st_loc + 1, coll_bufsize) */
        ntimes = (int) ((end_loc - st_loc + coll_bufsize) / coll_bufsize);
    }

    MPI_Allreduce(&ntimes, &max_ntimes, 1, MPI_INT, MPI_MAX, fd->comm);

    read_buf = fd->io_buf;

    curr_offlen_ptr = ADIOI_Calloc(nprocs, sizeof(*curr_offlen_ptr));
    /* its use is explained below. calloc initializes to 0. */

    count = ADIOI_Malloc(nprocs * sizeof(*count));
    /* to store count of how many off-len pairs per proc are satisfied
     * in an iteration. */

    partial_send = ADIOI_Calloc(nprocs, sizeof(*partial_send));
    /* if only a portion of the last off-len pair is sent to a process
     * in a particular iteration, the length sent is stored here.
     * calloc initializes to 0. */

    send_size = ADIOI_Malloc(nprocs * sizeof(*send_size));
    /* total size of data to be sent to each proc. in an iteration */

    recv_size = ADIOI_Malloc(nprocs * sizeof(*recv_size));
    /* total size of data to be recd. from each proc. in an iteration.
     * Of size nprocs so that I can use MPI_Alltoall later. */

    recd_from_proc = ADIOI_Calloc(nprocs, sizeof(*recd_from_proc));
    /* amount of data recd. so far from each proc. Used in
     * ADIOI_Fill_user_buffer. initialized to 0 here. */

    start_pos = ADIOI_Malloc(nprocs * sizeof(*start_pos));
    /* used to store the starting value of curr_offlen_ptr[i] in
     * this iteration */

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    if (!buftype_is_contig) {
        flat_buf = ADIOI_Flatten_and_find(datatype);
    }
    MPI_Type_get_extent(datatype, &lb, &buftype_extent);

    done = 0;
    off = st_loc;
    for_curr_iter = for_next_iter = 0;

    MPI_Comm_rank(fd->comm, &rank);

#ifdef PROFILE
    MPE_Log_event(14, 0, "end computation");
#endif

    for (m = 0; m < ntimes; m++) {
        /* read buf of size coll_bufsize (or less) */
        /* go through all others_req and check if any are satisfied
         * by the current read */

        /* since MPI guarantees that displacements in filetypes are in
         * monotonically nondecreasing order, I can maintain a pointer
         * (curr_offlen_ptr) to
         * current off-len pair for each process in others_req and scan
         * further only from there. There is still a problem of filetypes
         * such as:  (1, 2, 3 are not process nos. They are just numbers for
         * three chunks of data, specified by a filetype.)
         *
         * 1  -------!--
         * 2    -----!----
         * 3       --!-----
         *
         * where ! indicates where the current read_size limitation cuts
         * through the filetype.  I resolve this by reading up to !, but
         * filling the communication buffer only for 1. I copy the portion
         * left over for 2 into a tmp_buf for use in the next
         * iteration. i.e., 2 and 3 will be satisfied in the next
         * iteration. This simplifies filling in the user's buf at the
         * other end, as only one off-len pair with incomplete data
         * will be sent. I also don't need to send the individual
         * offsets and lens along with the data, as the data is being
         * sent in a particular order. */

        /* off = start offset in the file for the data actually read in
         * this iteration
         * size = size of data read corresponding to off
         * real_off = off minus whatever data was retained in memory from
         * previous iteration for cases like 2, 3 illustrated above
         * real_size = size plus the extra corresponding to real_off
         * req_off = off in file for a particular contiguous request
         * minus what was satisfied in previous iteration
         * req_size = size corresponding to req_off */

#ifdef PROFILE
        MPE_Log_event(13, 0, "start computation");
#endif
        size = MPL_MIN(coll_bufsize, end_loc - st_loc + 1 - done);
        real_off = off - for_curr_iter;
        real_size = size + for_curr_iter;

        for (i = 0; i < nprocs; i++)
            count[i] = send_size[i] = 0;
        for_next_iter = 0;

        for (i = 0; i < nprocs; i++) {
#ifdef RDCOLL_DEBUG
            DBG_FPRINTF(stderr, "rank %d, i %d, others_count %lld\n", rank, i, others_req[i].count);
#endif
            if (others_req[i].count) {
                start_pos[i] = curr_offlen_ptr[i];
                MPI_Count j;
                for (j = curr_offlen_ptr[i]; j < others_req[i].count; j++) {
                    if (partial_send[i]) {
                        /* this request may have been partially
                         * satisfied in the previous iteration. */
                        req_off = others_req[i].offsets[j] + partial_send[i];
                        req_len = others_req[i].lens[j] - partial_send[i];
                        partial_send[i] = 0;
                        /* modify the off-len pair to reflect this change */
                        others_req[i].offsets[j] = req_off;
                        others_req[i].lens[j] = req_len;
                    } else {
                        req_off = others_req[i].offsets[j];
                        req_len = others_req[i].lens[j];
                    }
                    if (req_off < real_off + real_size) {
                        count[i]++;
                        MPI_Aint addr;
                        /* 'mem_ptrs' is Count for the _c datatypes, but need aint here */
                        MPI_Get_address(read_buf + req_off - real_off, &addr);
                        others_req[i].mem_ptrs[j] = addr;
                        send_size[i] +=
                            (MPL_MIN(real_off + real_size - req_off, (ADIO_Offset) req_len));

                        if (real_off + real_size - req_off < req_len) {
                            partial_send[i] = (real_off + real_size - req_off);
                            if ((j + 1 < others_req[i].count) &&
                                (others_req[i].offsets[j + 1] < real_off + real_size)) {
                                /* this is the case illustrated in the
                                 * figure above. */
                                for_next_iter = MPL_MAX(for_next_iter,
                                                        real_off + real_size -
                                                        others_req[i].offsets[j + 1]);
                                /* max because it must cover requests
                                 * from different processes */
                            }
                            break;
                        }
                    } else
                        break;
                }
                curr_offlen_ptr[i] = j;
            }
        }

        flag = 0;
        for (i = 0; i < nprocs; i++)
            if (count[i])
                flag = 1;

#ifdef PROFILE
        MPE_Log_event(14, 0, "end computation");
#endif
        if (flag) {
            char round[50];
            snprintf(round, sizeof(round), "two-phase-round=%d", m);
            setenv("LIBIOLOG_EXTRA_INFO", round, 1);
            ADIO_ReadContig(fd, read_buf + for_curr_iter, size, MPI_BYTE,
                            ADIO_EXPLICIT_OFFSET, off, &status, error_code);
#ifdef RDCOLL_DEBUG
            DBG_FPRINTF(stderr, "\tread_coll: 700, data read [%lld] = ", (long long) size);
            for (iii = 0; iii < size && iii < 80; iii++) {
                DBGV_FPRINTF(stderr, "%3d,", *((unsigned char *) read_buf + for_curr_iter + iii));
            }
            DBG_FPRINTF(stderr, "\n");
#endif

            if (*error_code != MPI_SUCCESS)
                return;
        }

        for_curr_iter = for_next_iter;

#ifdef PROFILE
        MPE_Log_event(7, 0, "start communication");
#endif
        if (gpfsmpio_comm == 1)
            ADIOI_R_Exchange_data(fd, buf, flat_buf, offset_list, len_list,
                                  send_size, recv_size, count,
                                  start_pos, partial_send, recd_from_proc, nprocs,
                                  myrank,
                                  buftype_is_contig, contig_access_count,
                                  min_st_offset, fd_size, fd_start, fd_end,
                                  others_req, m, buftype_extent, buf_idx);
        else if (gpfsmpio_comm == 0) {
            ADIOI_R_Exchange_data_alltoallv(fd, buf, flat_buf, offset_list, len_list,
                                            send_size, recv_size, count,
                                            start_pos, partial_send, recd_from_proc, nprocs,
                                            myrank,
                                            buftype_is_contig, contig_access_count,
                                            min_st_offset, fd_size, fd_start, fd_end,
                                            others_req, m, buftype_extent, buf_idx);
        }
#ifdef PROFILE
        MPE_Log_event(8, 0, "end communication");
#endif

        if (for_next_iter) {
            tmp_buf = (char *) ADIOI_Malloc(for_next_iter);
            memcpy(tmp_buf, read_buf + real_size - for_next_iter, for_next_iter);
            ADIOI_Free(fd->io_buf);
            fd->io_buf = (char *) ADIOI_Malloc(for_next_iter + coll_bufsize);
            memcpy(fd->io_buf, tmp_buf, for_next_iter);
            read_buf = fd->io_buf;
            ADIOI_Free(tmp_buf);
        }

        off += size;
        done += size;
    }

    for (i = 0; i < nprocs; i++)
        count[i] = send_size[i] = 0;
#ifdef PROFILE
    MPE_Log_event(7, 0, "start communication");
#endif
    for (m = ntimes; m < max_ntimes; m++)
/* nothing to send, but check for recv. */

        if (gpfsmpio_comm == 1)
            ADIOI_R_Exchange_data(fd, buf, flat_buf, offset_list, len_list,
                                  send_size, recv_size, count,
                                  start_pos, partial_send, recd_from_proc, nprocs,
                                  myrank,
                                  buftype_is_contig, contig_access_count,
                                  min_st_offset, fd_size, fd_start, fd_end,
                                  others_req, m, buftype_extent, buf_idx);
        else /* strncmp(env_switch, "alltoall", 8) == 0 */ if (gpfsmpio_comm == 0)
            ADIOI_R_Exchange_data_alltoallv(fd, buf, flat_buf, offset_list, len_list,
                                            send_size, recv_size, count,
                                            start_pos, partial_send, recd_from_proc, nprocs,
                                            myrank,
                                            buftype_is_contig, contig_access_count,
                                            min_st_offset, fd_size, fd_start, fd_end,
                                            others_req, m, buftype_extent, buf_idx);

#ifdef PROFILE
    MPE_Log_event(8, 0, "end communication");
#endif

    ADIOI_Free(curr_offlen_ptr);
    ADIOI_Free(count);
    ADIOI_Free(partial_send);
    ADIOI_Free(send_size);
    ADIOI_Free(recv_size);
    ADIOI_Free(recd_from_proc);
    ADIOI_Free(start_pos);

    unsetenv("LIBIOLOG_EXTRA_INFO");
}

static void ADIOI_R_Exchange_data(ADIO_File fd, void *buf, ADIOI_Flatlist_node
                                  * flat_buf, ADIO_Offset * offset_list, ADIO_Offset
                                  * len_list, MPI_Count * send_size, MPI_Count * recv_size,
                                  MPI_Count * count, MPI_Count * start_pos,
                                  MPI_Count * partial_send, MPI_Count * recd_from_proc, int nprocs,
                                  int myrank, int buftype_is_contig, MPI_Count contig_access_count,
                                  ADIO_Offset min_st_offset, ADIO_Offset fd_size,
                                  ADIO_Offset * fd_start, ADIO_Offset * fd_end,
                                  ADIOI_Access * others_req, int iter, MPI_Aint buftype_extent,
                                  MPI_Aint * buf_idx)
{
    int i, j, nprocs_recv, nprocs_send;
    char **recv_buf = NULL;
    MPI_Request *requests;
    MPI_Datatype send_type;
    MPI_Status *statuses;

/* exchange send_size info so that each process knows how much to
   receive from whom and how much memory to allocate. */

    MPI_Alltoall(send_size, 1, MPI_COUNT, recv_size, 1, MPI_COUNT, fd->comm);

    nprocs_recv = 0;
    for (i = 0; i < nprocs; i++)
        if (recv_size[i])
            nprocs_recv++;

    nprocs_send = 0;
    for (i = 0; i < nprocs; i++)
        if (send_size[i])
            nprocs_send++;

    requests = (MPI_Request *)
        ADIOI_Malloc((nprocs_send + nprocs_recv + 1) * sizeof(MPI_Request));
/* +1 to avoid a 0-size malloc */

/* post recvs. if buftype_is_contig, data can be directly recd. into
   user buf at location given by buf_idx. else use recv_buf. */

#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5032, 0, NULL);
#endif

    if (buftype_is_contig) {
        j = 0;
        for (i = 0; i < nprocs; i++)
            if (recv_size[i]) {
                MPI_Irecv_c(((char *) buf) + buf_idx[i], recv_size[i],
                            MPI_BYTE, i, ADIOI_COLL_TAG(i, iter), fd->comm, requests + j);
                j++;
                buf_idx[i] += recv_size[i];
            }
    } else {
/* allocate memory for recv_buf and post receives */
        recv_buf = (char **) ADIOI_Malloc(nprocs * sizeof(char *));
        for (i = 0; i < nprocs; i++) {
            if (recv_size[i])
                recv_buf[i] = (char *) ADIOI_Malloc(recv_size[i]);
        }

        j = 0;
        for (i = 0; i < nprocs; i++) {
            if (recv_size[i]) {
                MPI_Irecv_c(recv_buf[i], recv_size[i], MPI_BYTE, i,
                            ADIOI_COLL_TAG(i, iter), fd->comm, requests + j);
                j++;
#ifdef RDCOLL_DEBUG
                DBG_FPRINTF(stderr, "node %d, recv_size %lld, tag %d \n",
                            myrank, recv_size[i], ADIOI_COLL_TAG(i, iter));
#endif
            }
        }
    }

/* create derived datatypes and send data */

    j = 0;
    for (i = 0; i < nprocs; i++) {
        if (send_size[i]) {
/* take care if the last off-len pair is a partial send */
            MPI_Count k = 0, tmp = 0;
            if (partial_send[i]) {
                k = start_pos[i] + count[i] - 1;
                tmp = others_req[i].lens[k];
                others_req[i].lens[k] = partial_send[i];
            }
            ADIOI_Type_create_hindexed_x(count[i],
                                         &(others_req[i].lens[start_pos[i]]),
                                         &(others_req[i].mem_ptrs[start_pos[i]]),
                                         MPI_BYTE, &send_type);
            /* absolute displacement; use MPI_BOTTOM in send */
            MPI_Type_commit(&send_type);
            MPI_Isend(MPI_BOTTOM, 1, send_type, i, ADIOI_COLL_TAG(i, iter),
                      fd->comm, requests + nprocs_recv + j);
            MPI_Type_free(&send_type);
            if (partial_send[i])
                others_req[i].lens[k] = tmp;
            j++;
        }
    }

#ifdef MPI_STATUSES_IGNORE
    statuses = MPI_STATUSES_IGNORE;
#else
    statuses = (MPI_Status *) ADIOI_Malloc((nprocs_send + nprocs_recv + 1) * sizeof(MPI_Status));
    /* +1 to avoid a 0-size malloc */
#endif

    /* wait on the receives */
    if (nprocs_recv) {
#ifdef NEEDS_MPI_TEST
        j = 0;
        while (!j)
            MPI_Testall(nprocs_recv, requests, &j, statuses);
#else
        MPI_Waitall(nprocs_recv, requests, statuses);
#endif

        /* if noncontiguous, to the copies from the recv buffers */
        if (!buftype_is_contig)
            ADIOI_Fill_user_buffer(fd, buf, flat_buf, recv_buf,
                                   offset_list, len_list, recv_size,
                                   requests, statuses, recd_from_proc,
                                   nprocs, contig_access_count,
                                   min_st_offset, fd_size, fd_start, fd_end, buftype_extent);
    }

    /* wait on the sends */
    MPI_Waitall(nprocs_send, requests + nprocs_recv, statuses + nprocs_recv);

#ifndef MPI_STATUSES_IGNORE
    ADIOI_Free(statuses);
#endif
    ADIOI_Free(requests);

    if (!buftype_is_contig) {
        for (i = 0; i < nprocs; i++)
            if (recv_size[i])
                ADIOI_Free(recv_buf[i]);
        ADIOI_Free(recv_buf);
    }
#ifdef AGGREGATION_PROFILE
    MPE_Log_event(5033, 0, NULL);
#endif
}

#define ADIOI_BUF_INCR                                                  \
    {                                                                   \
        while (buf_incr) {                                              \
            size_in_buf = MPL_MIN(buf_incr, flat_buf_sz);               \
            user_buf_idx += size_in_buf;                                \
            flat_buf_sz -= size_in_buf;                                 \
            if (!flat_buf_sz) {                                         \
                if (flat_buf_idx < (flat_buf->count - 1)) flat_buf_idx++; \
                else {                                                  \
                    flat_buf_idx = 0;                                   \
                    n_buftypes++;                                       \
                }                                                       \
                user_buf_idx = flat_buf->indices[flat_buf_idx] +        \
                    (ADIO_Offset)n_buftypes*(ADIO_Offset)buftype_extent; \
                flat_buf_sz = flat_buf->blocklens[flat_buf_idx];        \
            }                                                           \
            buf_incr -= size_in_buf;                                    \
        }                                                               \
    }


#define ADIOI_BUF_COPY                                                  \
    {                                                                   \
        while (size) {                                                  \
            size_in_buf = MPL_MIN(size, flat_buf_sz);                   \
            memcpy(((char *) buf) + user_buf_idx,                       \
                   &(recv_buf[p][recv_buf_idx[p]]), size_in_buf);       \
            recv_buf_idx[p] += size_in_buf; /* already tested (size_t)size_in_buf*/ \
            user_buf_idx += size_in_buf;                                \
            flat_buf_sz -= size_in_buf;                                 \
            if (!flat_buf_sz) {                                         \
                if (flat_buf_idx < (flat_buf->count - 1)) flat_buf_idx++; \
                else {                                                  \
                    flat_buf_idx = 0;                                   \
                    n_buftypes++;                                       \
                }                                                       \
                user_buf_idx = flat_buf->indices[flat_buf_idx] +        \
                    (ADIO_Offset)n_buftypes*(ADIO_Offset)buftype_extent; \
                flat_buf_sz = flat_buf->blocklens[flat_buf_idx];        \
            }                                                           \
            size -= size_in_buf;                                        \
            buf_incr -= size_in_buf;                                    \
        }                                                               \
        ADIOI_BUF_INCR                                                  \
    }

static void ADIOI_Fill_user_buffer(ADIO_File fd, void *buf, ADIOI_Flatlist_node
                                   * flat_buf, char **recv_buf, ADIO_Offset
                                   * offset_list, ADIO_Offset * len_list,
                                   MPI_Count * recv_size,
                                   MPI_Request * requests, MPI_Status * statuses,
                                   MPI_Count * recd_from_proc, int nprocs,
                                   MPI_Count contig_access_count,
                                   ADIO_Offset min_st_offset,
                                   ADIO_Offset fd_size, ADIO_Offset * fd_start,
                                   ADIO_Offset * fd_end, MPI_Aint buftype_extent)
{

/* this function is only called if buftype is not contig */

    int p, flat_buf_idx;
    ADIO_Offset flat_buf_sz, size_in_buf, buf_incr, size;
    int n_buftypes;
    ADIO_Offset off, len, rem_len, user_buf_idx;
    MPI_Count *curr_from_proc, *done_from_proc, *recv_buf_idx;

    MPL_UNREFERENCED_ARG(requests);
    MPL_UNREFERENCED_ARG(statuses);

/*  curr_from_proc[p] = amount of data recd from proc. p that has already
                        been accounted for so far
    done_from_proc[p] = amount of data already recd from proc. p and
                        filled into user buffer in previous iterations
    user_buf_idx = current location in user buffer
    recv_buf_idx[p] = current location in recv_buf of proc. p  */
    curr_from_proc = ADIOI_Malloc(nprocs * sizeof(*curr_from_proc));
    done_from_proc = ADIOI_Malloc(nprocs * sizeof(*done_from_proc));
    recv_buf_idx = ADIOI_Malloc(nprocs * sizeof(*recv_buf_idx));

    for (int i = 0; i < nprocs; i++) {
        recv_buf_idx[i] = curr_from_proc[i] = 0;
        done_from_proc[i] = recd_from_proc[i];
    }

    user_buf_idx = flat_buf->indices[0];
    flat_buf_idx = 0;
    n_buftypes = 0;
    flat_buf_sz = flat_buf->blocklens[0];

    /* flat_buf_idx = current index into flattened buftype
     * flat_buf_sz = size of current contiguous component in
     * flattened buf */

    for (MPI_Count i = 0; i < contig_access_count; i++) {
        off = offset_list[i];
        rem_len = len_list[i];

        /* this request may span the file domains of more than one process */
        while (rem_len > 0) {
            len = rem_len;
            /* NOTE: len value is modified by ADIOI_Calc_aggregator() to be no
             * longer than the single region that processor "p" is responsible
             * for.
             */
            p = ADIOI_GPFS_Calc_aggregator(fd, off, min_st_offset, &len, fd_size, fd_start, fd_end);

            if (recv_buf_idx[p] < recv_size[p]) {
                if (curr_from_proc[p] + len > done_from_proc[p]) {
                    if (done_from_proc[p] > curr_from_proc[p]) {
                        size = MPL_MIN(curr_from_proc[p] + len -
                                       done_from_proc[p], recv_size[p] - recv_buf_idx[p]);
                        buf_incr = done_from_proc[p] - curr_from_proc[p];
                        ADIOI_BUF_INCR buf_incr = curr_from_proc[p] + len - done_from_proc[p];
                        curr_from_proc[p] = done_from_proc[p] + size;
                    ADIOI_BUF_COPY} else {
                        size = MPL_MIN(len, recv_size[p] - recv_buf_idx[p]);
                        buf_incr = len;
                        curr_from_proc[p] += size;
                    ADIOI_BUF_COPY}
                } else {
                    curr_from_proc[p] += len;
                    buf_incr = len;
                ADIOI_BUF_INCR}
            } else {
                buf_incr = len;
            ADIOI_BUF_INCR}
            off += len;
            rem_len -= len;
        }
    }
    for (int i = 0; i < nprocs; i++)
        if (recv_size[i])
            recd_from_proc[i] = curr_from_proc[i];

    ADIOI_Free(curr_from_proc);
    ADIOI_Free(done_from_proc);
    ADIOI_Free(recv_buf_idx);
}

static void ADIOI_R_Exchange_data_alltoallv(ADIO_File fd, void *buf, ADIOI_Flatlist_node
                                            * flat_buf, ADIO_Offset * offset_list, ADIO_Offset
                                            * len_list, MPI_Count * send_size,
                                            MPI_Count * recv_size, MPI_Count * count,
                                            MPI_Count * start_pos, MPI_Count * partial_send,
                                            MPI_Count * recd_from_proc, int nprocs, int myrank,
                                            int buftype_is_contig, MPI_Count contig_access_count,
                                            ADIO_Offset min_st_offset, ADIO_Offset fd_size,
                                            ADIO_Offset * fd_start, ADIO_Offset * fd_end,
                                            ADIOI_Access * others_req, int iter,
                                            MPI_Aint buftype_extent, MPI_Aint * buf_idx)
{
    int i, j, nprocs_recv, nprocs_send;
    char **recv_buf = NULL;
    MPI_Request *requests = NULL;
    MPI_Status *statuses = NULL;
    MPI_Count rtail, stail;
    char *sbuf_ptr, *from_ptr;
    size_t len;                 /* for memcpy: must be size_t */
    MPI_Aint *sdispls, *rdispls;
    char *all_recv_buf, *all_send_buf;

    /* exchange send_size info so that each process knows how much to
     * receive from whom and how much memory to allocate. */
    MPI_Alltoall(send_size, 1, MPI_COUNT, recv_size, 1, MPI_COUNT, fd->comm);

    nprocs_recv = 0;
    for (i = 0; i < nprocs; i++)
        if (recv_size[i]) {
            nprocs_recv++;
            break;
        }

    nprocs_send = 0;
    for (i = 0; i < nprocs; i++)
        if (send_size[i]) {
            nprocs_send++;
            break;
        }

    /* receiver side data structures */
    rdispls = ADIOI_Malloc(nprocs * sizeof(*rdispls));
    rtail = 0;
    for (i = 0; i < nprocs; i++) {
        rdispls[i] = rtail;
        rtail += recv_size[i];
    }

    /* data buffer */
    all_recv_buf = (char *) ADIOI_Malloc(rtail);
    recv_buf = (char **) ADIOI_Malloc(nprocs * sizeof(char *));
    for (i = 0; i < nprocs; i++) {
        recv_buf[i] = all_recv_buf + rdispls[i];
    }

    /* sender side data structures */
    sdispls = ADIOI_Malloc(nprocs * sizeof(*sdispls));
    stail = 0;
    for (i = 0; i < nprocs; i++) {
        sdispls[i] = stail;
        stail += send_size[i];
    }

    /* data buffer */
    all_send_buf = (char *) ADIOI_Malloc(stail);
    for (i = 0; i < nprocs; i++) {
        if (send_size[i]) {
            MPI_Offset k = 0, tmp = 0;
            if (partial_send[i]) {
                k = start_pos[i] + count[i] - 1;
                tmp = others_req[i].lens[k];
                others_req[i].lens[k] = partial_send[i];
            }
            sbuf_ptr = all_send_buf + sdispls[i];
            for (j = 0; j < count[i]; j++) {
                from_ptr =
                    (char *) ADIOI_AINT_CAST_TO_VOID_PTR(others_req[i].mem_ptrs[start_pos[i] + j]);
                len = others_req[i].lens[start_pos[i] + j];
                memcpy(sbuf_ptr, from_ptr, len);
                sbuf_ptr += len;
            }
            if (partial_send[i])
                others_req[i].lens[k] = tmp;
        }
    }

#if RDCOLL_DEBUG
    DBG_FPRINTF(stderr, "\tsend_size = [%d]%2lld,", 0, send_size[0]);
    for (i = 1; i < nprocs; i++)
        if (send_size[i - 1] != send_size[i]) {
            DBG_FPRINTF(stderr, "\t\t[%d]%2lld,", i, send_size[i]);
        }
    DBG_FPRINTF(stderr, "\trecv_size =  [%d]%2lld,", 0, recv_size[0]);
    for (i = 1; i < nprocs; i++)
        if (recv_size[i - 1] != recv_size[i]) {
            DBG_FPRINTF(stderr, "\t\t[%d]%2lld,", i, recv_size[i]);
        }
    DBG_FPRINTF(stderr, "\tsdispls   =  [%d]%2ld,", 0, sdispls[0]);
    for (i = 1; i < nprocs; i++)
        if (sdispls[i - 1] != sdispls[i]) {
            DBG_FPRINTF(stderr, "\t\t[%d]%2ld,", i, sdispls[i]);
        }
    DBG_FPRINTF(stderr, "\trdispls   =  [%d]%2ld,", 0, rdispls[0]);
    for (i = 1; i < nprocs; i++)
        if (rdispls[i - 1] != rdispls[i]) {
            DBG_FPRINTF(stderr, "\t\t[%d]%2ld,", i, rdispls[i]);
        }
    DBG_FPRINTF(stderr, "\ttails = %4lld, %4lld\n", (long long) stail, (long long) rtail);
    if (nprocs_send) {
        DBG_FPRINTF(stderr, "\tall_send_buf =  [%d]%2d,", 0, all_send_buf[0]);
        /* someone at some point found it useful to look at the 128th kilobyte of data from each processor, but this segfaults in many situations if "all debugging" enabled */
        //for (i=1; i<nprocs; i++) if (all_send_buf[(i-1)*131072]!=all_send_buf[i*131072]){ DBG_FPRINTF(stderr, "\t\t[%d]%2d,", i, all_send_buf  [i*131072]); }
    }
#endif

    /* alltoallv */
    MPI_Alltoallv_c(all_send_buf, send_size, sdispls, MPI_BYTE,
                    all_recv_buf, recv_size, rdispls, MPI_BYTE, fd->comm);

#if 0
    DBG_FPRINTF(stderr, "\tall_recv_buf = ");
    for (i = 131072; i < 131073; i++) {
        DBG_FPRINTF(stderr, "%2d,", all_recv_buf[i]);
    }
    DBG_FPRINTF(stderr, "\n");
#endif

    /* unpack at the receiver side */
    if (nprocs_recv) {
        if (!buftype_is_contig)
            ADIOI_Fill_user_buffer(fd, buf, flat_buf, recv_buf, offset_list, len_list, recv_size, requests, statuses,   /* never used inside */
                                   recd_from_proc,
                                   nprocs, contig_access_count,
                                   min_st_offset, fd_size, fd_start, fd_end, buftype_extent);
        else {
            rtail = 0;
            for (i = 0; i < nprocs; i++)
                if (recv_size[i]) {
                    memcpy((char *) buf + buf_idx[i], all_recv_buf + rtail, recv_size[i]);
                    buf_idx[i] += recv_size[i];
                    rtail += recv_size[i];
                }
        }
    }

    ADIOI_Free(all_send_buf);
    ADIOI_Free(all_recv_buf);
    ADIOI_Free(recv_buf);
    ADIOI_Free(sdispls);
    ADIOI_Free(rdispls);
    return;
}
