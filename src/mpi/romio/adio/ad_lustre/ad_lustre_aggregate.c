/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

void ADIOI_LUSTRE_Get_striping_info(ADIO_File fd, int **striping_info_ptr,
				    int mode)
{
    int *striping_info = NULL;
    /* get striping information:
     *  striping_info[0]: stripe_size
     *  striping_info[1]: stripe_count
     *  striping_info[2]: avail_cb_nodes
     */
    int stripe_size, stripe_count, CO = 1, CO_max = 1, CO_nodes, lflag;
    int avail_cb_nodes, divisor, nprocs_for_coll = fd->hints->cb_nodes;
    char *value = (char *) ADIOI_Malloc((MPI_MAX_INFO_VAL + 1) * sizeof(char));

    /* Get hints value */
    /* stripe size */
    stripe_size = fd->hints->striping_unit;
    /* stripe count */
    /* stripe_size and stripe_count have been validated in ADIOI_LUSTRE_Open() */
    stripe_count = fd->hints->striping_factor;

    /* Calculate the available number of I/O clients, that is
     *  avail_cb_nodes=min(cb_nodes, stripe_count*CO), where
     *  CO=1 by default
     */
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
        /* CO_max: the largest number of IO clients for each ost group */
        CO_max = (nprocs_for_coll - 1)/ stripe_count + 1;
        /* CO also has been validated in ADIOI_LUSTRE_Open(), >0 */
	CO = fd->hints->fs_hints.lustre.co_ratio;
	CO = ADIOI_MIN(CO_max, CO);
    }
    /* Calculate how many IO clients we need */
    /* To avoid extent lock conflicts,
     * avail_cb_nodes should divide (stripe_count*CO) exactly,
     * so that each OST is accessed by only one or more constant clients. */
    CO_nodes = stripe_count * CO;
    avail_cb_nodes = ADIOI_MIN(nprocs_for_coll, CO_nodes);
    if (avail_cb_nodes ==  CO_nodes) {
        do {
            /* find the divisor of CO_nodes */
            divisor = 1;
            do {
                divisor ++;
            } while (CO_nodes % divisor);
            CO_nodes = CO_nodes / divisor;
            /* if stripe_count*CO is a prime number, change nothing */
            if ((CO_nodes <= avail_cb_nodes) && (CO_nodes != 1)) {
                avail_cb_nodes = CO_nodes;
                break;
            }
        } while (CO_nodes != 1);
    }

    *striping_info_ptr = (int *) ADIOI_Malloc(3 * sizeof(int));
    striping_info = *striping_info_ptr;
    striping_info[0] = stripe_size;
    striping_info[1] = stripe_count;
    striping_info[2] = avail_cb_nodes;

    ADIOI_Free(value);
}

