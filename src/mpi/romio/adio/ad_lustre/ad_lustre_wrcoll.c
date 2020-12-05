/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "ad_lustre.h"
#include "adio_extern.h"


#ifdef HAVE_LUSTRE_LOCKAHEAD
/* in ad_lustre_lock.c */
void ADIOI_LUSTRE_lock_ahead_ioctl(ADIO_File fd,
                                   int avail_cb_nodes, ADIO_Offset next_offset, int *error_code);

/* Handle lock ahead.  If this write is outside our locked region, lock it now */
#define ADIOI_LUSTRE_WR_LOCK_AHEAD(fd,cb_nodes,offset,error_code)           \
if (fd->hints->fs_hints.lustre.lock_ahead_write) {                           \
    if (offset > fd->hints->fs_hints.lustre.lock_ahead_end_extent) {        \
        ADIOI_LUSTRE_lock_ahead_ioctl(fd, cb_nodes, offset, error_code);    \
    }                                                                       \
    else if (offset < fd->hints->fs_hints.lustre.lock_ahead_start_extent) { \
        ADIOI_LUSTRE_lock_ahead_ioctl(fd, cb_nodes, offset, error_code);    \
    }                                                                       \
}
#else
#define ADIOI_LUSTRE_WR_LOCK_AHEAD(fd,cb_nodes,offset,error_code)

#endif


/* prototypes of functions used for collective writes only. */
static void ADIOI_LUSTRE_Exch_and_write(ADIO_File fd, const void *buf,
                                        MPI_Datatype datatype, int nprocs,
                                        int myrank,
                                        ADIOI_Access * others_req,
                                        ADIOI_Access * my_req,
                                        ADIO_Offset * offset_list,
                                        ADIO_Offset * len_list,
                                        int contig_access_count,
                                        int *striping_info,
                                        ADIO_Offset ** buf_idx, int *error_code);
static void ADIOI_LUSTRE_Fill_send_buffer(ADIO_File fd, const void *buf,
                                          ADIOI_Flatlist_node * flat_buf,
                                          char **send_buf,
                                          ADIO_Offset * offset_list,
                                          ADIO_Offset * len_list, int *send_size,
                                          MPI_Request * requests,
                                          int *sent_to_proc, int nprocs,
                                          int myrank, int contig_access_count,
                                          int *striping_info,
                                          ADIO_Offset * send_buf_idx,
                                          int *curr_to_proc,
                                          int *done_to_proc, int iter, MPI_Aint buftype_extent);
static void ADIOI_LUSTRE_W_Exchange_data(ADIO_File fd, const void *buf,
                                         char *write_buf,
                                         ADIOI_Flatlist_node * flat_buf,
                                         ADIO_Offset * offset_list,
                                         ADIO_Offset * len_list, int *send_size,
                                         int *recv_size, ADIO_Offset off,
                                         int size, int *count,
                                         int *start_pos,
                                         int *sent_to_proc, int nprocs,
                                         int myrank, int buftype_is_contig,
                                         int contig_access_count,
                                         int *striping_info,
                                         ADIOI_Access * others_req,
                                         ADIO_Offset * send_buf_idx,
                                         int *curr_to_proc,
                                         int *done_to_proc, int *hole,
                                         int iter, MPI_Aint buftype_extent,
                                         ADIO_Offset * buf_idx,
                                         ADIO_Offset ** srt_off, int **srt_len, int *srt_num,
                                         int *error_code);
void ADIOI_Heap_merge(ADIOI_Access * others_req, int *count,
                      ADIO_Offset * srt_off, int *srt_len, int *start_pos,
                      int nprocs, int nprocs_recv, int total_elements);

static void ADIOI_LUSTRE_IterateOneSided(ADIO_File fd, const void *buf, int *striping_info,
                                         ADIO_Offset * offset_list, ADIO_Offset * len_list,
                                         int contig_access_count, int currentValidDataIndex,
                                         int count, int file_ptr_type, ADIO_Offset offset,
                                         ADIO_Offset start_offset, ADIO_Offset end_offset,
                                         ADIO_Offset firstFileOffset, ADIO_Offset lastFileOffset,
                                         MPI_Datatype datatype, int myrank, int *error_code);

