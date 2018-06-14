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
static void ADIOI_LUSTRE_Exch_and_write(ADIO_File fd,
                                        const void *buf,
                                        MPI_Datatype datatype,
                                        int nprocs,
                                        int myrank,
                                        ADIOI_LUSTRE_Access * my_access,
                                        ADIOI_Access * my_req,
                                        ADIOI_Access * others_req,
                                        ADIOI_LUSTRE_file_domain * file_domain,
                                        int *striping_info, int *error_code);
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
                                          ADIOI_LUSTRE_file_domain * file_domain,
                                          int *done_to_proc, int iter, MPI_Aint buftype_extent);
static void ADIOI_LUSTRE_W_Exchange_data(ADIO_File fd, const void *buf, char *write_buf,
                                         ADIOI_Flatlist_node * flat_buf, int *send_size,
                                         int *recv_size, ADIO_Offset off, ADIO_Offset size,
                                         int *count, int *start_pos, int *partial_recv,
                                         MPI_Datatype * recv_types, int *nprocs_recv_ptr,
                                         MPI_Request ** recv_reqs_ptr, int *sent_to_proc,
                                         int nprocs, int myrank, ADIOI_LUSTRE_Access * my_access,
                                         ADIOI_Access * my_req, ADIOI_Access * others_req,
                                         ADIOI_LUSTRE_file_domain * file_domain,
                                         ADIO_Offset * send_buf_idx,
                                         int *curr_to_proc, int *done_to_proc,
                                         int iter, int max_ntimes, int *striping_info,
                                         MPI_Aint buftype_extent, ADIO_Offset ** srt_off_ptr,
                                         int **srt_len_ptr, int *srt_num_ptr, int *error_code);
void ADIOI_Heap_merge(ADIOI_Access * others_req, int *count, ADIO_Offset * srt_off,
                      int *srt_len, int *start_pos, int nprocs, int nprocs_recv,
                      int total_elements);

static void ADIOI_LUSTRE_IterateOneSided(ADIO_File fd, const void *buf, int *striping_info,
                                         ADIO_Offset * offset_list, ADIO_Offset * len_list,
                                         int contig_access_count, int currentValidDataIndex,
                                         int count, int file_ptr_type, ADIO_Offset offset,
                                         ADIO_Offset start_offset, ADIO_Offset end_offset,
                                         ADIO_Offset firstFileOffset, ADIO_Offset lastFileOffset,
                                         MPI_Datatype datatype, int myrank, int *error_code);

static void ADIOI_LUSTRE_Calc_send_size(ADIO_File fd, int iter,
                                        ADIOI_LUSTRE_file_domain * file_domain,
                                        ADIOI_Access * my_req, int *striping_info, int *send_size);

static void ADIOI_LUSTRE_Calc_recv_size(ADIO_File fd,
                                        int nprocs, int myrank,
                                        ADIO_Offset off,
                                        int size,
                                        ADIOI_Access * others_req,
                                        int *striping_info,
                                        int *recv_count,
                                        int *recv_start_pos,
                                        int *recv_size,
                                        int *partial_recv_size,
                                        int *recv_curr_offlen_ptr, int *error_code);

static void ADIOI_LUSTRE_Write_data_pipe(ADIO_File fd,
                                         int nprocs, int myrank,
                                         int iter, int ntimes,
                                         int num_sub_buffers,
                                         int num_reqs_to_wait,
                                         char **write_buf_array,
                                         int **recv_count_array,
                                         int *striping_info,
                                         ADIOI_LUSTRE_file_domain * file_domain,
                                         int *srt_len,
                                         ADIO_Offset * srt_off, int *srt_num_ptr, int *error_code);

static void ADIOI_LUSTRE_Write_data(ADIO_File fd,
                                    int iter, int ntimes,
                                    ADIO_Offset offset,
                                    char *write_buf,
                                    int *striping_info,
                                    ADIOI_LUSTRE_file_domain * file_domain,
                                    int *srt_len,
                                    ADIO_Offset * srt_off, int *srt_num_ptr, int *error_code);