int ADIOI_LUSTRE_Calc_aggregator(ADIO_File fd, ADIO_Offset off,
                                 ADIO_Offset *len, int *striping_info)
{
    int rank_index, rank;
    ADIO_Offset avail_bytes;
    int stripe_size = striping_info[0];
    int avail_cb_nodes = striping_info[2];

    /* Produce the stripe-contiguous pattern for Lustre */
    rank_index = (int)((off / stripe_size) % avail_cb_nodes);

    /* we index into fd_end with rank_index, and fd_end was allocated to be no
     * bigger than fd->hins->cb_nodes.   If we ever violate that, we're
     * overrunning arrays.  Obviously, we should never ever hit this abort
     */
    if (rank_index >= fd->hints->cb_nodes)
	    MPI_Abort(MPI_COMM_WORLD, 1);

    avail_bytes = (off / (ADIO_Offset)stripe_size + 1) *
                  (ADIO_Offset)stripe_size - off;
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
void ADIOI_LUSTRE_Calc_my_req(ADIO_File fd, ADIO_Offset *offset_list,
			      int *len_list, int contig_access_count,
			      int *striping_info, int nprocs,
                              int *count_my_req_procs_ptr,
			      int **count_my_req_per_proc_ptr,
			      ADIOI_Access **my_req_ptr,
			      int **buf_idx_ptr)
{
    /* Nothing different from ADIOI_Calc_my_req(), except calling
     * ADIOI_Lustre_Calc_aggregator() instead of the old one */
    int *count_my_req_per_proc, count_my_req_procs, *buf_idx;
    int i, l, proc;
    ADIO_Offset avail_len, rem_len, curr_idx, off;
    ADIOI_Access *my_req;

    *count_my_req_per_proc_ptr = (int *) ADIOI_Calloc(nprocs, sizeof(int));
    count_my_req_per_proc = *count_my_req_per_proc_ptr;
    /* count_my_req_per_proc[i] gives the no. of contig. requests of this
     * process in process i's file domain. calloc initializes to zero.
     * I'm allocating memory of size nprocs, so that I can do an
     * MPI_Alltoall later on.
     */

    buf_idx = (int *) ADIOI_Malloc(nprocs * sizeof(int));
    /* buf_idx is relevant only if buftype_is_contig.
     * buf_idx[i] gives the index into user_buf where data received
     * from proc. i should be placed. This allows receives to be done
     * without extra buffer. This can't be done if buftype is not contig.
     */

    /* initialize buf_idx to -1 */
    for (i = 0; i < nprocs; i++)
	buf_idx[i] = -1;

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
	    off += avail_len;	/* point to first remaining byte */
	    avail_len = rem_len;	/* save remaining size, pass to calc */
	    proc = ADIOI_LUSTRE_Calc_aggregator(fd, off, &avail_len, striping_info);
	    count_my_req_per_proc[proc]++;
	    rem_len -= avail_len;	/* reduce remaining length by amount from fd */
	}
    }

    /* now allocate space for my_req, offset, and len */
    *my_req_ptr = (ADIOI_Access *) ADIOI_Malloc(nprocs * sizeof(ADIOI_Access));
    my_req = *my_req_ptr;

    count_my_req_procs = 0;
    for (i = 0; i < nprocs; i++) {
	if (count_my_req_per_proc[i]) {
	    my_req[i].offsets = (ADIO_Offset *)
		                ADIOI_Malloc(count_my_req_per_proc[i] *
                                             sizeof(ADIO_Offset));
	    my_req[i].lens = (int *) ADIOI_Malloc(count_my_req_per_proc[i] *
				                  sizeof(int));
	    count_my_req_procs++;
	}
	my_req[i].count = 0;	/* will be incremented where needed later */
    }

    /* now fill in my_req */
    curr_idx = 0;
    for (i = 0; i < contig_access_count; i++) {
	/* short circuit offset/len processing if len == 0
	 *	(zero-byte  read/write */
	if (len_list[i] == 0)
	    continue;
	off = offset_list[i];
	avail_len = len_list[i];
	proc = ADIOI_LUSTRE_Calc_aggregator(fd, off, &avail_len, striping_info);

	/* for each separate contiguous access from this process */
	if (buf_idx[proc] == -1)
	    buf_idx[proc] = (int) curr_idx;

	l = my_req[proc].count;
	curr_idx += (int) avail_len;	/* NOTE: Why is curr_idx an int?  Fix? */

	rem_len = len_list[i] - avail_len;

	/* store the proc, offset, and len information in an array
	 * of structures, my_req. Each structure contains the
	 * offsets and lengths located in that process's FD,
	 * and the associated count.
	 */
	my_req[proc].offsets[l] = off;
	my_req[proc].lens[l] = (int) avail_len;
	my_req[proc].count++;

	while (rem_len != 0) {
	    off += avail_len;
	    avail_len = rem_len;
	    proc = ADIOI_LUSTRE_Calc_aggregator(fd, off, &avail_len,
                                                striping_info);
	    if (buf_idx[proc] == -1)
		buf_idx[proc] = (int) curr_idx;

	    l = my_req[proc].count;
	    curr_idx += avail_len;
	    rem_len -= avail_len;

	    my_req[proc].offsets[l] = off;
	    my_req[proc].lens[l] = (int) avail_len;
	    my_req[proc].count++;
	}
    }

#ifdef AGG_DEBUG
    for (i = 0; i < nprocs; i++) {
	if (count_my_req_per_proc[i] > 0) {
	    FPRINTF(stdout, "data needed from %d (count = %d):\n",
		            i, my_req[i].count);
	    for (l = 0; l < my_req[i].count; l++) {
		FPRINTF(stdout, "   off[%d] = %lld, len[%d] = %d\n",
			        l, my_req[i].offsets[l], l, my_req[i].lens[l]);
	    }
	}
    }
#if 0
    for (i = 0; i < nprocs; i++) {
	FPRINTF(stdout, "buf_idx[%d] = 0x%x\n", i, buf_idx[i]);
    }
#endif
#endif

    *count_my_req_procs_ptr = count_my_req_procs;
    *buf_idx_ptr = buf_idx;
}

int ADIOI_LUSTRE_Docollect(ADIO_File fd, int contig_access_count,
			   int *len_list, int nprocs)
{
    /* If the processes are non-interleaved, we will check the req_size.
     *   if (avg_req_size > big_req_size) {
     *       docollect = 0;
     *   }
     */

    int i, docollect = 1, lflag, big_req_size = 0;
    ADIO_Offset req_size = 0, total_req_size;
    int avg_req_size, total_access_count;

    /* calculate total_req_size and total_access_count */
    for (i = 0; i < contig_access_count; i++)
        req_size += len_list[i];
    MPI_Allreduce(&req_size, &total_req_size, 1, MPI_LONG_LONG_INT, MPI_SUM,
               fd->comm);
    MPI_Allreduce(&contig_access_count, &total_access_count, 1, MPI_INT, MPI_SUM,
               fd->comm);
    /* estimate average req_size */
    avg_req_size = (int)(total_req_size / total_access_count);
    /* get hint of big_req_size */
    big_req_size = fd->hints->fs_hints.lustre.coll_threshold;
    /* Don't perform collective I/O if there are big requests */
    if ((big_req_size > 0) && (avg_req_size > big_req_size))
        docollect = 0;

    return docollect;
}