void ADIOI_LUSTRE_WriteStridedColl(ADIO_File fd, const void *buf, int count,
                                   MPI_Datatype datatype,
                                   int file_ptr_type, ADIO_Offset offset,
                                   ADIO_Status * status, int *error_code)
{
    /* Uses a generalized version of the extended two-phase method described
     * in "An Extended Two-Phase Method for Accessing Sections of
     * Out-of-Core Arrays", Rajeev Thakur and Alok Choudhary,
     * Scientific Programming, (5)4:301--317, Winter 1996.
     * http://www.mcs.anl.gov/home/thakur/ext2ph.ps
     */

    ADIOI_Access *my_req;
    /* array of nprocs access structures, one for each other process has
     * this process's request */

    ADIOI_Access *others_req;
    /* array of nprocs access structures, one for each other process
     * whose request is written by this process. */

    int i, filetype_is_contig, nprocs, myrank, do_collect = 0;
    int contig_access_count = 0, buftype_is_contig, interleave_count = 0;
    int *count_my_req_per_proc, count_my_req_procs, count_others_req_procs;
    ADIO_Offset orig_fp, start_offset, end_offset, off;
    ADIO_Offset *offset_list = NULL, *st_offsets = NULL, *end_offsets = NULL;
    ADIO_Offset *len_list = NULL;
    int striping_info[3];
    ADIO_Offset **buf_idx = NULL;
    int old_error, tmp_error;
    ADIO_Offset *lustre_offsets0, *lustre_offsets, *count_sizes = NULL;

    MPI_Comm_size(fd->comm, &nprocs);
    MPI_Comm_rank(fd->comm, &myrank);

    orig_fp = fd->fp_ind;

    /* IO patten identification if cb_write isn't disabled */
    if (fd->hints->cb_write != ADIOI_HINT_DISABLE) {
        /* For this process's request, calculate the list of offsets and
         * lengths in the file and determine the start and end offsets.
         * Note: end_offset points to the last byte-offset to be accessed.
         * e.g., if start_offset=0 and 100 bytes to be read, end_offset=99
         */
        ADIOI_Calc_my_off_len(fd, count, datatype, file_ptr_type, offset,
                              &offset_list, &len_list, &start_offset,
                              &end_offset, &contig_access_count);

        /* each process communicates its start and end offsets to other
         * processes. The result is an array each of start and end offsets
         * stored in order of process rank.
         */
        st_offsets = (ADIO_Offset *) ADIOI_Malloc(nprocs * 2 * sizeof(ADIO_Offset));
        end_offsets = st_offsets + nprocs;
        ADIO_Offset my_count_size = 0;
        /* One-sided aggregation needs the amount of data per rank as well
         * because the difference in starting and ending offsets for 1 byte is
         * 0 the same as 0 bytes so it cannot be distiguished.
         */
        if ((fd->romio_write_aggmethod == 1) || (fd->romio_write_aggmethod == 2)) {
            count_sizes = (ADIO_Offset *) ADIOI_Malloc(nprocs * sizeof(ADIO_Offset));
            MPI_Count buftype_size;
            MPI_Type_size_x(datatype, &buftype_size);
            my_count_size = (ADIO_Offset) count *(ADIO_Offset) buftype_size;
        }
        if (fd->romio_tunegather) {
            if ((fd->romio_write_aggmethod == 1) || (fd->romio_write_aggmethod == 2)) {
                lustre_offsets0 = (ADIO_Offset *) ADIOI_Malloc(6 * nprocs * sizeof(ADIO_Offset));
                lustre_offsets = lustre_offsets0 + 3 * nprocs;
                for (i = 0; i < nprocs; i++) {
                    lustre_offsets0[i * 3] = 0;
                    lustre_offsets0[i * 3 + 1] = 0;
                    lustre_offsets0[i * 3 + 2] = 0;
                }
                lustre_offsets0[myrank * 3] = start_offset;
                lustre_offsets0[myrank * 3 + 1] = end_offset;
                lustre_offsets0[myrank * 3 + 2] = my_count_size;
                MPI_Allreduce(lustre_offsets0, lustre_offsets, nprocs * 3, ADIO_OFFSET, MPI_MAX,
                              fd->comm);
                for (i = 0; i < nprocs; i++) {
                    st_offsets[i] = lustre_offsets[i * 3];
                    end_offsets[i] = lustre_offsets[i * 3 + 1];
                    count_sizes[i] = lustre_offsets[i * 3 + 2];
                }
            } else {
                lustre_offsets0 = (ADIO_Offset *) ADIOI_Malloc(4 * nprocs * sizeof(ADIO_Offset));
                lustre_offsets = lustre_offsets0 + 2 * nprocs;
                for (i = 0; i < nprocs; i++) {
                    lustre_offsets0[i * 2] = 0;
                    lustre_offsets0[i * 2 + 1] = 0;
                }
                lustre_offsets0[myrank * 2] = start_offset;
                lustre_offsets0[myrank * 2 + 1] = end_offset;

                MPI_Allreduce(lustre_offsets0, lustre_offsets, nprocs * 2, ADIO_OFFSET, MPI_MAX,
                              fd->comm);

                for (i = 0; i < nprocs; i++) {
                    st_offsets[i] = lustre_offsets[i * 2];
                    end_offsets[i] = lustre_offsets[i * 2 + 1];
                }
            }
            ADIOI_Free(lustre_offsets0);
        } else {
            MPI_Allgather(&start_offset, 1, ADIO_OFFSET, st_offsets, 1, ADIO_OFFSET, fd->comm);
            MPI_Allgather(&end_offset, 1, ADIO_OFFSET, end_offsets, 1, ADIO_OFFSET, fd->comm);
            if ((fd->romio_write_aggmethod == 1) || (fd->romio_write_aggmethod == 2)) {
                MPI_Allgather(&my_count_size, 1, ADIO_OFFSET, count_sizes, 1,
                              ADIO_OFFSET, fd->comm);
            }
        }
        /* are the accesses of different processes interleaved? */
        for (i = 1; i < nprocs; i++)
            if ((st_offsets[i] < end_offsets[i - 1]) && (st_offsets[i] <= end_offsets[i]))
                interleave_count++;
        /* This is a rudimentary check for interleaving, but should suffice
         * for the moment. */

        /* Two typical access patterns can benefit from collective write.
         *   1) the processes are interleaved, and
         *   2) the req size is small.
         */
        if (interleave_count > 0) {
            do_collect = 1;
        } else {
            do_collect = ADIOI_LUSTRE_Docollect(fd, contig_access_count, len_list, nprocs);
        }
    }
    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);

    /* Decide if collective I/O should be done */
    if ((!do_collect && fd->hints->cb_write == ADIOI_HINT_AUTO) ||
        fd->hints->cb_write == ADIOI_HINT_DISABLE) {

        /* use independent accesses */
        if (fd->hints->cb_write != ADIOI_HINT_DISABLE) {
            ADIOI_Free(offset_list);
            ADIOI_Free(st_offsets);
            if ((fd->romio_write_aggmethod == 1) || (fd->romio_write_aggmethod == 2))
                ADIOI_Free(count_sizes);
        }

        fd->fp_ind = orig_fp;
        ADIOI_Datatype_iscontig(fd->filetype, &filetype_is_contig);
        if (buftype_is_contig && filetype_is_contig) {
            if (file_ptr_type == ADIO_EXPLICIT_OFFSET) {
                off = fd->disp + (ADIO_Offset) (fd->etype_size) * offset;
                ADIO_WriteContig(fd, buf, count, datatype,
                                 ADIO_EXPLICIT_OFFSET, off, status, error_code);
            } else
                ADIO_WriteContig(fd, buf, count, datatype, ADIO_INDIVIDUAL, 0, status, error_code);
        } else {
            ADIO_WriteStrided(fd, buf, count, datatype, file_ptr_type, offset, status, error_code);
        }
        return;
    }

    ADIO_Offset lastFileOffset = 0, firstFileOffset = -1;
    int currentValidDataIndex = 0;
    if ((fd->romio_write_aggmethod == 1) || (fd->romio_write_aggmethod == 2)) {
        /* Take out the 0-data offsets by shifting the indexes with data to the front
         * and keeping track of the valid data index for use as the length.
         */
        for (i = 0; i < nprocs; i++) {
            if (count_sizes[i] > 0) {
                st_offsets[currentValidDataIndex] = st_offsets[i];
                end_offsets[currentValidDataIndex] = end_offsets[i];

                lastFileOffset = MPL_MAX(lastFileOffset, end_offsets[currentValidDataIndex]);
                if (firstFileOffset == -1)
                    firstFileOffset = st_offsets[currentValidDataIndex];
                else
                    firstFileOffset = MPL_MIN(firstFileOffset, st_offsets[currentValidDataIndex]);

                currentValidDataIndex++;
            }
        }
    }

    /* Get Lustre hints information */
    ADIOI_LUSTRE_Get_striping_info(fd, striping_info, 1);
    /* If the user has specified to use a one-sided aggregation method then do
     * that at this point instead of the two-phase I/O.
     */
    if ((fd->romio_write_aggmethod == 1) || (fd->romio_write_aggmethod == 2)) {

        ADIOI_LUSTRE_IterateOneSided(fd, buf, striping_info, offset_list, len_list,
                                     contig_access_count, currentValidDataIndex, count,
                                     file_ptr_type, offset, start_offset, end_offset,
                                     firstFileOffset, lastFileOffset, datatype, myrank, error_code);

        ADIOI_Free(offset_list);
        ADIOI_Free(st_offsets);
        ADIOI_Free(count_sizes);
        goto fn_exit;
    }   // onesided aggregation

    /* calculate what portions of the access requests of this process are
     * located in which process
     */
    ADIOI_LUSTRE_Calc_my_req(fd, offset_list, len_list, contig_access_count,
                             striping_info, nprocs, &count_my_req_procs,
                             &count_my_req_per_proc, &my_req, &buf_idx);

    /* based on everyone's my_req, calculate what requests of other processes
     * will be accessed by this process.
     * count_others_req_procs = number of processes whose requests (including
     * this process itself) will be accessed by this process
     * count_others_req_per_proc[i] indicates how many separate contiguous
     * requests of proc. i will be accessed by this process.
     */

    ADIOI_Calc_others_req(fd, count_my_req_procs, count_my_req_per_proc,
                          my_req, nprocs, myrank, &count_others_req_procs, &others_req);
    ADIOI_Free(count_my_req_per_proc);

    /* exchange data and write in sizes of no more than stripe_size. */
    ADIOI_LUSTRE_Exch_and_write(fd, buf, datatype, nprocs, myrank,
                                others_req, my_req, offset_list, len_list,
                                contig_access_count, striping_info, buf_idx, error_code);

    /* If this collective write is followed by an independent write,
     * it's possible to have those subsequent writes on other processes
     * race ahead and sneak in before the read-modify-write completes.
     * We carry out a collective communication at the end here so no one
     * can start independent i/o before collective I/O completes.
     *
     * need to do some gymnastics with the error codes so that if something
     * went wrong, all processes report error, but if a process has a more
     * specific error code, we can still have that process report the
     * additional information */

    old_error = *error_code;
    if (*error_code != MPI_SUCCESS)
        *error_code = MPI_ERR_IO;

    /* optimization: if only one process performing i/o, we can perform
     * a less-expensive Bcast  */
#ifdef ADIOI_MPE_LOGGING
    MPE_Log_event(ADIOI_MPE_postwrite_a, 0, NULL);
#endif
    if (fd->hints->cb_nodes == 1)
        MPI_Bcast(error_code, 1, MPI_INT, fd->hints->ranklist[0], fd->comm);
    else {
        tmp_error = *error_code;
        MPI_Allreduce(&tmp_error, error_code, 1, MPI_INT, MPI_MAX, fd->comm);
    }
#ifdef ADIOI_MPE_LOGGING
    MPE_Log_event(ADIOI_MPE_postwrite_b, 0, NULL);
#endif

    if ((old_error != MPI_SUCCESS) && (old_error != MPI_ERR_IO))
        *error_code = old_error;

    /* free all memory allocated for collective I/O */
    /* free others_req */
    ADIOI_Free(others_req[0].offsets);
    ADIOI_Free(others_req[0].mem_ptrs);
    ADIOI_Free(others_req);
    ADIOI_Free(buf_idx[0]);     /* also my_req[*].offsets and my_req[*].lens */
    ADIOI_Free(buf_idx);
    ADIOI_Free(my_req);
    ADIOI_Free(offset_list);
    ADIOI_Free(st_offsets);

  fn_exit:
#ifdef HAVE_STATUS_SET_BYTES
    if (status) {
        MPI_Count bufsize, size;
        /* Don't set status if it isn't needed */
        MPI_Type_size_x(datatype, &size);
        bufsize = size * count;
        MPIR_Status_set_bytes(status, datatype, bufsize);
    }
    /* This is a temporary way of filling in status. The right way is to
     * keep track of how much data was actually written during collective I/O.
     */
#endif

    fd->fp_sys_posn = -1;       /* set it to null. */
}

/* If successful, error_code is set to MPI_SUCCESS.  Otherwise an error
 * code is created and returned in error_code.
 */