static int ADIOI_LUSTRE_Wait_recvs(int iter,
                                   int ntimes,
                                   int num_sub_buffers,
                                   int num_reqs_to_wait,
                                   int *nprocs_recv_array,
                                   ADIOI_LUSTRE_file_domain * file_domain,
                                   MPI_Request ** recv_reqs_array);

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
    int *striping_info = NULL;
    ADIO_Offset **buf_idx = NULL;
    int old_error, tmp_error;
    ADIO_Offset *lustre_offsets0, *lustre_offsets, *count_sizes = NULL;
    ADIOI_LUSTRE_Access my_access;
    ADIOI_LUSTRE_file_domain file_domain;

    MPI_Comm_size(fd->comm, &nprocs);
    MPI_Comm_rank(fd->comm, &myrank);

    orig_fp = fd->fp_ind;

    /* IO patten identification if cb_write isn't disabled */
    if (fd->hints->cb_write != ADIOI_HINT_DISABLE) {
        /* For this process's request, calculate the list of offsets and
         * lengths in the file and determine the start and end offsets. */

        /* Note: end_offset points to the last byte-offset that will be accessed.
         * e.g., if start_offset=0 and 100 bytes to be read, end_offset=99
         */

        ADIOI_LUSTRE_Calc_my_off_len(fd, count, datatype, file_ptr_type, offset, &my_access);

        start_offset = my_access.start_offset;
        end_offset = my_access.end_offset;
        contig_access_count = my_access.contig_access_count;
        offset_list = my_access.offset_list;
        len_list = my_access.len_list;

        /* each process communicates its start and end offsets to other
         * processes. The result is an array each of start and end offsets
         * stored in order of process rank.
         */
        st_offsets = (ADIO_Offset *) ADIOI_Malloc(nprocs * sizeof(ADIO_Offset));
        end_offsets = (ADIO_Offset *) ADIOI_Malloc(nprocs * sizeof(ADIO_Offset));
        ADIO_Offset my_count_size = 0;
        /* One-sided aggregation needs the amount of data per rank as well because
         * the difference in starting and ending offsets for 1 byte is 0 the same
         * as 0 bytes so it cannot be distiguished.
         */
        if ((romio_write_aggmethod == 1) || (romio_write_aggmethod == 2)) {
            count_sizes = (ADIO_Offset *) ADIOI_Malloc(nprocs * sizeof(ADIO_Offset));
            MPI_Count buftype_size;
            MPI_Type_size_x(datatype, &buftype_size);
            my_count_size = (ADIO_Offset) count *(ADIO_Offset) buftype_size;
        }
        if (romio_tunegather) {
            if ((romio_write_aggmethod == 1) || (romio_write_aggmethod == 2)) {
                lustre_offsets0 = (ADIO_Offset *) ADIOI_Malloc(3 * nprocs * sizeof(ADIO_Offset));
                lustre_offsets = (ADIO_Offset *) ADIOI_Malloc(3 * nprocs * sizeof(ADIO_Offset));
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
                lustre_offsets0 = (ADIO_Offset *) ADIOI_Malloc(2 * nprocs * sizeof(ADIO_Offset));
                lustre_offsets = (ADIO_Offset *) ADIOI_Malloc(2 * nprocs * sizeof(ADIO_Offset));
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
            ADIOI_Free(lustre_offsets);
        } else {
            MPI_Allgather(&start_offset, 1, ADIO_OFFSET, st_offsets, 1, ADIO_OFFSET, fd->comm);
            MPI_Allgather(&end_offset, 1, ADIO_OFFSET, end_offsets, 1, ADIO_OFFSET, fd->comm);
            if ((romio_write_aggmethod == 1) || (romio_write_aggmethod == 2)) {
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
            ADIOI_Free(len_list);
            ADIOI_Free(st_offsets);
            ADIOI_Free(end_offsets);
            if ((romio_write_aggmethod == 1) || (romio_write_aggmethod == 2))
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
    if ((romio_write_aggmethod == 1) || (romio_write_aggmethod == 2)) {
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
    ADIOI_LUSTRE_Get_striping_info(fd, &striping_info, 1);
    /* If the user has specified to use a one-sided aggregation method then do that at
     * this point instead of the two-phase I/O.
     */
    if ((romio_write_aggmethod == 1) || (romio_write_aggmethod == 2)) {

        ADIOI_LUSTRE_IterateOneSided(fd, buf, striping_info, offset_list, len_list,
                                     contig_access_count, currentValidDataIndex, count,
                                     file_ptr_type, offset, start_offset, end_offset,
                                     firstFileOffset, lastFileOffset, datatype, myrank, error_code);

        ADIOI_Free(offset_list);
        ADIOI_Free(len_list);
        ADIOI_Free(st_offsets);
        ADIOI_Free(end_offsets);
        ADIOI_Free(count_sizes);
        ADIOI_Free(striping_info);

        goto fn_exit;

    }   // onesided aggregation


    /* Divide the I/O workload among "cb_nodes" processes (aggregator).
     * This is done by (logically) dividing the file into file domains,
     * each aggregator directly accesses only its own file domain.
     * Non-aggregators do not read/write files directly.
     *
     * There is one MPI_Allreduce() in ADIOI_LUSTRE_Calc_file_domains().
     */
    my_access.buftype_is_contig = buftype_is_contig;

    ADIOI_LUSTRE_Calc_file_domains(fd, nprocs, myrank, &my_access, &file_domain);

    /* calculate what portions of the access requests of this process are
     * located in which process
     */
    ADIOI_LUSTRE_Calc_my_req(fd, offset_list, len_list, contig_access_count,
                             striping_info, nprocs, &count_my_req_procs,
                             &count_my_req_per_proc, &file_domain, &my_req, &buf_idx);

    /* based on everyone's my_req, calculate what requests of other processes
     * will be accessed by this process.
     * count_others_req_procs = number of processes whose requests (including
     * this process itself) will be accessed by this process
     * count_others_req_per_proc[i] indicates how many separate contiguous
     * requests of proc. i will be accessed by this process.
     */

    ADIOI_LUSTRE_Calc_others_req(fd, nprocs, myrank, count_my_req_per_proc, my_req, &others_req);
    ADIOI_Free(count_my_req_per_proc);

    /* exchange data and write in sizes of no more than stripe_size. */
    ADIOI_LUSTRE_Exch_and_write(fd, buf, datatype, nprocs, myrank,
                                &my_access, my_req, others_req, &file_domain,
                                striping_info, error_code);

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
    ADIOI_Free(others_req[0].lens);
    ADIOI_Free(others_req[0].mem_ptrs);
    ADIOI_Free(others_req);
    /* free my_req here */
    for (i = 0; i < nprocs; i++) {
        if (my_req[i].count) {
            ADIOI_Free(my_req[i].offsets);
            ADIOI_Free(my_req[i].lens);
            ADIOI_Free(my_req[i].mem_ptrs);
        }
    }
    ADIOI_Free(my_req);
    for (i = 0; i < nprocs; i++) {
        ADIOI_Free(buf_idx[i]);
    }
    ADIOI_Free(buf_idx);
    ADIOI_Free(offset_list);
    ADIOI_Free(len_list);
    ADIOI_Free(st_offsets);
    ADIOI_Free(end_offsets);
    ADIOI_Free(striping_info);

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
static void ADIOI_LUSTRE_Exch_and_write(ADIO_File fd,
                                        const void *buf,
                                        MPI_Datatype datatype,
                                        int nprocs, int myrank,
                                        ADIOI_LUSTRE_Access * my_access,
                                        ADIOI_Access * my_req,
                                        ADIOI_Access * others_req,
                                        ADIOI_LUSTRE_file_domain * file_domain,
                                        int *striping_info, int *error_code)
{
    int i, j, m;
    int size, flag;
    int buf_idx = 0;
    int num_reqs_to_wait = 0;
    int ntimes, max_ntimes;
    int nprocs_recv;
    int srt_num;
    int num_sub_buffers;
    int sub_buffer_size;
    int *srt_len = NULL;
    int *send_size = NULL;
    int *send_curr_offlen_ptr = NULL;
    int *recv_size_ptr = NULL;
    int *recv_count_ptr = NULL;
    int *recv_size = NULL;
    int *recv_count = NULL;
    int *recv_start_pos = NULL;
    int *partial_recv_size = NULL;
    int *recv_curr_offlen_ptr = NULL;
    int *sent_to_proc = NULL;
    int *curr_to_proc = NULL;
    int *done_to_proc = NULL;
    int *nprocs_recv_array = NULL;
    int **recv_count_array = NULL;
    int **recv_size_array = NULL;
    const int is_agg = fd->is_agg;
    const int stripe_size = striping_info[0];
    const int striping_factor = striping_info[1];
    const int avail_cb_nodes = striping_info[2];
    const int cb_buffer_size = fd->hints->cb_buffer_size;
    char *write_buf = NULL;
    char **write_buf_array = NULL;
    ADIO_Offset off;
    ADIO_Offset coalesced_size;
    ADIO_Offset *srt_off = NULL;
    ADIO_Offset *send_buf_idx = NULL;
    MPI_Aint buftype_extent;
    MPI_Status *statuses = NULL;
    MPI_Request *recv_reqs = NULL;
    MPI_Request **recv_reqs_array = NULL;
    MPI_Datatype *recv_types = NULL;
    MPI_Datatype **recv_types_array = NULL;
    ADIOI_Flatlist_node *flat_buf = NULL;

    *error_code = MPI_SUCCESS;  /* changed below if error */

    /* Send requests to appropriate aggregators which write in sizes of no
     * more than coll_bufsize. The idea is to reduce the amount of extra
     * memory required for collective I/O. If all data were written all at
     * once, which is much easier, it would require temp space more than
     * the size of user_buf, which is often unacceptable. For example, to
     * write a distributed array to a file, where each local array is
     * 8Mbytes, requiring at least another 8Mbytes of temp space is
     * unacceptable. The temp buffers are used for packing, unpacking,
     * sending, and receiving.
     */
    ntimes = (is_agg) ? file_domain->ntimes[file_domain->my_agg_rank] : 0;

    /* max_ntimes is the maximal ntimes among all aggregators */
    max_ntimes = 0;
    for (i = 0; i < avail_cb_nodes; i++)
        max_ntimes = MPL_MAX(max_ntimes, file_domain->ntimes[i]);

    /* Calculate number of buffers and size of each buffer. */
    if (file_domain->pipelining == 1) {
        if (ntimes == 0)
            num_sub_buffers = 1;
        else
            num_sub_buffers = MPL_MIN(ntimes, cb_buffer_size / stripe_size);
        sub_buffer_size = stripe_size;
        if (ntimes <= num_sub_buffers)
            num_reqs_to_wait = 0;
        else
            num_reqs_to_wait = MPL_MIN(ntimes, num_sub_buffers) - 1;
    } else {
        num_sub_buffers = 1;    /* We only have a single collective buffer. */
        sub_buffer_size = cb_buffer_size;
        num_reqs_to_wait = 0;
    }

    /* Allocate memory spaces for metadata. Non-aggregators do not enter this code.
     * since ntimes is 0. */
    if (ntimes) {
        nprocs_recv_array = (int *) ADIOI_Malloc(sizeof(int) * num_sub_buffers);
        recv_size_array = (int **) ADIOI_Malloc(sizeof(int *) * num_sub_buffers);
        recv_count_array = (int **) ADIOI_Malloc(sizeof(int *) * num_sub_buffers);
        write_buf_array = (char **) ADIOI_Malloc(sizeof(char *) * num_sub_buffers);
        recv_reqs_array = (MPI_Request **) ADIOI_Malloc(sizeof(MPI_Request *) * num_sub_buffers);
        recv_types_array = (MPI_Datatype **) ADIOI_Malloc(sizeof(MPI_Datatype *) * num_sub_buffers);

        for (i = 0; i < num_sub_buffers; i++) {
            recv_size_array[i] = (int *) ADIOI_Malloc(sizeof(int) * nprocs);
            recv_count_array[i] = (int *) ADIOI_Malloc(sizeof(int) * nprocs);
            write_buf_array[i] = (char *) ADIOI_Malloc(sizeof(char) * sub_buffer_size);
            recv_types_array[i] = (MPI_Datatype *) ADIOI_Malloc(sizeof(MPI_Datatype) * nprocs);
            for (j = 0; j < nprocs; j++)
                recv_types_array[i][j] = MPI_BYTE;
        }
        recv_start_pos = (int *) ADIOI_Malloc(nprocs * sizeof(int));
        partial_recv_size = (int *) ADIOI_Calloc(nprocs, sizeof(int));
        recv_curr_offlen_ptr = (int *) ADIOI_Calloc(nprocs, sizeof(int));
    }

    send_size = (int *) ADIOI_Malloc(nprocs * sizeof(int));
    send_curr_offlen_ptr = (int *) ADIOI_Calloc(nprocs, sizeof(int));
    curr_to_proc = (int *) ADIOI_Malloc(nprocs * sizeof(int));
    done_to_proc = (int *) ADIOI_Malloc(nprocs * sizeof(int));
    sent_to_proc = (int *) ADIOI_Calloc(nprocs, sizeof(int));
    send_buf_idx = (ADIO_Offset *) ADIOI_Malloc(nprocs * sizeof(ADIO_Offset));

    if (!my_access->buftype_is_contig)
        flat_buf = ADIOI_Flatten_and_find(datatype);
    MPI_Type_extent(datatype, &buftype_extent);

    /* main loop m:
     * only aggregators may have ntimes > 0
     * non-aggregators has ntimes == 0 and they only send their data.
     * some of the aggregators that have ntimes < max_ntimes
     * perform ntimes recvs and max_ntimes sends.
     */
    for (m = 0; m < max_ntimes; m++) {
        /* the collective write is divided into ntimes iterations, each
         * iteration goes through all others_req[*] and check which ones will
         * be satisfied by this iteration m
         *
         * Note that MPI guarantees that displacements in filetypes are in
         * monotonically nondecreasing order and that, for writes, the
         * filetypes cannot specify overlapping regions in the file. This
         * simplifies implementation a bit compared to reads.
         *
         * file_domain->offsets[ntimes] file_domain->sizes[ntimes] have been
         * set in ADIOI_LUSTRE_Calc_file_domains() that together indicate the
         * range of this aggregator's file sub-domain in this current
         * iteration m
         */
        if (m < ntimes) {
            buf_idx = m % num_sub_buffers;

            write_buf = write_buf_array[buf_idx];
            recv_types = recv_types_array[buf_idx];
            recv_size_ptr = recv_size_array[buf_idx];
            recv_count_ptr = recv_count_array[buf_idx];
            memset(recv_size_ptr, 0, sizeof(int) * nprocs);
            memset(recv_count_ptr, 0, sizeof(int) * nprocs);
        } else {
            recv_size_ptr = NULL;
            recv_count_ptr = NULL;
        }

        /* Calculate metadata for recv.
         * Note that offset and length are needed only for recv.
         *  - recv_size[]
         *  - partial_recv_size[]
         *  - recv_count[]
         *  - recv_start_pos[]
         *  Note that recv_size[] is calculated only 'ntimes' times.
         *  When m >= ntimes, this process sends data out only.
         */
        if (m < ntimes) {
            off = file_domain->offsets[file_domain->my_agg_rank][m];
            size = file_domain->sizes[file_domain->my_agg_rank][m];

            ADIOI_LUSTRE_Calc_recv_size(fd, nprocs, myrank, off, size,
                                        others_req, striping_info,
                                        recv_count_ptr, recv_start_pos,
                                        recv_size_ptr, partial_recv_size,
                                        recv_curr_offlen_ptr, error_code);
        }

        /* Calculate metadata for send.
         *  - send_size[]
         *  If ntimes < max_ntimes, it means this process has to send
         *  some data out for other processes.
         */
        memset(send_size, 0, sizeof(int) * nprocs);
        ADIOI_LUSTRE_Calc_send_size(fd, m, file_domain, my_req, striping_info, send_size);

        /* Exchange the data based on the metadata calculated above. */
        ADIOI_LUSTRE_W_Exchange_data(fd, buf, write_buf, flat_buf,
                                     send_size, recv_size_ptr, off, size,
                                     recv_count_ptr, recv_start_pos, partial_recv_size, recv_types,
                                     &nprocs_recv, &recv_reqs,
                                     sent_to_proc, nprocs, myrank,
                                     my_access, my_req, others_req, file_domain,
                                     send_buf_idx, curr_to_proc, done_to_proc,
                                     m, max_ntimes, striping_info, buftype_extent,
                                     &srt_off, &srt_len, &srt_num, error_code);
        if (*error_code != MPI_SUCCESS)
            return;

        /* Wait for all the recvs initiated in ADIOI_LUSTRE_W_Exchange_data().
         * Note that sends have been already waited inside the function. */
        if (m < ntimes) {
            nprocs_recv_array[buf_idx] = nprocs_recv;
            recv_reqs_array[buf_idx] = recv_reqs;

            flag = ADIOI_LUSTRE_Wait_recvs(m, ntimes,
                                           num_sub_buffers,
                                           num_reqs_to_wait,
                                           nprocs_recv_array, file_domain, recv_reqs_array);

            /* Write data here! Since only aggregators receive data,
             * they are also the ones who write the data.
             */
            if (flag) { /* now it is time to write to the file */
                if (file_domain->pipelining == 1) {
                    ADIOI_LUSTRE_Write_data_pipe(fd, nprocs, myrank,
                                                 m, ntimes,
                                                 num_sub_buffers,
                                                 num_reqs_to_wait,
                                                 write_buf_array,
                                                 recv_count_array,
                                                 striping_info, file_domain,
                                                 srt_len, srt_off, &srt_num, error_code);
                } else {
                    ADIOI_LUSTRE_Write_data(fd, m, ntimes,
                                            off, write_buf,
                                            striping_info, file_domain,
                                            srt_len, srt_off, &srt_num, error_code);
                }
            }
        }
    }
    /* end of loop m */
    if (ntimes) {
        for (i = 0; i < num_sub_buffers; i++) {
            for (j = 0; j < nprocs; j++) {
                if (recv_types_array[i][j] != MPI_BYTE)
                    MPI_Type_free(&recv_types_array[i][j]);
            }
            ADIOI_Free(recv_types_array[i]);
            ADIOI_Free(write_buf_array[i]);
            ADIOI_Free(recv_size_array[i]);
            ADIOI_Free(recv_count_array[i]);
        }
        ADIOI_Free(nprocs_recv_array);
        ADIOI_Free(recv_reqs_array);
        ADIOI_Free(write_buf_array);
        ADIOI_Free(recv_types_array);
        ADIOI_Free(recv_count_array);
        ADIOI_Free(recv_size_array);
        ADIOI_Free(recv_curr_offlen_ptr);
        ADIOI_Free(recv_start_pos);
        ADIOI_Free(partial_recv_size);
    }
    ADIOI_Free(send_size);
    ADIOI_Free(sent_to_proc);
    ADIOI_Free(send_buf_idx);
    ADIOI_Free(curr_to_proc);
    ADIOI_Free(done_to_proc);
}

/* Sets error_code to MPI_SUCCESS if successful, or creates an error code
 * in the case of error.
 */
static void ADIOI_LUSTRE_W_Exchange_data(ADIO_File fd, const void *buf, char *write_buf,
                                         ADIOI_Flatlist_node * flat_buf, int *send_size,
                                         int *recv_size, ADIO_Offset off, ADIO_Offset size,
                                         int *count, int *start_pos, int *partial_recv,
                                         MPI_Datatype * recv_types, int *nprocs_recv_ptr,
                                         MPI_Request ** recv_reqs_ptr, int *sent_to_proc,
                                         int nprocs, int myrank, ADIOI_LUSTRE_Access * my_access,
                                         ADIOI_Access * my_req, ADIOI_Access * others_req,
                                         ADIOI_LUSTRE_file_domain * file_domain,
                                         ADIO_Offset * send_buf_idx,
                                         int *curr_to_proc, int *done_to_proc,
                                         int iter, int max_ntimes, int *striping_info,
                                         MPI_Aint buftype_extent, ADIO_Offset ** srt_off_ptr,
                                         int **srt_len_ptr, int *srt_num_ptr, int *error_code)
{
    int i, j, k, *tmp_len, nprocs_recv, nprocs_send, err;
    char **send_buf = NULL;
    MPI_Request *recv_reqs = NULL;
    MPI_Datatype *send_types = NULL;
    MPI_Status *statuses, status;
    int sum, sum_recv;
    static char myname[] = "ADIOI_W_EXCHANGE_DATA";
    int cb_nodes, cyclic_nprocs;

    *srt_off_ptr = NULL;
    *srt_len_ptr = NULL;
    *srt_num_ptr = 0;

    cb_nodes = striping_info[2];
    cyclic_nprocs = MPL_MIN(cb_nodes, fd->hints->striping_factor);

    send_types = (MPI_Datatype *) ADIOI_Malloc(nprocs * sizeof(MPI_Datatype));
    for (i = 0; i < nprocs; i++)
        send_types[i] = MPI_BYTE;

    /* (aggregators only) create derived datatypes for recv */
    nprocs_recv = 0;
    if (recv_size != NULL) {
        for (i = 0; i < nprocs; i++)
            if (recv_size[i])
                nprocs_recv++;
    }
    *nprocs_recv_ptr = nprocs_recv;

    if (nprocs_recv) {  /* prepare the receiving part (aggregators only) */
        int striping_unit = fd->hints->striping_unit;
        int *srt_len;
        ADIO_Offset *srt_off;

        tmp_len = (int *) ADIOI_Malloc(nprocs * sizeof(int));
        for (i = 0; i < nprocs; i++) {
            if (recv_size[i] == 0)
                continue;

            /* take care if the last off-len pair is a partial recv */
            if (partial_recv[i]) {
                k = start_pos[i] + count[i] - 1;        /* k is the last request */
                tmp_len[i] = others_req[i].lens[k];
                others_req[i].lens[k] = partial_recv[i];
            }

            /* [Sunwoo] Since ADIO_Access->lens is ADIO_Offset which is 8 bytes,
             * it cannot be used for MPI_Type_hindexed() directly.
             * Here we allocate a temporary int array and move the lengths into the array.
             * This should be fixed later. */
            ADIOI_Type_create_hindexed_x(count[i],
                                         &(others_req[i].lens[start_pos[i]]),
                                         &(others_req[i].mem_ptrs[start_pos[i]]),
                                         MPI_BYTE, recv_types + i);
            MPI_Type_commit(recv_types + i);
        }

        /* To avoid a read-modify-write, check if there are holes in the
         * data to be written. For this, merge the (sorted) offset lists
         * others_req using a heap-merge.
         */

        sum = 0;
        for (i = 0; i < nprocs; i++)
            sum += count[i];
        srt_off = (ADIO_Offset *) ADIOI_Malloc(sum * sizeof(ADIO_Offset));
        srt_len = (int *) ADIOI_Malloc(sum * sizeof(int));

        /* sort the write requests for this sub-domain (off, size) received
         * from all other processes into an increasing order based on
         * their file offsets. The two arrays, srt_off[sum] and srt_len[sum],
         * contain the sorted requests.
         */
        ADIOI_Heap_merge(others_req, count, srt_off, srt_len, start_pos, nprocs, nprocs_recv, sum);

        /* for partial recvs, restore original lengths */
        for (i = 0; i < nprocs; i++)
            if (partial_recv[i]) {
                k = start_pos[i] + count[i] - 1;
                others_req[i].lens[k] = tmp_len[i];
            }
        ADIOI_Free(tmp_len);

        /* check if there are any holes, if yes, must do read-modify-write */

        /* first coalesce the sorted offset-length pairs */
        j = 0;
        for (i = 1; i < sum; i++) {
            ADIO_Offset stripe_end = srt_off[j] - srt_off[j] % striping_unit + striping_unit;
            if (srt_off[i] < stripe_end &&      /* check if done with this stripe */
                srt_off[i] <= srt_off[j] + srt_len[j]) {
                /* j and i intersect, merge i to j */
                int new_len = srt_off[i] + srt_len[i] - srt_off[j];
                if (new_len > srt_len[j])
                    srt_len[j] = new_len;
            } else {    /* move i to the place of j+1 */
                j++;
                if (i != j) {
                    srt_off[j] = srt_off[i];
                    srt_len[j] = srt_len[i];
                }
            }
        }
        sum = j + 1;
        *srt_off_ptr = srt_off;
        *srt_len_ptr = srt_len;
        *srt_num_ptr = sum;

        /* The current implementation does not skip the stripes in the
         * middle of file domain that contains no write requests.
         * Read-modify-write is still performed for those stripes.
         */
        if (fd->hints->ds_write != ADIOI_HINT_DISABLE) {
            /* check for holes. If there is, must do read-modify-write */
            char *buf_ptr = write_buf;
            ADIO_Offset rem, read_off = off, cyclic_gap;
            cyclic_gap = (ADIO_Offset) striping_unit *(cyclic_nprocs - 1);
            rem = size;

            j = 0;
            while (rem > 0) {   /* check holes in each stripe */
                int read_size = striping_unit - read_off % striping_unit;
                read_size = MPL_MIN(read_size, rem);
                /* check holes in stripe (read_off, read_off + read_size) */

                /* find the first srt_off[i] that intersects this stripe */
                for (i = j; i < sum; i++) {
                    MPI_Offset hole_off = srt_off[i] + srt_len[i];

                    if (hole_off <= read_off)
                        continue;
                    if (read_off < srt_off[i]) {
                        /* a hole is from read_off to srt_off[i]
                         * simply read the entire stripe and move on to the
                         * next stripe. This strategy ensures at most one read
                         * per stripe.
                         */
                        ADIO_ReadContig(fd, buf_ptr, read_size, MPI_BYTE,
                                        ADIO_EXPLICIT_OFFSET, read_off, &status, &err);
                        j = i;
                        break;
                    }
                    if (hole_off < read_off + read_size) {
                        /* a hole is at the end of stripe
                         * simple read the tail of this stripe and move on to
                         * the next stripe. This strategy ensures at most one
                         * read per stripe.
                         */
                        ADIO_ReadContig(fd, buf_ptr + (hole_off - read_off),
                                        read_off + read_size - hole_off,
                                        MPI_BYTE, ADIO_EXPLICIT_OFFSET, hole_off, &status, &err);
                        j = i;
                        break;
                    }
                    if (read_off + read_size <= hole_off) {
                        j = i;  /* this stripe is entirely covered */
                        break;
                    }
                }
                /* move on to next stripe */
                buf_ptr += read_size;
                rem -= read_size + cyclic_gap;

                /* move on to next stripe */
                if (rem > 0)
                    read_off += read_size + cyclic_gap;
            }
        }
    }
    /* end of if (nprocs_recv) */

    nprocs_send = 0;
    for (i = 0; i < nprocs; i++)
        if (send_size[i])
            nprocs_send++;

    static MPI_Request *send_reqs;
#ifndef _USE_BSEND
    int send_req_idx;
    static int num_send_reqs;
    if (iter == 0 || my_access->buftype_is_contig == 0) {
        /* if it is the first iteration or buffer type is non-contiguous */
        send_reqs = NULL;
        if (nprocs_send > 0)
            send_reqs = (MPI_Request *) ADIOI_Malloc(nprocs_send * sizeof(MPI_Request));
        num_send_reqs = nprocs_send;
        send_req_idx = 0;
    } else if (nprocs_send > 0) {
        /* augment the request array for the 2nd and following iterations */
        send_req_idx = num_send_reqs;
        num_send_reqs += nprocs_send;
        send_reqs = (MPI_Request *) ADIOI_Realloc(send_reqs, num_send_reqs * sizeof(MPI_Request))
    }
#endif

    if (!fd->atomicity && nprocs_recv > 0) {
        /* post receives from all other processes */
        recv_reqs = (MPI_Request *) ADIOI_Malloc(nprocs_recv * sizeof(MPI_Request));
        j = 0;
        for (i = 0; i < nprocs; i++) {
            if (recv_size[i]) {
                MPI_Irecv(write_buf, 1, recv_types[i], i,
                          myrank + i + 100 * iter, fd->comm, recv_reqs + j);
                j++;
            }
        }
        /* assert(j == nprocs_recv); */
    }
    *recv_reqs_ptr = recv_reqs;

    /* Till now, all irecv have been posted. Start to post isend.
     * If my_access->buftype_is_contig, data can be directly sent from
     * user buffer at location given by buf_idx. Otherwise allocate
     * a new send_buf[nprocs][] and copy the requests to it
     *
     * All processes participate the sends, not just aggregators
     */
#ifdef _USE_BSEND
    static void *bsend_buf = NULL;
    int bsend_buf_size;
    if (bsend_buf != NULL) {
        MPI_Buffer_detach(&bsend_buf, &bsend_buf_size);
        ADIOI_Free(bsend_buf);
    }
    bsend_buf_size = my_access->buf_size + nprocs_send * MPI_BSEND_OVERHEAD;
    bsend_buf = ADIOI_Malloc(bsend_buf_size);
    MPI_Buffer_attach(bsend_buf, bsend_buf_size);
#endif

    if (my_access->buftype_is_contig) {
        /* build non-contiguous data type and post the async send */
#ifndef _USE_BSEND
        j = send_req_idx;
#endif
        for (i = 0; i < nprocs; i++) {  /* send to aggregators only */
            int l, tail, rem, k_len_rem;
            if (send_size[i] == 0)
                continue;

            rem = send_size[i];
            tail = -1;
            for (k = 0; k < my_req[i].count; k++) {
                if (tail == -1 && my_req[i].lens[k] > 0)
                    tail = k;
                if (rem > my_req[i].lens[k])
                    rem -= my_req[i].lens[k];
                else {
                    k_len_rem = my_req[i].lens[k] - rem;
                    my_req[i].offsets[k] += my_req[i].lens[k] - rem;
                    my_req[i].lens[k] = rem;
                    break;
                }
            }
            if (k == tail) {    /* if tail==k, no need to create send type */
#ifdef _USE_BSEND
                MPI_Bsend(((char *) buf) + my_req[i].mem_ptrs[k],
                          my_req[i].lens[k], MPI_BYTE, i, myrank + i + 100 * iter, fd->comm);
#else
                MPI_Isend(((char *) buf) + my_req[i].mem_ptrs[k],
                          my_req[i].lens[k], MPI_BYTE, i, myrank + i + 100 * iter, fd->comm,
                          send_reqs + j);
                j++;
#endif
            } else {    /* there are (k-tail+1) non-contiguous */
                ADIOI_Type_create_hindexed_x(k - tail + 1,
                                             &(my_req[i].lens[tail]),
                                             &(my_req[i].mem_ptrs[tail]), MPI_BYTE, send_types + i);
                MPI_Type_commit(send_types + i);

#ifdef _USE_BSEND
                MPI_Bsend(buf, 1, send_types[i], i, myrank + i + 100 * iter, fd->comm);
#else
                MPI_Isend(buf, 1, send_types[i], i, myrank + i + 100 * iter, fd->comm,
                          send_reqs + j);
                j++;
#endif
            }
            for (l = tail; l < k; l++)
                my_req[i].lens[l] = 0;
            my_req[i].mem_ptrs[k] += my_req[i].lens[k];
            my_req[i].offsets[k] += my_req[i].lens[k] - k_len_rem;
            my_req[i].lens[k] = k_len_rem;
        }
    } /* of if (my_access->buftype_is_contig) */
    else if (nprocs_send) {
        /* buftype is not contig, need to pack send buffer */
        send_buf = (char **) ADIOI_Malloc(nprocs * sizeof(char *));
        sum = 0;
        for (i = 0; i < nprocs; i++)
            sum += send_size[i];
        send_buf[0] = (char *) ADIOI_Malloc(sum);
        for (i = 1; i < nprocs; i++)
            send_buf[i] = send_buf[i - 1] + send_size[i - 1];
        ADIOI_LUSTRE_Fill_send_buffer(fd, buf, flat_buf, send_buf, my_access->offset_list,
                                      my_access->len_list, send_size, send_reqs,
                                      sent_to_proc, nprocs, myrank,
                                      my_access->contig_access_count, striping_info,
                                      send_buf_idx, curr_to_proc,
                                      file_domain, done_to_proc, iter, buftype_extent);
        /* the send is done in ADIOI_LUSTRE_Fill_send_buffer */
    }

    if (fd->atomicity && nprocs_recv) {
        /* bug fix from Wei-keng Liao and Kenin Coloma */
        for (i = 0; i < nprocs; i++) {
            if (recv_size[i]) {
                /* blocking recv makes sure the receiving order is in
                 * increasng process ranks
                 */
                MPI_Recv(write_buf, 1, recv_types[i], i, myrank + i + 100 * iter,
                         fd->comm, &status);
            }
        }
    }
#ifndef _USE_BSEND
    /* wait for all asynchronous send to complete */
    if (num_send_reqs > 0 && (!my_access->buftype_is_contig || iter == max_ntimes - 1)) {
        statuses = (MPI_Status *) ADIOI_Malloc(num_send_reqs * sizeof(MPI_Status));
        MPI_Waitall(num_send_reqs, send_reqs, statuses);
        ADIOI_Free(statuses);
        ADIOI_Free(send_reqs);
    }
#endif

    for (i = 0; i < nprocs; i++) {
        if (send_types[i] != MPI_BYTE)
            MPI_Type_free(&send_types[i]);
    }
    ADIOI_Free(send_types);

    if (send_buf != NULL) {
        ADIOI_Free(send_buf[0]);
        ADIOI_Free(send_buf);
    }
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
                                          ADIOI_LUSTRE_file_domain * file_domain,
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
            p = ADIOI_LUSTRE_Calc_aggregator(fd, off, file_domain, &len, striping_info);

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
                        MPI_Isend(send_buf[p], send_size[p], MPI_BYTE, p,
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

    /* The maximum number of aggregators we can use is the number of
     * stripes used in the file - each agg writes exactly 1 stripe.
     */
    int numStripedAggs = striping_info[2];

    int orig_cb_nodes = fd->hints->cb_nodes;
    if (fd->hints->cb_nodes > numStripedAggs)
        fd->hints->cb_nodes = numStripedAggs;
    else if (fd->hints->cb_nodes < numStripedAggs)
        numStripedAggs = fd->hints->cb_nodes;

    /* Declare ADIOI_OneSidedStripeParms here as some fields will not change.
     */
    ADIOI_OneSidedStripeParms stripeParms;
    stripeParms.stripeSize = striping_info[0];
    stripeParms.stripedLastFileOffset = lastFileOffset;

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
    int prev_romio_onesided_no_rmw = romio_onesided_no_rmw;

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
                if (firstFileOffset > segmentFirstFileOffset)
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
                                               &holeFoundThisRound, stripeParms);
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
                                               stripeParms);
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
            if (!romio_onesided_always_rmw && stripeParms.flushCB) {
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
        if (!romio_onesided_no_rmw) {
            int anyHolesFound = 0;
            MPI_Allreduce(&holeFound, &anyHolesFound, 1, MPI_INT, MPI_MAX, fd->comm);

            if (anyHolesFound) {
                ADIOI_Free(offset_list);
                ADIOI_Free(len_list);
                ADIOI_Calc_my_off_len(fd, count, datatype, file_ptr_type, offset,
                                      &offset_list, &len_list, &start_offset,
                                      &end_offset, &contig_access_count);

                currentSegementOffset =
                    (ADIO_Offset) startingStripeWithData *(ADIO_Offset) (striping_info[0]);
                romio_onesided_always_rmw = 1;
                romio_onesided_no_rmw = 1;

                /* Holes are found in the data and the user has not set
                 * romio_onesided_no_rmw --- set romio_onesided_always_rmw to 1
                 * and redo the entire aggregation and write and if the user has
                 * romio_onesided_inform_rmw set then inform him of this condition
                 * and behavior.
                 */
                if (romio_onesided_inform_rmw && (myrank == 0)) {
                    FPRINTF(stderr, "Information: Holes found during one-sided "
                            "write aggregation algorithm --- re-running one-sided "
                            "write aggregation with ROMIO_ONESIDED_ALWAYS_RMW set to 1.\n");
                }
            } else
                doAggregation = 0;
        } else
            doAggregation = 0;
    }   // while doAggregation
    romio_onesided_no_rmw = prev_romio_onesided_no_rmw;

    ADIOI_Free(segment_stripe_start);
    ADIOI_Free(segment_stripe_end);

    fd->hints->cb_nodes = orig_cb_nodes;

}

static void ADIOI_LUSTRE_Calc_send_size(ADIO_File fd,
                                        int iter,
                                        ADIOI_LUSTRE_file_domain * file_domain,
                                        ADIOI_Access * my_req, int *striping_info, int *send_size)
{
    int i, k;
    const int cb_nodes = striping_info[2];

    for (i = 0; i < cb_nodes; i++) {
        if (iter >= file_domain->ntimes[i])
            continue;

        int dest = fd->hints->ranklist[i];
        int dest_len = 0;
        for (k = 0; k < my_req[dest].count; k++) {
            if (my_req[dest].lens[k] == 0)
                continue;
            if (my_req[dest].offsets[k] + my_req[dest].lens[k] < file_domain->offsets[i][iter] ||
                my_req[dest].offsets[k] >=
                file_domain->offsets[i][iter] + file_domain->sizes[i][iter])
                continue;

            if (my_req[dest].offsets[k] < file_domain->offsets[i][iter])
                dest_len += my_req[dest].lens[k] - (file_domain->offsets[i][iter] -
                                                    my_req[dest].offsets[k]);
            else
                dest_len += my_req[dest].lens[k];

            ADIO_Offset extra =
                (my_req[dest].offsets[k] + my_req[dest].lens[k]) - (file_domain->offsets[i][iter] +
                                                                    file_domain->sizes[i][iter]);
            if (extra > 0)
                dest_len -= extra;
        }
        send_size[dest] = dest_len;
    }
}

static void ADIOI_LUSTRE_Calc_recv_size(ADIO_File fd,
                                        int nprocs, int myrank,
                                        ADIO_Offset off,
                                        int size,
                                        ADIOI_Access * others_req,
                                        int *striping_info,
                                        int *recv_count,
                                        int *recv_start_pos,
                                        int *recv_size,
                                        int *partial_recv_size,
                                        int *recv_curr_offlen_ptr, int *error_code)
{
    int i, j;
    int req_len;
    int num_gaps;
    int cyclic_nprocs;
    ADIO_Offset cyclic_gap;
    ADIO_Offset req_off;
    ADIO_Offset buf_off;
    static char myname[] = "ADIOI_EXCH_AND_WRITE";
    const int stripe_size = striping_info[0];
    const int striping_factor = striping_info[1];
    const int avail_cb_nodes = striping_info[2];

    /* cyclic_gap is the distance between 2 consecutive stripes
     * in an I/O aggregator's file domain */
    cyclic_nprocs = MPL_MIN(avail_cb_nodes, striping_factor);
    cyclic_gap = (ADIO_Offset) stripe_size *(cyclic_nprocs - 1);

    for (i = 0; i < nprocs; i++) {
        if (others_req[i].count) {
            /* recv_start_pos[i] will be used in ADIOI_LUSTRE_W_Exchange_data()
             * later to create datatype for receive buffer from process i.
             * it is the starting index in others_req[i].offsets[] and
             * others_req[i].lens[] for this loop m.
             *
             * recv_curr_offlen_ptr[i] will be used in the next iteration m+1,
             * For this iteration m, its value is resulsted from the previous
             * iteration m-1 and indicates the starting index of loop i
             * for others_req[i].offsets[j...]
             */
            recv_start_pos[i] = recv_curr_offlen_ptr[i];

            for (j = recv_curr_offlen_ptr[i]; j < others_req[i].count; j++) {
                req_off = others_req[i].offsets[j];
                req_len = others_req[i].lens[j];
                if (partial_recv_size[i]) {
                    req_off += partial_recv_size[i];
                    req_len -= partial_recv_size[i];

                    partial_recv_size[i] = 0;

                    /* modify the off-len pair to reflect this change */
                    others_req[i].offsets[j] = req_off;
                    others_req[i].lens[j] = req_len;
                }
                if (req_off >= off + size)
                    break;      /* loop j */
                buf_off = req_off - off;
                num_gaps = (req_off / stripe_size - off / stripe_size) / cyclic_nprocs;
                buf_off -= cyclic_gap * num_gaps;

                others_req[i].mem_ptrs[j] = buf_off;
                recv_size[i] += MPL_MIN(req_len, off + size - req_off);

                recv_count[i]++;

                if (off + size - req_off < req_len) {
                    partial_recv_size[i] = (int) (off + size - req_off);

                    /* --BEGIN ERROR HANDLING-- */
                    if ((j + 1 < others_req[i].count) &&
                        (others_req[i].offsets[j + 1] < off + size)) {
                        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                                           MPIR_ERR_RECOVERABLE,
                                                           myname,
                                                           __LINE__,
                                                           MPI_ERR_ARG,
                                                           "Filetype specifies overlapping write regions (which is illegal according to the MPI-2 specification)",
                                                           0);
                        /* allow to continue since additional
                         * communication might have to occur
                         */
                    }
                    /* --END ERROR HANDLING-- */
                    break;      /* loop j */
                }
            }   /* end of loop j */
            recv_curr_offlen_ptr[i] = j;
        }
    }   /* end of loop i */
}

static void ADIOI_LUSTRE_Write_data_pipe(ADIO_File fd,
                                         int nprocs, int myrank,
                                         int iter, int ntimes,
                                         int num_sub_buffers,
                                         int num_reqs_to_wait,
                                         char **write_buf_array,
                                         int **recv_count_array,
                                         int *striping_info,
                                         ADIOI_LUSTRE_file_domain * file_domain,
                                         int *srt_len,
                                         ADIO_Offset * srt_off, int *srt_num_ptr, int *error_code)
{
    int i;
    ADIO_Offset off;
    int write_flag;
    int write_len;
    int curr_iter;
    int curr_buf_idx;
    const int stripe_size = striping_info[0];
    char *write_buf;
    MPI_Status status;

    curr_iter = iter - num_reqs_to_wait;
    curr_buf_idx = (iter % num_sub_buffers) - num_reqs_to_wait;
    if (curr_buf_idx < 0)
        curr_buf_idx += num_sub_buffers;

    while (1) {
        write_flag = 0;
        for (i = 0; i < nprocs; i++) {
            if (recv_count_array[curr_buf_idx][i] > 0) {
                write_flag = 1;
                break;
            }
        }

        if (write_flag == 1) {
            write_buf = write_buf_array[curr_buf_idx];
            off = file_domain->offsets[file_domain->my_agg_rank][curr_iter];
            write_len = file_domain->sizes[file_domain->my_agg_rank][curr_iter];

            ADIO_WriteContig(fd, write_buf, write_len, MPI_BYTE,
                             ADIO_EXPLICIT_OFFSET, off, &status, error_code);
            if (*error_code != MPI_SUCCESS)
                return;

            if (fd->hints->ds_write == ADIOI_HINT_DISABLE) {
                if (*srt_num_ptr > 0) {
                    ADIOI_Free(srt_off);
                    ADIOI_Free(srt_len);
                    *srt_num_ptr = 0;
                }
            }
        }

        if (iter == ntimes - 1) {
            curr_buf_idx++;
            if (curr_buf_idx == num_sub_buffers)
                curr_buf_idx = 0;

            curr_iter++;
            if (curr_iter == ntimes)    /* finished all the requests at the last iteration */
                break;
        } else {
            break;
        }
    }   /* end of while loop */
}

static void ADIOI_LUSTRE_Write_data(ADIO_File fd,
                                    int iter, int ntimes,
                                    ADIO_Offset offset,
                                    char *write_buf,
                                    int *striping_info,
                                    ADIOI_LUSTRE_file_domain * file_domain,
                                    int *srt_len,
                                    ADIO_Offset * srt_off, int *srt_num_ptr, int *error_code)
{
    int i;
    char *buf_ptr;
    int cyclic_gap;
    int cyclic_nprocs;
    int coalesced_size;
    int rem_unit;
    int write_len;
    ADIO_Offset off;
    ADIO_Offset stripe_end_off;
    const int stripe_size = striping_info[0];
    const int striping_factor = striping_info[1];
    const int avail_cb_nodes = striping_info[2];
    const int cb_buffer_size = fd->hints->cb_buffer_size;
    const int buffer_size = cb_buffer_size;
    MPI_Status status;

    cyclic_nprocs = MPL_MIN(avail_cb_nodes, striping_factor);
    cyclic_gap = (ADIO_Offset) stripe_size *(cyclic_nprocs - 1);

    coalesced_size = buffer_size;
    if (iter == ntimes - 1) {
        /* in the last iteration, the write buffer size may be less than
         * cb_buffer_size
         */
        coalesced_size = file_domain->fd_size % buffer_size;
        if (coalesced_size == 0)
            coalesced_size = buffer_size;
    }

    buf_ptr = write_buf;
    off = offset;
    if (fd->hints->ds_write != ADIOI_HINT_DISABLE) {
        /* data sieving will do read-modify-write if there are holes
         * in this process's file domain. The read part has been
         * carried out in ADIOI_LUSTRE_W_Exchange_data()
         */
        while (coalesced_size > 0) {
            rem_unit = stripe_size - off % stripe_size;
            write_len = MPL_MIN(rem_unit, coalesced_size);
            /* write a stripe at a time */
            ADIO_WriteContig(fd, buf_ptr, write_len, MPI_BYTE,
                             ADIO_EXPLICIT_OFFSET, off, &status, error_code);
            if (*error_code != MPI_SUCCESS)
                return;
            /* move to next stripe */
            coalesced_size -= write_len;
            buf_ptr += write_len;
            off += cyclic_gap + rem_unit;
        }
    } else {    /* data sieving is disabled, write only those segments
                 * with valid data and skip holes
                 */
        for (i = 0; i < (*srt_num_ptr); i++) {
            rem_unit = stripe_size - off % stripe_size;
            stripe_end_off = off + rem_unit;
            /* make sure off and srt_off[i] are in the same stripe */
            if (srt_off[i] >= stripe_end_off) {
                /* done with this stripe, move onto next stripe */
                buf_ptr += rem_unit;
                off = stripe_end_off + cyclic_gap;
                i--;    /* srt_off[i] and srt_len[i] not done yet */
            } else {
                /* now, write from srt_off[i] for length of
                 * MIN(rem_unit, srt_len[i])
                 */
                write_len = MPL_MIN(rem_unit, srt_len[i]);
                /* move buf_ptr and off forward if off < srt_off[i] */
                buf_ptr += srt_off[i] - off;
                off = MPL_MAX(off, srt_off[i]);
                ADIO_WriteContig(fd, buf_ptr, write_len, MPI_BYTE,
                                 ADIO_EXPLICIT_OFFSET, off, &status, error_code);
                if (*error_code != MPI_SUCCESS)
                    return;

                buf_ptr += write_len;
                off += write_len;

                /* check if we are done with this stripe */
                if (off == stripe_end_off ||
                    (i < (*srt_num_ptr) - 1 && stripe_end_off <= srt_off[i + 1])) {
                    /* skip rest of the stripe for the write buffer */
                    buf_ptr += stripe_end_off - off;

                    /* move on to the next stripe */
                    off = stripe_end_off + cyclic_gap;
                }
                /* check if we are done with str_off[i] and str_len[i]
                 * this is for the case that striping_factor == 1 or
                 * np == 1 so str_off[i] and str_len[i] is across two
                 * stripes
                 */
                if (srt_off[i] + srt_len[i] > stripe_end_off) {
                    srt_off[i] = stripe_end_off;
                    i--;
                }
            }
        }
        if (*srt_num_ptr > 0) {
            ADIOI_Free(srt_off);
            ADIOI_Free(srt_len);
            *srt_num_ptr = 0;
        }
    }
}

static int ADIOI_LUSTRE_Wait_recvs(int iter,
                                   int ntimes,
                                   int num_sub_buffers,
                                   int num_reqs_to_wait,
                                   int *nprocs_recv_array,
                                   ADIOI_LUSTRE_file_domain * file_domain,
                                   MPI_Request ** recv_reqs_array)
{
    int i;
    int flag = 0;
    int curr_buf_idx;
    MPI_Status *statuses = NULL;

    if (file_domain->pipelining == 1) {
        curr_buf_idx = (iter % num_sub_buffers) - num_reqs_to_wait;
        if (curr_buf_idx < 0)
            curr_buf_idx += num_sub_buffers;
        if ((iter >= num_reqs_to_wait) && (iter < ntimes - 1)) {
            if (nprocs_recv_array[curr_buf_idx] > 0) {
                statuses =
                    (MPI_Status *) ADIOI_Malloc(nprocs_recv_array[curr_buf_idx] *
                                                sizeof(MPI_Status));
                MPI_Waitall(nprocs_recv_array[curr_buf_idx], recv_reqs_array[curr_buf_idx],
                            statuses);
                flag = 1;
                ADIOI_Free(statuses);
                ADIOI_Free(recv_reqs_array[curr_buf_idx]);
            }
        } else if (iter == ntimes - 1) {
            for (i = 0; i < num_reqs_to_wait + 1; i++) {
                if (nprocs_recv_array[curr_buf_idx] > 0) {
                    statuses =
                        (MPI_Status *) ADIOI_Malloc(nprocs_recv_array[curr_buf_idx] *
                                                    sizeof(MPI_Status));
                    MPI_Waitall(nprocs_recv_array[curr_buf_idx], recv_reqs_array[curr_buf_idx],
                                statuses);
                    flag = 1;
                    ADIOI_Free(statuses);
                    ADIOI_Free(recv_reqs_array[curr_buf_idx]);
                }
                curr_buf_idx++;
                if (curr_buf_idx == num_sub_buffers)
                    curr_buf_idx = 0;
            }
        }
    } else {
        curr_buf_idx = 0;

        if (nprocs_recv_array[curr_buf_idx]) {
            statuses =
                (MPI_Status *) ADIOI_Malloc(nprocs_recv_array[curr_buf_idx] * sizeof(MPI_Status));
            MPI_Waitall(nprocs_recv_array[curr_buf_idx], recv_reqs_array[curr_buf_idx], statuses);
            flag = 1;
            ADIOI_Free(statuses);
            ADIOI_Free(recv_reqs_array[curr_buf_idx]);
        }
    }
    return flag;
}