static void ADIOI_LUSTRE_Exch_and_write(ADIO_File fd, const void *buf,
                                        MPI_Datatype datatype, int nprocs,
                                        int myrank, ADIOI_Access * others_req,
                                        ADIOI_Access * my_req,
                                        ADIO_Offset * offset_list,
                                        ADIO_Offset * len_list,
                                        int contig_access_count,
                                        int *striping_info, ADIO_Offset ** buf_idx, int *error_code)
{
    /* Send data to appropriate processes and write in sizes of no more
     * than lustre stripe_size.
     * The idea is to reduce the amount of extra memory required for
     * collective I/O. If all data were written all at once, which is much
     * easier, it would require temp space more than the size of user_buf,
     * which is often unacceptable. For example, to write a distributed
     * array to a file, where each local array is 8Mbytes, requiring
     * at least another 8Mbytes of temp space is unacceptable.
     */

    int hole, i, j, m, flag, ntimes = 1, max_ntimes, buftype_is_contig;
    ADIO_Offset st_loc = -1, end_loc = -1, min_st_loc, max_end_loc;
    ADIO_Offset off, req_off, send_off, iter_st_off, *off_list;
    ADIO_Offset max_size, step_size = 0;
    int real_size, req_len, send_len;
    int *recv_curr_offlen_ptr, *recv_count, *recv_size;
    int *send_curr_offlen_ptr, *send_size;
    int *sent_to_proc, *recv_start_pos;
    int *curr_to_proc, *done_to_proc;
    ADIO_Offset *send_buf_idx, *this_buf_idx;
    char *write_buf = NULL;
    MPI_Status status;
    ADIOI_Flatlist_node *flat_buf = NULL;
    MPI_Aint buftype_extent;
    int stripe_size = striping_info[0], avail_cb_nodes = striping_info[2];
    int data_sieving = 0;
    ADIO_Offset *srt_off = NULL;
    int *srt_len = NULL;
    int srt_num = 0;
    ADIO_Offset block_offset;
    int block_len;

    *error_code = MPI_SUCCESS;  /* changed below if error */
    /* only I/O errors are currently reported */

    /* calculate the number of writes of stripe size to be done.
     * That gives the no. of communication phases as well.
     * Note:
     *   Because we redistribute data in stripe-contiguous pattern for Lustre,
     *   each process has the same no. of communication phases.
     */

    for (i = 0; i < nprocs; i++) {
        if (others_req[i].count) {
            st_loc = others_req[i].offsets[0];
            end_loc = others_req[i].offsets[0];
            break;
        }
    }
    for (i = 0; i < nprocs; i++) {
        for (j = 0; j < others_req[i].count; j++) {
            st_loc = MPL_MIN(st_loc, others_req[i].offsets[j]);
            end_loc = MPL_MAX(end_loc, (others_req[i].offsets[j] + others_req[i].lens[j] - 1));
        }
    }
    /* this process does no writing. */
    if ((st_loc == -1) && (end_loc == -1))
        ntimes = 0;
    MPI_Allreduce(&end_loc, &max_end_loc, 1, MPI_LONG_LONG_INT, MPI_MAX, fd->comm);
    /* avoid min_st_loc be -1 */
    if (st_loc == -1)
        st_loc = max_end_loc;
    MPI_Allreduce(&st_loc, &min_st_loc, 1, MPI_LONG_LONG_INT, MPI_MIN, fd->comm);
    /* align downward */
    min_st_loc -= min_st_loc % (ADIO_Offset) stripe_size;

    /* Each time, only avail_cb_nodes number of IO clients perform IO,
     * so, step_size=avail_cb_nodes*stripe_size IO will be performed at most,
     * and ntimes=whole_file_portion/step_size
     */
    step_size = (ADIO_Offset) avail_cb_nodes *stripe_size;
    max_ntimes = (max_end_loc - min_st_loc + 1) / step_size
        + (((max_end_loc - min_st_loc + 1) % step_size) ? 1 : 0);
/*     max_ntimes = (int)((max_end_loc - min_st_loc) / step_size + 1); */
    if (ntimes)
        write_buf = (char *) ADIOI_Malloc(stripe_size);

    /* calculate the start offset for each iteration */
    off_list = (ADIO_Offset *) ADIOI_Malloc((max_ntimes + 2 * nprocs) * sizeof(ADIO_Offset));
    send_buf_idx = off_list + max_ntimes;
    this_buf_idx = send_buf_idx + nprocs;

    for (m = 0; m < max_ntimes; m++)
        off_list[m] = max_end_loc;
    for (i = 0; i < nprocs; i++) {
        for (j = 0; j < others_req[i].count; j++) {
            req_off = others_req[i].offsets[j];
            m = (int) ((req_off - min_st_loc) / step_size);
            off_list[m] = MPL_MIN(off_list[m], req_off);
        }
    }

    recv_curr_offlen_ptr = (int *) ADIOI_Calloc(nprocs * 9, sizeof(int));
    send_curr_offlen_ptr = recv_curr_offlen_ptr + nprocs;
    /* their use is explained below. calloc initializes to 0. */

    recv_count = send_curr_offlen_ptr + nprocs;
    /* to store count of how many off-len pairs per proc are satisfied
     * in an iteration. */

    send_size = recv_count + nprocs;
    /* total size of data to be sent to each proc. in an iteration.
     * Of size nprocs so that I can use MPI_Alltoall later. */

    recv_size = send_size + nprocs;
    /* total size of data to be recd. from each proc. in an iteration. */

    sent_to_proc = recv_size + nprocs;
    /* amount of data sent to each proc so far. Used in
     * ADIOI_Fill_send_buffer. initialized to 0 here. */

    curr_to_proc = sent_to_proc + nprocs;
    done_to_proc = curr_to_proc + nprocs;
    /* Above three are used in ADIOI_Fill_send_buffer */

    recv_start_pos = done_to_proc + nprocs;
    /* used to store the starting value of recv_curr_offlen_ptr[i] in
     * this iteration */

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    if (!buftype_is_contig) {
        flat_buf = ADIOI_Flatten_and_find(datatype);
    }
    MPI_Type_extent(datatype, &buftype_extent);
    /* I need to check if there are any outstanding nonblocking writes to
     * the file, which could potentially interfere with the writes taking
     * place in this collective write call. Since this is not likely to be
     * common, let me do the simplest thing possible here: Each process
     * completes all pending nonblocking operations before completing.
     */
    /*ADIOI_Complete_async(error_code);
     * if (*error_code != MPI_SUCCESS) return;
     * MPI_Barrier(fd->comm);
     */

    iter_st_off = min_st_loc;

    /* Although we have recognized the data according to OST index,
     * a read-modify-write will be done if there is a hole between the data.
     * For example: if blocksize=60, xfersize=30 and stripe_size=100,
     * then rank0 will collect data [0, 30] and [60, 90] then write. There
     * is a hole in [30, 60], which will cause a read-modify-write in [0, 90].
     *
     * To reduce its impact on the performance, we can disable data sieving
     * by hint "ds_in_coll".
     */
    /* check the hint for data sieving */
    data_sieving = fd->hints->fs_hints.lustre.ds_in_coll;

    for (m = 0; m < max_ntimes; m++) {
        /* go through all others_req and my_req to check which will be received
         * and sent in this iteration.
         */

        /* Note that MPI guarantees that displacements in filetypes are in
         * monotonically nondecreasing order and that, for writes, the
         * filetypes cannot specify overlapping regions in the file. This
         * simplifies implementation a bit compared to reads. */

        /*
         * off         = start offset in the file for the data to be written in
         * this iteration
         * iter_st_off = start offset of this iteration
         * real_size   = size of data written (bytes) corresponding to off
         * max_size    = possible maximum size of data written in this iteration
         * req_off     = offset in the file for a particular contiguous request minus
         * what was satisfied in previous iteration
         * send_off    = offset the request needed by other processes in this iteration
         * req_len     = size corresponding to req_off
         * send_len    = size corresponding to send_off
         */

        /* first calculate what should be communicated */
        for (i = 0; i < nprocs; i++)
            recv_count[i] = recv_size[i] = send_size[i] = 0;

        off = off_list[m];
        max_size = MPL_MIN(step_size, max_end_loc - iter_st_off + 1);
        real_size = (int) MPL_MIN((off / stripe_size + 1) * stripe_size - off, end_loc - off + 1);

        for (i = 0; i < nprocs; i++) {
            if (my_req[i].count) {
                this_buf_idx[i] = buf_idx[i][send_curr_offlen_ptr[i]];
                for (j = send_curr_offlen_ptr[i]; j < my_req[i].count; j++) {
                    send_off = my_req[i].offsets[j];
                    send_len = my_req[i].lens[j];
                    if (send_off < iter_st_off + max_size) {
                        send_size[i] += send_len;
                    } else {
                        break;
                    }
                }
                send_curr_offlen_ptr[i] = j;
            }
            if (others_req[i].count) {
                recv_start_pos[i] = recv_curr_offlen_ptr[i];
                for (j = recv_curr_offlen_ptr[i]; j < others_req[i].count; j++) {
                    req_off = others_req[i].offsets[j];
                    req_len = others_req[i].lens[j];
                    if (req_off < iter_st_off + max_size) {
                        recv_count[i]++;
                        ADIOI_Assert((((ADIO_Offset) (uintptr_t) write_buf) + req_off - off) ==
                                     (ADIO_Offset) (uintptr_t) (write_buf + req_off - off));
                        MPI_Address(write_buf + req_off - off, &(others_req[i].mem_ptrs[j]));
                        recv_size[i] += req_len;
                    } else {
                        break;
                    }
                }
                recv_curr_offlen_ptr[i] = j;
            }
        }
        /* use variable "hole" to pass data_sieving flag into W_Exchange_data */
        hole = data_sieving;
        ADIOI_LUSTRE_W_Exchange_data(fd, buf, write_buf, flat_buf, offset_list,
                                     len_list, send_size, recv_size, off, real_size,
                                     recv_count, recv_start_pos,
                                     sent_to_proc, nprocs, myrank,
                                     buftype_is_contig, contig_access_count,
                                     striping_info, others_req, send_buf_idx,
                                     curr_to_proc, done_to_proc, &hole, m,
                                     buftype_extent, this_buf_idx,
                                     &srt_off, &srt_len, &srt_num, error_code);

        if (*error_code != MPI_SUCCESS)
            goto over;

        flag = 0;
        for (i = 0; i < nprocs; i++)
            if (recv_count[i]) {
                flag = 1;
                break;
            }
        if (flag) {
            /* check whether to do data sieving */
            if (data_sieving == ADIOI_HINT_ENABLE) {
                ADIOI_LUSTRE_WR_LOCK_AHEAD(fd, striping_info[2], off, error_code);
                ADIO_WriteContig(fd, write_buf, real_size, MPI_BYTE,
                                 ADIO_EXPLICIT_OFFSET, off, &status, error_code);
            } else {
                /* if there is no hole, write data in one time;
                 * otherwise, write data in several times */
                if (!hole) {
                    ADIOI_LUSTRE_WR_LOCK_AHEAD(fd, striping_info[2], off, error_code);
                    ADIO_WriteContig(fd, write_buf, real_size, MPI_BYTE,
                                     ADIO_EXPLICIT_OFFSET, off, &status, error_code);
                } else {
                    block_offset = -1;
                    block_len = 0;
                    for (i = 0; i < srt_num; ++i) {
                        if (srt_off[i] < off + real_size && srt_off[i] >= off) {
                            if (block_offset == -1) {
                                block_offset = srt_off[i];
                                block_len = srt_len[i];
                            } else {
                                if (srt_off[i] == block_offset + block_len) {
                                    block_len += srt_len[i];
                                } else {
                                    ADIOI_LUSTRE_WR_LOCK_AHEAD(fd, striping_info[2], block_offset,
                                                               error_code);
                                    ADIO_WriteContig(fd, write_buf + block_offset - off, block_len,
                                                     MPI_BYTE, ADIO_EXPLICIT_OFFSET, block_offset,
                                                     &status, error_code);
                                    if (*error_code != MPI_SUCCESS)
                                        goto over;
                                    block_offset = srt_off[i];
                                    block_len = srt_len[i];
                                }
                            }
                        }
                    }
                    if (block_offset != -1) {
                        ADIOI_LUSTRE_WR_LOCK_AHEAD(fd, striping_info[2], block_offset, error_code);
                        ADIO_WriteContig(fd,
                                         write_buf + block_offset - off,
                                         block_len,
                                         MPI_BYTE, ADIO_EXPLICIT_OFFSET,
                                         block_offset, &status, error_code);
                        if (*error_code != MPI_SUCCESS)
                            goto over;
                    }
                }
            }
            if (*error_code != MPI_SUCCESS)
                goto over;
        }
        iter_st_off += max_size;
    }
  over:
    if (srt_off)
        ADIOI_Free(srt_off);
    if (srt_len)
        ADIOI_Free(srt_len);
    if (ntimes)
        ADIOI_Free(write_buf);
    ADIOI_Free(recv_curr_offlen_ptr);
    ADIOI_Free(off_list);
}

/* Sets error_code to MPI_SUCCESS if successful, or creates an error code
 * in the case of error.
 */
static void ADIOI_LUSTRE_W_Exchange_data(ADIO_File fd, const void *buf,
                                         char *write_buf,
                                         ADIOI_Flatlist_node * flat_buf,
                                         ADIO_Offset * offset_list,
                                         ADIO_Offset * len_list, int *send_size,
                                         int *recv_size, ADIO_Offset real_off,
                                         int real_size, int *recv_count,
                                         int *start_pos,
                                         int *sent_to_proc, int nprocs,
                                         int myrank, int buftype_is_contig,
                                         int contig_access_count,
                                         int *striping_info,
                                         ADIOI_Access * others_req,
                                         ADIO_Offset * send_buf_idx,
                                         int *curr_to_proc, int *done_to_proc,
                                         int iter,
                                         MPI_Aint buftype_extent,
                                         ADIO_Offset * buf_idx,
                                         ADIO_Offset ** srt_off, int **srt_len, int *srt_num,
                                         int *error_code)
{
    char *buf_ptr, *contig_buf, **send_buf = NULL, *send_buf_start;
    int i, j, k, nprocs_recv, nprocs_send, err;
    int sum_recv, nreqs, tag, hole, check_hole;
    size_t send_total_size;
    MPI_Aint local_data_size;
    static size_t malloc_srt_num = 0;
    MPI_Request *requests = NULL;
    MPI_Status status;
    /* Requests for TAM */
    MPI_Request *req = fd->req;
    MPI_Status *sts = fd->sts;
    static char myname[] = "ADIOI_LUSTRE_W_EXCHANGE_DATA";

    /* calculate send receive metadata */
    *srt_num = 0;
    sum_recv = 0;
    nprocs_recv = 0;
    nprocs_send = 0;
    for (i = 0; i < nprocs; i++) {
        *srt_num += recv_count[i];
        sum_recv += recv_size[i];
        if (recv_size[i])
            nprocs_recv++;
        if (send_size[i])
            nprocs_send++;
    }

    /* determine whether checking holes is necessary */
    check_hole = 1;
    if (*srt_num == 0) {
        /* this process has nothing to receive and hence no hole */
        check_hole = 0;
        hole = 0;
    } else if (fd->hints->ds_write == ADIOI_HINT_AUTO) {
        if (*srt_num > fd->hints->ds_wr_lb) {
            /* Number of offset-length pairs is too large, making merge sort
             * expensive. Skip the sorting in hole checking and proceed with
             * read-modify-write.
             */
            check_hole = 0;
            hole = 1;
        }
        /* else: merge sort is less expensive, proceed to check_hole */
    }
    /* else: fd->hints->ds_write == ADIOI_HINT_ENABLE or ADIOI_HINT_DISABLE,
     * proceed to check_hole, as we must construct srt_off and srt_len.
     */

    if (check_hole) {
        /* merge the offset-length pairs of all others_req[] (already sorted
         * individually) into a single list of offset-length pairs (srt_off and
         * srt_len) in an increasing order of file offsets using a heap-merge
         * sorting algorithm.
         */
        if (*srt_off == NULL || *srt_num > malloc_srt_num) {
            /* Try to avoid malloc each round. If *srt_num is less than
             * previous round, the already allocated space can be reused.
             */
            if (*srt_off != NULL) {
                ADIOI_Free(*srt_off);
                ADIOI_Free(*srt_len);
            }
            *srt_off = (ADIO_Offset *) ADIOI_Malloc(*srt_num * sizeof(ADIO_Offset));
            *srt_len = (int *) ADIOI_Malloc(*srt_num * sizeof(int));
            malloc_srt_num = *srt_num;
        }

        heap_merge(others_req, recv_count, *srt_off, *srt_len, start_pos,
                   nprocs, nprocs_recv, srt_num);

        /* (*srt_num) has been updated in heap_merge() such that (*srt_off) and
         * (*srt_len) were coalesced
         */
        hole = (*srt_num > 1);
    }

    /* data sieving */
    if (fd->hints->ds_write != ADIOI_HINT_DISABLE && hole) {
        ADIO_ReadContig(fd, write_buf, real_size, MPI_BYTE, ADIO_EXPLICIT_OFFSET,
                        real_off, &status, &err);
        if (err != MPI_SUCCESS) {
            *error_code = MPIO_Err_create_code(err,
                                               MPIR_ERR_RECOVERABLE,
                                               myname, __LINE__, MPI_ERR_IO, "**ioRMWrdwr", 0);
            return;
        }

        /* Once read, holes have been filled and thus the number of
         * offset-length pairs, *srt_num, becomes one.
         */
        *srt_num = 1;
        if (*srt_off == NULL) {
            *srt_off = (ADIO_Offset *) ADIOI_Malloc(sizeof(ADIO_Offset));
            *srt_len = (int *) ADIOI_Malloc(sizeof(int));
            malloc_srt_num = 1;
        }
        (*srt_off)[0] = real_off;
        (*srt_len)[0] = real_size;
    }

    /* It is possible sum_recv (sum of message sizes to be received) is larger
     * than the size of collective buffer, write_buf, if writes from multiple
     * remote processes overlap. Receiving messages into overlapped regions of
     * the same write_buffer may cause a problem. To avoid it, we allocate a
     * temporary buffer big enough to receive all messages into disjointed
     * regions. Earlier in ADIOI_LUSTRE_Exch_and_write(), write_buf is already
     * allocated with twice amount of the file stripe size, wth the second half
     * to be used to receive messages. If sum_recv is smalled than file stripe
     * size, we can reuse that space. But if sum_recv is bigger (an overlap
     * case, which is rare), we allocate a separate buffer of size sum_recv.
     */
    sum_recv -= recv_size[myrank];
    if (sum_recv > striping_info[0])
        contig_buf = (char *) ADIOI_Malloc(sum_recv);
    else
        contig_buf = write_buf + striping_info[0];

#define MEMCPY_UNPACK(x, inbuf) {                                   \
    int _k;                                                         \
    char *_ptr = (inbuf);                                           \
    MPI_Aint    *mem_ptrs = others_req[x].mem_ptrs + start_pos[x];  \
    ADIO_Offset *mem_lens = others_req[x].lens     + start_pos[x];  \
    for (_k=0; _k<recv_count[x]; _k++) {                            \
        memcpy(write_buf + mem_ptrs[_k], _ptr, mem_lens[_k]);       \
        _ptr += mem_lens[_k];                                       \
    }                                                               \
}

    if (fd->atomicity) {
        /* atomicity uses traditional two-phase I/O communication strategy */
        /* nreqs is the number of Issend and Irecv to be posted */
        nreqs = 0;
        /* atomic mode calls blocking receive only */
        requests = (MPI_Request *) ADIOI_Malloc((nprocs_send + 1) * sizeof(MPI_Request));
        /* Post sends: if buftype_is_contig, data can be directly sent from user
         * buf at location given by buf_idx. Otherwise, copy write data to send_buf
         * first and use send_buf to send.
         */
        if (buftype_is_contig) {
            tag = myrank + 100 * iter;
            for (i = 0; i < nprocs; i++)
                if (send_size[i] && i != myrank) {
                    ADIOI_Assert(buf_idx[i] != -1);
                    MPI_Issend((char *) buf + buf_idx[i], send_size[i],
                               MPI_BYTE, i, tag + i, fd->comm, &requests[nreqs++]);
                }
        } else if (nprocs_send) {
            /* If buftype is not contiguous, pack data into send_buf[], including
             * ones sent to self.
             */
            send_total_size = 0;
            for (i = 0; i < nprocs; i++)
                send_total_size += send_size[i];
            send_buf = (char **) ADIOI_Malloc(nprocs * sizeof(char *));
            send_buf[0] = (char *) ADIOI_Malloc(send_total_size);
            for (i = 1; i < nprocs; i++)
                send_buf[i] = send_buf[i - 1] + send_size[i - 1];

            ADIOI_LUSTRE_Fill_send_buffer(fd, buf, flat_buf, send_buf, offset_list,
                                          len_list, send_size, requests + nreqs,
                                          sent_to_proc, nprocs, myrank,
                                          contig_access_count, striping_info,
                                          send_buf_idx, curr_to_proc, done_to_proc,
                                          iter, buftype_extent);
            /* MPI_Issend calls have been posted in ADIOI_Fill_send_buffer */
            nreqs += (send_size[myrank]) ? nprocs_send - 1 : nprocs_send;
        }
        /* In atomic mode, we must receive write data in the increasing order
         * of MPI process rank IDs,
         */
        tag = myrank + 100 * iter;
        for (i = 0; i < nprocs; i++) {
            if (recv_size[i] == 0)
                continue;
            if (i != myrank) {
                MPI_Recv(contig_buf, recv_size[i], MPI_BYTE, i, tag + i, fd->comm, &status);
                MEMCPY_UNPACK(i, contig_buf);
            } else {
                /* send/recv to/from self uses memcpy() */
                char *ptr = (buftype_is_contig) ? (char *) buf + buf_idx[i] : send_buf[i];
                MEMCPY_UNPACK(i, ptr);
            }
        }
        if (nreqs > 0) {
#ifdef MPI_STATUSES_IGNORE
            MPI_Waitall(nreqs, requests, MPI_STATUSES_IGNORE);
#else
            MPI_Status *statuses = (MPI_Status *) ADIOI_Malloc(nreqs * sizeof(MPI_Status));
            MPI_Waitall(nreqs, requests, statuses);
            ADIOI_Free(statuses);
#endif
        }
        if (!buftype_is_contig && nprocs_send) {
            ADIOI_Free(send_buf[0]);
            ADIOI_Free(send_buf);
        }
        ADIOI_Free(requests);
    } else {
        /* Non-atomicity case adopt TAM communication strategy.
         * Most metadata arrays has been malloced at ad_open.c because they have fixed size.
         * We only need to prepare the buffer for data since they have variable size.
         * The fd->local_buf is enlarged when necessary.
         * TAM has three communication phases plus data preparation.
         * 0. prepare local contiguous send buffer.
         * 1. Intra-node aggregation of message size from nonaggregators to local aggregators.
         * 2. Intra-node aggregation of data from nonaggregators to local aggregators.
         * 3. Inter-node aggregation of data from local aggregators to global aggregators. */

        /* 0. This section first pack local send data into send_buf */
        send_total_size = 0;
        /* Only global aggregators receive data. The rest of send_size entry must be zero. 
         * cb_send_size is used in order to reduce the number of integer at intra-node aggregation. */
        for (i = 0; i < fd->hints->cb_nodes; ++i) {
            send_total_size += send_size[fd->hints->ranklist[i]];
            /* Skip self-send. */
            if ( myrank != fd->hints->ranklist[i] ) {
                fd->cb_send_size[i] = send_size[fd->hints->ranklist[i]];
            } else {
                fd->cb_send_size[i] = 0;
            }
        }
        /* We always put send_buf[myrank] to the end of the contiguous array because we do not want to break contiguous array into two parts after removing it
         * send_buf[myrank] is unpacked directly without participating in communication. */
        send_buf = (char **) ADIOI_Malloc(nprocs * sizeof(char *));
        send_buf_start = (char *) ADIOI_Malloc(send_total_size+1);
        if (myrank != fd->hints->ranklist[0]) {
            /* nprocs >=2 for this case, we are pretty safe to put send_buf[myrank] into the end. */
            send_buf[fd->hints->ranklist[0]] = send_buf_start;
            buf_ptr = send_buf[fd->hints->ranklist[0]] + send_size[fd->hints->ranklist[0]];
            for (i = 1; i < fd->hints->cb_nodes; ++i) {
                if ( fd->hints->ranklist[i] != myrank ) {
                    send_buf[fd->hints->ranklist[i]] = buf_ptr;
                    buf_ptr += send_size[fd->hints->ranklist[i]];
                }
            }
            send_buf[myrank] = buf_ptr;
        } else {
            /* myrank == first global aggregator, but nprocs can be 1, need to be extra careful.
             * We split into two cases. */
            if (fd->hints->cb_nodes > 1) {
                /* Must check if we have more than 1 global aggregator to enter this block*/
                send_buf[fd->hints->ranklist[1]] = send_buf_start;
                for ( i = 2; i < fd->hints->cb_nodes; ++i ) {
                    send_buf[fd->hints->ranklist[i]] = send_buf[fd->hints->ranklist[i-1]] + send_size[fd->hints->ranklist[i-1]];
                }
                send_buf[fd->hints->ranklist[0]] = send_buf[fd->hints->ranklist[fd->hints->cb_nodes - 1]] + send_size[fd->hints->ranklist[fd->hints->cb_nodes - 1]];
            } else {
                /* I am the only rank 0, so I am the start of send_buf_start */
                send_buf[myrank] = send_buf_start;
            }
        }

        if (buftype_is_contig) {
            /* We copy contiguous buftype into a contiguous array. */
            for (i = 0; i < nprocs; i++) {
                if (send_size[i]) {
                    memcpy(send_buf[i], (char *) buf + buf_idx[i], sizeof(char) * send_size[i]);
                }
            }
        } else if (nprocs_send) {
            /* If buftype is not contiguous, pack data into send_buf[], including
             * ones sent to self.
             * We use a no send version because send is handled by TAM.
             */
            ADIOI_LUSTRE_Fill_send_buffer_no_send(fd, buf, flat_buf, send_buf, offset_list,
                                          len_list, send_size, NULL,
                                          sent_to_proc, nprocs, myrank,
                                          contig_access_count, striping_info,
                                          send_buf_idx, curr_to_proc, done_to_proc,
                                          iter, buftype_extent);
        }
        /* Local message directly unpack, otherwise it has to go to a local aggregator then sent back, a waste of bandwidth */
        if ( send_size[myrank] ) {
            MEMCPY_UNPACK(myrank, send_buf[myrank]);
        }
        /* End of buffer preparation */
        /* 1. Local aggregators receive the message size from non-local aggregators
         * We do not want to gather the entire send_size array, since this would be each of length nprocs and does not scale as the number of processes increases.
         * For example, 16K process would have 16K*16K*4B=1GB total data exchanged at this stage. Although this is intra-node aggregation, this is even more than most Lustre stripe size * stripe count, which can also cause a problem.
         * send_size[i] must be 0 for i being non-global aggregators, so we only gather cb_nodes number of integers that indicate. 
         * For example, 64 Lustre stripe count would have 16K * 64 * 4B = 4MB This is a much smaller number */
        j = 0;
        if (fd->is_local_aggregator) {
            /* Array for local message size of size fd->nprocs_aggregator * cb_nodes */
            for ( i = 0; i < fd->nprocs_aggregator; ++i ) {
                if ( fd->aggregator_local_ranks[i] != myrank ){
                    MPI_Irecv(fd->local_send_size + fd->hints->cb_nodes * i, fd->hints->cb_nodes, MPI_INT, fd->aggregator_local_ranks[i], fd->aggregator_local_ranks[i] + myrank, fd->comm, &req[j++]);
                } else {
                    memcpy(fd->local_send_size + fd->hints->cb_nodes * i, fd->cb_send_size, sizeof(int) * fd->hints->cb_nodes);
                }
            }
        }
        /* Send message size to local aggregators*/
        if ( fd->my_local_aggregator != myrank ){
            MPI_Issend(fd->cb_send_size, fd->hints->cb_nodes, MPI_INT, fd->my_local_aggregator, myrank + fd->my_local_aggregator, fd->comm, &req[j++]);
        }
        if (j) {
#ifdef MPI_STATUSES_IGNORE
            MPI_Waitall(j, req, MPI_STATUSES_IGNORE);
#else
            MPI_Waitall(j, req, sts);
#endif
        }
        /* End of gathering message size */
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
                if ( fd->aggregator_local_ranks[0] != myrank ){
                    MPI_Irecv(fd->local_buf, fd->local_lens[fd->hints->cb_nodes - 1], MPI_BYTE, fd->aggregator_local_ranks[0], fd->aggregator_local_ranks[0] + myrank, fd->comm, &req[j++]);
                } else {
                    memcpy(fd->local_buf, send_buf_start, fd->local_lens[fd->hints->cb_nodes - 1] * sizeof(char) );
                }
            }
            /* The rest of local processes*/
            for ( i = 1; i < fd->nprocs_aggregator; ++i ) {
                if (fd->local_lens[(i+1)*fd->hints->cb_nodes-1] == fd->local_lens[i*fd->hints->cb_nodes-1]) {
                    /* No data for this local aggregator from this local process, just jump to the next one. */
                    continue;
                }
                /* Shift the buffer with prefix-sum and do receive*/
                if ( fd->aggregator_local_ranks[i] != myrank ){
                    MPI_Irecv(fd->local_buf + fd->local_lens[i * fd->hints->cb_nodes - 1], fd->local_lens[(i+1)*fd->hints->cb_nodes-1] - fd->local_lens[i*fd->hints->cb_nodes-1], MPI_BYTE, fd->aggregator_local_ranks[i], fd->aggregator_local_ranks[i] + myrank, fd->comm, &req[j++]);
                } else {
                    memcpy(fd->local_buf + fd->local_lens[i * fd->hints->cb_nodes - 1], send_buf_start, (fd->local_lens[(i + 1) * fd->hints->cb_nodes-1] - fd->local_lens[i * fd->hints->cb_nodes-1]) * sizeof(char) );
                }
            }
        }
        /* Send a large contiguous array of all data at local process to my local aggregator. */
        if ( fd->my_local_aggregator != myrank && (send_total_size - send_size[myrank]) ){
            MPI_Issend(send_buf_start, send_total_size - send_size[myrank], MPI_BYTE, fd->my_local_aggregator, myrank + fd->my_local_aggregator, fd->comm, &req[j++]);
        }
        if (j) {
#ifdef MPI_STATUSES_IGNORE
            MPI_Waitall(j, req, MPI_STATUSES_IGNORE);
#else
            MPI_Waitall(j, req, sts);
#endif
        }
        /* End of intra-node aggregation phase */

        /* Contiguous send buffer is no longer needed*/

        ADIOI_Free(send_buf_start);
        ADIOI_Free(send_buf);

        /* 3. Inter-node aggregation phase of data from local aggregators to global aggregators.
         * Global aggregators know the data size from all processes in recv_size, so there is no need to exchange data size, this can boost performance. */
        j = 0;
        if (nprocs_recv) {
            /* global_recv_size is an array that indicate the sum of data size to be received from local aggregators. */
//            memset(fd->global_recv_size, 0, sizeof(MPI_Aint) * fd->local_aggregator_size);
            for ( i = 0; i < fd->local_aggregator_size; ++i ) {
                /* We need to count data size from local aggregators 
                 * global_recv_size is an array of local aggregator size. */
                fd->global_recv_size[i] = 0;
                for ( k = 0; k < fd->local_aggregator_domain_size[i]; ++k ) {
                    /* Remeber a global aggregator does not receive anything from itself (so does its domain handled by any local aggregators)*/
                    if (myrank != fd->local_aggregator_domain[i][k]) {
                        fd->global_recv_size[i] += recv_size[fd->local_aggregator_domain[i][k]];
                    }
                }
            }
            /* Now we can do the Irecv post from local aggregators to global aggregators
             * receive messages into contig_buf, a temporary buffer (global aggregators only)
             * This buffer has enough size to fit all data. We unpack it to offset/length mempory later. */
            buf_ptr = contig_buf;
            for ( i = 0; i < fd->local_aggregator_size; ++i ) {
                if (fd->local_aggregators[i] != myrank) {
                    if (fd->global_recv_size[i]) {
                        MPI_Irecv(buf_ptr, fd->global_recv_size[i], MPI_BYTE, fd->local_aggregators[i], fd->local_aggregators[i] + myrank, fd->comm, &req[j++]);
                        buf_ptr += fd->global_recv_size[i];
                    }
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
                            MEMCPY_UNPACK(fd->aggregator_local_ranks[k], fd->local_buf + fd->local_lens[k * fd->hints->cb_nodes + i - 1]);
                        } else {
                            MEMCPY_UNPACK(fd->aggregator_local_ranks[k], fd->local_buf);
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
        /* End of inter-node aggregation, no more MPI communications. */

        /* local aggregators free derived datatypes */
        if ( fd->is_local_aggregator ){
            for ( i = 0; i < fd->hints->cb_nodes; ++i){
                /* A simple check for if we have actually created the type */
                if (fd->new_types[i] != MPI_BYTE) {
                    MPI_Type_free(fd->new_types + i);
                }
            }
        }
        /* We need to unpack global aggregator buffer into the correct offsets.
         * The ranks proxied by local aggregators may not be ordered from rank 0 to p-1, so we need to be careful by checking their domain. */
        if (nprocs_recv) {
            buf_ptr = contig_buf;
            for ( i = 0; i < fd->local_aggregator_size; ++i ) {
                /* Local aggregate messages are unpacked previously */
                if (fd->local_aggregators[i] != myrank){
                    /* Directly unpack from current buffer. The buffer may not be ordered.
                     * local_aggregator_domain record which processes a local aggregator represents for.
                     * The contig_buf is in the order of local aggregators (contiguously with respect to its domain), so we just unpack them one by one carefully. */
                    for ( k = 0; k < fd->local_aggregator_domain_size[i]; ++k ) {
                        /* Local data has been unpacked at the beginning, nothing should be received from its local aggregator. */
                        if (fd->local_aggregator_domain[i][k] != myrank) {
                            MEMCPY_UNPACK(fd->local_aggregator_domain[i][k], (char *) buf_ptr);
                            buf_ptr += recv_size[fd->local_aggregator_domain[i][k]];
                        }
                    }
                }
            }
        }
    }
    /* free temporary receive buffer */
    if (sum_recv > striping_info[0])
        ADIOI_Free(contig_buf);
}


#define ADIOI_BUF_INCR \
{ \
    while (buf_incr) { \
        size_in_buf = MPL_MIN(buf_incr, flat_buf_sz); \
        user_buf_idx += size_in_buf; \
        flat_buf_sz -= size_in_buf; \
        if (!flat_buf_sz) { \
            if (flat_buf_idx < (flat_buf->count - 1)) flat_buf_idx++; \
            else { \
                flat_buf_idx = 0; \
                n_buftypes++; \
            } \
            user_buf_idx = flat_buf->indices[flat_buf_idx] + \
                (ADIO_Offset)n_buftypes*(ADIO_Offset)buftype_extent;  \
            flat_buf_sz = flat_buf->blocklens[flat_buf_idx]; \
        } \
        buf_incr -= size_in_buf; \
    } \
}


#define ADIOI_BUF_COPY \
{ \
    while (size) { \
        size_in_buf = MPL_MIN(size, flat_buf_sz); \
        ADIOI_Assert((((ADIO_Offset)(uintptr_t)buf) + user_buf_idx) == (ADIO_Offset)(uintptr_t)((uintptr_t)buf + user_buf_idx)); \
        ADIOI_Assert(size_in_buf == (size_t)size_in_buf);               \
        memcpy(&(send_buf[p][send_buf_idx[p]]), \
               ((char *) buf) + user_buf_idx, size_in_buf); \
        send_buf_idx[p] += size_in_buf; \
        user_buf_idx += size_in_buf; \
        flat_buf_sz -= size_in_buf; \
        if (!flat_buf_sz) { \
            if (flat_buf_idx < (flat_buf->count - 1)) flat_buf_idx++; \
            else { \
                flat_buf_idx = 0; \
                n_buftypes++; \
            } \
            user_buf_idx = flat_buf->indices[flat_buf_idx] + \
                (ADIO_Offset)n_buftypes*(ADIO_Offset)buftype_extent;    \
            flat_buf_sz = flat_buf->blocklens[flat_buf_idx]; \
        } \
        size -= size_in_buf; \
        buf_incr -= size_in_buf; \
    } \
    ADIOI_BUF_INCR \
}

static void ADIOI_LUSTRE_Fill_send_buffer(ADIO_File fd, const void *buf,
                                          ADIOI_Flatlist_node * flat_buf,
                                          char **send_buf,
                                          ADIO_Offset * offset_list,
                                          ADIO_Offset * len_list, int *send_size,
                                          MPI_Request * requests,
                                          int *sent_to_proc, int nprocs,
                                          int myrank,
                                          int contig_access_count,
                                          int *striping_info,
                                          ADIO_Offset * send_buf_idx,
                                          int *curr_to_proc,
                                          int *done_to_proc, int iter, MPI_Aint buftype_extent)
{
    /* this function is only called if buftype is not contig */
    int i, p, flat_buf_idx, size;
    int flat_buf_sz, buf_incr, size_in_buf, jj, n_buftypes;
    ADIO_Offset off, len, rem_len, user_buf_idx;

    /* curr_to_proc[p] = amount of data sent to proc. p that has already
     * been accounted for so far
     * done_to_proc[p] = amount of data already sent to proc. p in
     * previous iterations
     * user_buf_idx = current location in user buffer
     * send_buf_idx[p] = current location in send_buf of proc. p
     */

    for (i = 0; i < nprocs; i++) {
        send_buf_idx[i] = curr_to_proc[i] = 0;
        done_to_proc[i] = sent_to_proc[i];
    }
    jj = 0;

    user_buf_idx = flat_buf->indices[0];
    flat_buf_idx = 0;
    n_buftypes = 0;
    flat_buf_sz = flat_buf->blocklens[0];

    /* flat_buf_idx = current index into flattened buftype
     * flat_buf_sz = size of current contiguous component in flattened buf
     */
    for (i = 0; i < contig_access_count; i++) {
        off = offset_list[i];
        rem_len = (ADIO_Offset) len_list[i];

        /*this request may span to more than one process */
        while (rem_len != 0) {
            len = rem_len;
            /* NOTE: len value is modified by ADIOI_Calc_aggregator() to be no
             * longer than the single region that processor "p" is responsible
             * for.
             */
            p = ADIOI_LUSTRE_Calc_aggregator(fd, off, &len, striping_info);

            if (send_buf_idx[p] < send_size[p]) {
                if (curr_to_proc[p] + len > done_to_proc[p]) {
                    if (done_to_proc[p] > curr_to_proc[p]) {
                        size = (int) MPL_MIN(curr_to_proc[p] + len -
                                             done_to_proc[p], send_size[p] - send_buf_idx[p]);
                        buf_incr = done_to_proc[p] - curr_to_proc[p];
                        ADIOI_BUF_INCR
                            ADIOI_Assert((curr_to_proc[p] + len - done_to_proc[p]) ==
                                         (unsigned) (curr_to_proc[p] + len - done_to_proc[p]));
                        buf_incr = (int) (curr_to_proc[p] + len - done_to_proc[p]);
                        ADIOI_Assert((done_to_proc[p] + size) ==
                                     (unsigned) (done_to_proc[p] + size));
                        curr_to_proc[p] = done_to_proc[p] + size;
                    ADIOI_BUF_COPY} else {
                        size = (int) MPL_MIN(len, send_size[p] - send_buf_idx[p]);
                        buf_incr = (int) len;
                        ADIOI_Assert((curr_to_proc[p] + size) ==
                                     (unsigned) ((ADIO_Offset) curr_to_proc[p] + size));
                        curr_to_proc[p] += size;
                    ADIOI_BUF_COPY}
                    if (send_buf_idx[p] == send_size[p]) {
                        MPI_Issend(send_buf[p], send_size[p], MPI_BYTE, p,
                                   myrank + p + 100 * iter, fd->comm, requests + jj);
                        jj++;
                    }
                } else {
                    ADIOI_Assert((curr_to_proc[p] + len) ==
                                 (unsigned) ((ADIO_Offset) curr_to_proc[p] + len));
                    curr_to_proc[p] += (int) len;
                    buf_incr = (int) len;
                ADIOI_BUF_INCR}
            } else {
                buf_incr = (int) len;
            ADIOI_BUF_INCR}
            off += len;
            rem_len -= len;
        }
    }
    for (i = 0; i < nprocs; i++)
        if (send_size[i])
            sent_to_proc[i] = curr_to_proc[i];
}

/* This function calls ADIOI_OneSidedWriteAggregation iteratively to
 * essentially pack stripes of data into the collective buffer and then
 * flush the collective buffer to the file when fully packed, repeating this
 * process until all the data is written to the file.
 */
static void ADIOI_LUSTRE_IterateOneSided(ADIO_File fd, const void *buf, int *striping_info,
                                         ADIO_Offset * offset_list, ADIO_Offset * len_list,
                                         int contig_access_count, int currentValidDataIndex,
                                         int count, int file_ptr_type, ADIO_Offset offset,
                                         ADIO_Offset start_offset, ADIO_Offset end_offset,
                                         ADIO_Offset firstFileOffset, ADIO_Offset lastFileOffset,
                                         MPI_Datatype datatype, int myrank, int *error_code)
{
    int i;
    int stripesPerAgg = fd->hints->cb_buffer_size / striping_info[0];
    if (stripesPerAgg == 0) {
        /* The striping unit is larger than the collective buffer size
         * therefore we must abort since the buffer has already been
         * allocated during the open.
         */
        FPRINTF(stderr, "Error: The collective buffer size %d is less "
                "than the striping unit size %d - the ROMIO "
                "Lustre one-sided write aggregation algorithm "
                "cannot continue.\n", fd->hints->cb_buffer_size, striping_info[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* Based on the co_ratio the number of aggregators we can use is the number of
     * stripes used in the file times this co_ratio - each stripe is written by
     * co_ratio aggregators this information is contained in the striping_info.
     */
    int numStripedAggs = striping_info[2];

    int orig_cb_nodes = fd->hints->cb_nodes;
    fd->hints->cb_nodes = numStripedAggs;

    /* Declare ADIOI_OneSidedStripeParms here - these parameters will be locally managed
     * for this invokation of ADIOI_LUSTRE_IterateOneSided.  This will allow for concurrent
     * one-sided collective writes via multi-threading as well as multiple communicators.
     */
    ADIOI_OneSidedStripeParms stripeParms;
    stripeParms.stripeSize = striping_info[0];
    stripeParms.stripedLastFileOffset = lastFileOffset;
    stripeParms.iWasUsedStripingAgg = 0;
    stripeParms.numStripesUsed = 0;
    stripeParms.amountOfStripedDataExpected = 0;
    stripeParms.bufTypeExtent = 0;
    stripeParms.lastDataTypeExtent = 0;
    stripeParms.lastFlatBufIndice = 0;
    stripeParms.lastIndiceOffset = 0;

    /* The general algorithm here is to divide the file up into segements, a segment
     * being defined as a contiguous region of the file which has up to one occurrence
     * of each stripe - the data for each stripe being written out by a particular
     * aggregator.  The segmentLen is the maximum size in bytes of each segment
     * (stripeSize*number of aggs).  Iteratively call ADIOI_OneSidedWriteAggregation
     * for each segment to aggregate the data to the collective buffers, but only do
     * the actual write (via flushCB stripe parm) once stripesPerAgg stripes
     * have been packed or the aggregation for all the data is complete, minimizing
     * synchronization.
     */
    stripeParms.segmentLen = ((ADIO_Offset) numStripedAggs) * ((ADIO_Offset) (striping_info[0]));

    /* These arrays define the file offsets for the stripes for a given segment - similar
     * to the concept of file domains in GPFS, essentially file domeains for the segment.
     */
    ADIO_Offset *segment_stripe_start =
        (ADIO_Offset *) ADIOI_Malloc(numStripedAggs * sizeof(ADIO_Offset));
    ADIO_Offset *segment_stripe_end =
        (ADIO_Offset *) ADIOI_Malloc(numStripedAggs * sizeof(ADIO_Offset));

    /* Find the actual range of stripes in the file that have data in the offset
     * ranges being written -- skip holes at the front and back of the file.
     */
    int currentOffsetListIndex = 0;
    int fileSegmentIter = 0;
    int startingStripeWithData = 0;
    int foundStartingStripeWithData = 0;
    while (!foundStartingStripeWithData) {
        if (((startingStripeWithData + 1) * (ADIO_Offset) (striping_info[0])) > firstFileOffset)
            foundStartingStripeWithData = 1;
        else
            startingStripeWithData++;
    }

    ADIO_Offset currentSegementOffset =
        (ADIO_Offset) startingStripeWithData * (ADIO_Offset) (striping_info[0]);

    int numSegments =
        (int) ((lastFileOffset + (ADIO_Offset) 1 - currentSegementOffset) / stripeParms.segmentLen);
    if ((lastFileOffset + (ADIO_Offset) 1 - currentSegementOffset) % stripeParms.segmentLen > 0)
        numSegments++;

    /* To support read-modify-write use a while-loop to redo the aggregation if necessary
     * to fill in the holes.
     */
    int doAggregation = 1;
    int holeFound = 0;

    /* Remember romio_onesided_no_rmw setting if we have to re-do
     * the aggregation if holes are found.
     */
    int prev_romio_onesided_no_rmw = fd->romio_onesided_no_rmw;

    while (doAggregation) {

        int totalDataWrittenLastRound = 0;

        /* This variable tracks how many segment stripes we have packed into the agg
         * buffers so we know when to flush to the file system.
         */
        stripeParms.segmentIter = 0;

        /* stripeParms.stripesPerAgg is the number of stripes to aggregate before doing a flush.
         */
        stripeParms.stripesPerAgg = stripesPerAgg;
        if (stripeParms.stripesPerAgg > numSegments)
            stripeParms.stripesPerAgg = numSegments;

        for (fileSegmentIter = 0; fileSegmentIter < numSegments; fileSegmentIter++) {

            int dataWrittenThisRound = 0;

            /* Define the segment range in terms of file offsets.
             */
            ADIO_Offset segmentFirstFileOffset = currentSegementOffset;
            if ((currentSegementOffset + stripeParms.segmentLen - (ADIO_Offset) 1) > lastFileOffset)
                currentSegementOffset = lastFileOffset;
            else
                currentSegementOffset += (stripeParms.segmentLen - (ADIO_Offset) 1);
            ADIO_Offset segmentLastFileOffset = currentSegementOffset;
            currentSegementOffset++;

            ADIO_Offset segment_stripe_offset = segmentFirstFileOffset;
            for (i = 0; i < numStripedAggs; i++) {
                if (firstFileOffset > segment_stripe_offset)
                    segment_stripe_start[i] = firstFileOffset;
                else
                    segment_stripe_start[i] = segment_stripe_offset;
                if ((segment_stripe_offset + (ADIO_Offset) (striping_info[0])) > lastFileOffset)
                    segment_stripe_end[i] = lastFileOffset;
                else
                    segment_stripe_end[i] =
                        segment_stripe_offset + (ADIO_Offset) (striping_info[0]) - (ADIO_Offset) 1;
                segment_stripe_offset += (ADIO_Offset) (striping_info[0]);
            }

            /* In the interest of performance for non-contiguous data with large offset lists
             * essentially modify the given offset and length list appropriately for this segment
             * and then pass pointers to the sections of the lists being used for this segment
             * to ADIOI_OneSidedWriteAggregation.  Remember how we have modified the list for this
             * segment, and then restore it appropriately after processing for this segment has
             * concluded, so it is ready for the next segment.
             */
            int segmentContigAccessCount = 0;
            int startingOffsetListIndex = -1;
            int endingOffsetListIndex = -1;
            ADIO_Offset startingOffsetAdvancement = 0;
            ADIO_Offset startingLenTrim = 0;
            ADIO_Offset endingLenTrim = 0;

            while (((offset_list[currentOffsetListIndex] +
                     ((ADIO_Offset) (len_list[currentOffsetListIndex])) - (ADIO_Offset) 1) <
                    segmentFirstFileOffset) && (currentOffsetListIndex < (contig_access_count - 1)))
                currentOffsetListIndex++;
            startingOffsetListIndex = currentOffsetListIndex;
            endingOffsetListIndex = currentOffsetListIndex;
            int offsetInSegment = 0;
            ADIO_Offset offsetStart = offset_list[currentOffsetListIndex];
            ADIO_Offset offsetEnd =
                (offset_list[currentOffsetListIndex] +
                 ((ADIO_Offset) (len_list[currentOffsetListIndex])) - (ADIO_Offset) 1);

            if (len_list[currentOffsetListIndex] == 0)
                offsetInSegment = 0;
            else if ((offsetStart >= segmentFirstFileOffset) &&
                     (offsetStart <= segmentLastFileOffset)) {
                offsetInSegment = 1;
            } else if ((offsetEnd >= segmentFirstFileOffset) &&
                       (offsetEnd <= segmentLastFileOffset)) {
                offsetInSegment = 1;
            } else if ((offsetStart <= segmentFirstFileOffset) &&
                       (offsetEnd >= segmentLastFileOffset)) {
                offsetInSegment = 1;
            }

            if (!offsetInSegment) {
                segmentContigAccessCount = 0;
            } else {
                /* We are in the segment, advance currentOffsetListIndex until we are out of segment.
                 */
                segmentContigAccessCount = 1;

                while ((offset_list[currentOffsetListIndex] <= segmentLastFileOffset) &&
                       (currentOffsetListIndex < contig_access_count)) {
                    dataWrittenThisRound += (int) len_list[currentOffsetListIndex];
                    currentOffsetListIndex++;
                }

                if (currentOffsetListIndex > startingOffsetListIndex) {
                    /* If we did advance, if we are at the end need to check if we are still in segment.
                     */
                    if (currentOffsetListIndex == contig_access_count) {
                        currentOffsetListIndex--;
                    } else if (offset_list[currentOffsetListIndex] > segmentLastFileOffset) {
                        /* We advanced into the last one and it still in the segment.
                         */
                        currentOffsetListIndex--;
                    } else {
                        dataWrittenThisRound += (int) len_list[currentOffsetListIndex];
                    }
                    segmentContigAccessCount += (currentOffsetListIndex - startingOffsetListIndex);
                    endingOffsetListIndex = currentOffsetListIndex;
                }
            }

            if (segmentContigAccessCount > 0) {
                /* Trim edges here so all data in the offset list range fits exactly in the segment.
                 */
                if (offset_list[startingOffsetListIndex] < segmentFirstFileOffset) {
                    startingOffsetAdvancement =
                        segmentFirstFileOffset - offset_list[startingOffsetListIndex];
                    offset_list[startingOffsetListIndex] += startingOffsetAdvancement;
                    dataWrittenThisRound -= (int) startingOffsetAdvancement;
                    startingLenTrim = startingOffsetAdvancement;
                    len_list[startingOffsetListIndex] -= startingLenTrim;
                }

                if ((offset_list[endingOffsetListIndex] +
                     ((ADIO_Offset) (len_list[endingOffsetListIndex])) - (ADIO_Offset) 1) >
                    segmentLastFileOffset) {
                    endingLenTrim =
                        offset_list[endingOffsetListIndex] +
                        ((ADIO_Offset) (len_list[endingOffsetListIndex])) - (ADIO_Offset) 1 -
                        segmentLastFileOffset;
                    len_list[endingOffsetListIndex] -= endingLenTrim;
                    dataWrittenThisRound -= (int) endingLenTrim;
                }
            }

            int holeFoundThisRound = 0;

            /* Once we have packed the collective buffers do the actual write.
             */
            if ((stripeParms.segmentIter == (stripeParms.stripesPerAgg - 1)) ||
                (fileSegmentIter == (numSegments - 1))) {
                stripeParms.flushCB = 1;
            } else
                stripeParms.flushCB = 0;

            stripeParms.firstStripedWriteCall = 0;
            stripeParms.lastStripedWriteCall = 0;
            if (fileSegmentIter == 0) {
                stripeParms.firstStripedWriteCall = 1;
            } else if (fileSegmentIter == (numSegments - 1))
                stripeParms.lastStripedWriteCall = 1;

            /* The difference in calls to ADIOI_OneSidedWriteAggregation is based on the whether the buftype is
             * contiguous.  The algorithm tracks the position in the source buffer when called
             * multiple times --  in the case of contiguous data this is simple and can be externalized with
             * a buffer offset, in the case of non-contiguous data this is complex and the state must be tracked
             * internally, therefore no external buffer offset.  Care was taken to minimize
             * ADIOI_OneSidedWriteAggregation changes at the expense of some added complexity to the caller.
             */
            int bufTypeIsContig;
            ADIOI_Datatype_iscontig(datatype, &bufTypeIsContig);
            if (bufTypeIsContig) {
                ADIOI_OneSidedWriteAggregation(fd,
                                               (ADIO_Offset *) &
                                               (offset_list[startingOffsetListIndex]),
                                               (ADIO_Offset *) &
                                               (len_list[startingOffsetListIndex]),
                                               segmentContigAccessCount,
                                               buf + totalDataWrittenLastRound, datatype,
                                               error_code, segmentFirstFileOffset,
                                               segmentLastFileOffset, currentValidDataIndex,
                                               segment_stripe_start, segment_stripe_end,
                                               &holeFoundThisRound, &stripeParms);
            } else {
                ADIOI_OneSidedWriteAggregation(fd,
                                               (ADIO_Offset *) &
                                               (offset_list[startingOffsetListIndex]),
                                               (ADIO_Offset *) &
                                               (len_list[startingOffsetListIndex]),
                                               segmentContigAccessCount, buf, datatype, error_code,
                                               segmentFirstFileOffset, segmentLastFileOffset,
                                               currentValidDataIndex, segment_stripe_start,
                                               segment_stripe_end, &holeFoundThisRound,
                                               &stripeParms);
            }

            if (stripeParms.flushCB) {
                stripeParms.segmentIter = 0;
                if (stripesPerAgg > (numSegments - fileSegmentIter - 1))
                    stripeParms.stripesPerAgg = numSegments - fileSegmentIter - 1;
                else
                    stripeParms.stripesPerAgg = stripesPerAgg;
            } else
                stripeParms.segmentIter++;

            if (holeFoundThisRound)
                holeFound = 1;

            /* If we know we won't be doing a pre-read in a subsequent call to
             * ADIOI_OneSidedWriteAggregation which will have a barrier to keep
             * feeder ranks from doing rma to the collective buffer before the
             * write completes that we told it do with the stripeParms.flushCB
             * flag then we need to do a barrier here.
             */
            if (!fd->romio_onesided_always_rmw && stripeParms.flushCB) {
                if (fileSegmentIter < (numSegments - 1)) {
                    MPI_Barrier(fd->comm);
                }
            }

            /* Restore the offset_list and len_list to values that are ready for the
             * next iteration.
             */
            if (segmentContigAccessCount > 0) {
                offset_list[endingOffsetListIndex] += len_list[endingOffsetListIndex];
                len_list[endingOffsetListIndex] = endingLenTrim;
            }
            totalDataWrittenLastRound += dataWrittenThisRound;
        }       // fileSegmentIter for-loop

        /* Check for holes in the data unless romio_onesided_no_rmw is set.
         * If a hole is found redo the entire aggregation and write.
         */
        if (!fd->romio_onesided_no_rmw) {
            int anyHolesFound = 0;
            MPI_Allreduce(&holeFound, &anyHolesFound, 1, MPI_INT, MPI_MAX, fd->comm);

            if (anyHolesFound) {
                ADIOI_Free(offset_list);
                ADIOI_Calc_my_off_len(fd, count, datatype, file_ptr_type, offset,
                                      &offset_list, &len_list, &start_offset,
                                      &end_offset, &contig_access_count);

                currentSegementOffset =
                    (ADIO_Offset) startingStripeWithData *(ADIO_Offset) (striping_info[0]);
                fd->romio_onesided_always_rmw = 1;
                fd->romio_onesided_no_rmw = 1;

                /* Holes are found in the data and the user has not set
                 * romio_onesided_no_rmw --- set romio_onesided_always_rmw to 1
                 * and redo the entire aggregation and write and if the user has
                 * romio_onesided_inform_rmw set then inform him of this condition
                 * and behavior.
                 */
                if (fd->romio_onesided_inform_rmw && (myrank == 0)) {
                    FPRINTF(stderr, "Information: Holes found during one-sided "
                            "write aggregation algorithm --- re-running one-sided "
                            "write aggregation with ROMIO_ONESIDED_ALWAYS_RMW set to 1.\n");
                }
            } else
                doAggregation = 0;
        } else
            doAggregation = 0;
    }   // while doAggregation
    fd->romio_onesided_no_rmw = prev_romio_onesided_no_rmw;

    ADIOI_Free(segment_stripe_start);
    ADIOI_Free(segment_stripe_end);

    fd->hints->cb_nodes = orig_cb_nodes;

}
