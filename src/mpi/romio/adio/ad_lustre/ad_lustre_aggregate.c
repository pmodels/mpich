/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *   Copyright (C) 1997 University of Chicago.
 *   See COPYRIGHT notice in top-level directory.
 *
 *   Copyright (C) 2007 Oak Ridge National Laboratory
 *
 *   Copyright (C) 2008 Sun Microsystems, Lustre group
 */

#include <assert.h>
#include "ad_lustre.h"
#include "adio_extern.h"

#undef AGG_DEBUG

#define FLOOR(a,b) ((a) - (a) % (b))
#define CEILING(a,b) ((a) % (b) > 0 ? (a) / (b) + 1 : (a) / (b))

void ADIOI_LUSTRE_Calc_my_off_len (ADIO_File fd,
                                   int bufcount,
                                   MPI_Datatype datatype,
                                   int file_ptr_type,
                                   ADIO_Offset offset,
                                   ADIOI_LUSTRE_Access *my_access)
{
    int i, j, k;
    int filetype_size=0, buftype_size=0, etype_size=0, st_index=0;
    int filetype_is_contig=0;
    ADIO_Offset n_filetypes=0, etype_in_filetype=0;
    ADIO_Offset bufsize=0, sum=0, n_etypes_in_filetype=0, size_in_filetype=0;
    ADIO_Offset frd_size=0, old_frd_size=0;
    ADIO_Offset abs_off_in_filetype;
    ADIO_Offset off, disp, dist;
    ADIOI_Flatlist_node *flat_file;
    MPI_Aint filetype_extent, filetype_lb;

/* For this process's request, calculate the list of offsets and
 * length in the file and determine the start and end offsets. */
    my_access->end_offset = 0;

    ADIOI_Datatype_iscontig(fd->filetype, &filetype_is_contig);

    MPI_Type_size(fd->filetype, &filetype_size);
    MPI_Type_extent(fd->filetype, &filetype_extent);
    MPI_Type_lb(fd->filetype, &filetype_lb);
    MPI_Type_size(datatype, &buftype_size);
    etype_size = fd->etype_size;
    my_access->buf_size = buftype_size * bufcount;

    if ( ! filetype_size ) {
        my_access->contig_access_count = 0;
        my_access->offset_list = (ADIO_Offset *) ADIOI_Malloc(2*sizeof(ADIO_Offset));
        my_access->len_list = (ADIO_Offset *) ADIOI_Malloc(2*sizeof(ADIO_Offset));
        /* 2 is for consistency. everywhere I malloc one more than needed */

        my_access->offset_list[0] = (file_ptr_type == ADIO_INDIVIDUAL)
                                  ? fd->fp_ind : fd->disp + etype_size * offset;
        my_access->len_list[0]  = 0;
        my_access->start_offset = my_access->offset_list[0];
        my_access->end_offset   = my_access->offset_list[0]
                                + my_access->len_list[0] - 1;
        return;
    }

    if ( filetype_is_contig ) {
        my_access->contig_access_count = 1;
        my_access->offset_list = (ADIO_Offset *) ADIOI_Malloc(2*sizeof(ADIO_Offset));
        my_access->len_list = (ADIO_Offset *) ADIOI_Malloc(2*sizeof(ADIO_Offset));
        /* 2 is for consistency. everywhere I malloc one more than needed */

        my_access->offset_list[0] = (file_ptr_type == ADIO_INDIVIDUAL)
                                  ? fd->fp_ind : fd->disp + etype_size * offset;
        my_access->len_list[0] = bufcount * buftype_size;
        my_access->start_offset = my_access->offset_list[0];
        my_access->end_offset = my_access->offset_list[0]
                              + my_access->len_list[0] - 1;

        /* update file pointer */
        if (file_ptr_type == ADIO_INDIVIDUAL)
            fd->fp_ind = my_access->end_offset + 1;
    }
    else{
        /* First, calculate what size of offset_list and leg_list to allocate */
        /* filetype already falttened in ADIO_Open or ADIO_Fcntl */
        flat_file = ADIOI_Flatten_and_find(fd->filetype);
        disp = fd->disp;

        if ( file_ptr_type == ADIO_INDIVIDUAL ) {
            offset = fd->fp_ind - disp;
            n_filetypes = (offset - flat_file->indices[0]) / filetype_extent;
            offset -= (ADIO_Offset)n_filetypes * filetype_extent; // Now, offset is local to this extent.

            /* find the block where offset is located, skip blocklens[i] == 0 */
            for ( i=0; i<flat_file->count; i++ ) {
                if(flat_file->blocklens[i] == 0) continue;
                dist = flat_file->indices[i] + flat_file->blocklens[i] - offset;

                /* frd_size is from offset to the end of block i */
                if ( dist == 0 ) {
                    i++;
                    offset = flat_file->indices[i];
                    frd_size = flat_file->blocklens[i];
                    break;
                }

                if ( dist > 0 ) {
                    frd_size = dist;
                    break;
                }
            }
            st_index = i; // starting index in flat_file->indices[] */
            offset += disp + (ADIO_Offset)n_filetypes * filetype_extent;
        }
        else {
            n_etypes_in_filetype = filetype_size/etype_size;
            n_filetypes = (int) (offset / n_etypes_in_filetype);
            etype_in_filetype = (int) (offset % n_etypes_in_filetype);
            size_in_filetype = etype_in_filetype * etype_size;

            sum = 0;
            for ( i=0; i<flat_file->count; i++ ) {
                sum += flat_file->blocklens[i];
                if ( sum > size_in_filetype ) {
                    st_index = i;
                    frd_size = sum - size_in_filetype;
                    abs_off_in_filetype = flat_file->indices[i] + 
                                            size_in_filetype - (sum - flat_file->blocklens[i]);
                    break;
                }
            }
        
            /* abs. offset in bytes in the file */
            offset = disp + (ADIO_Offset)n_filetypes * filetype_extent + abs_off_in_filetype;
        }

        /* Calculate how much space to allocate for offset_list and len_list. */
        old_frd_size = frd_size;
        my_access->contig_access_count = i = 0;
        j = st_index;
        bufsize = buftype_size * bufcount;
        frd_size = MPL_MIN(frd_size, bufsize);
        while ( i < bufsize ) {
            if(frd_size) my_access->contig_access_count++;
            i += frd_size;
            j = (j+1)%flat_file->count;
            frd_size = MPL_MIN(flat_file->blocklens[j], bufsize-i);
        }

        /* Allocate space for offset_list and len_list (+1 to avoid 0-size malloc). */
        my_access->offset_list = (ADIO_Offset *) ADIOI_Malloc((my_access->contig_access_count+1)*sizeof(ADIO_Offset));
        my_access->len_list = (ADIO_Offset *) ADIOI_Malloc((my_access->contig_access_count+1)*sizeof(ADIO_Offset));

        /* Find start and end offsets, and fill in offset_list and len_list. */
        my_access->start_offset = offset;
        
        i = k = 0;
        j = st_index;
        off = offset;
        frd_size = MPL_MIN(old_frd_size, bufsize);
        while ( i < bufsize ) {
            if ( frd_size ) {
                my_access->offset_list[k] = off;
                my_access->len_list[k] = frd_size;
                k++;
            }
            i += frd_size;
            my_access->end_offset = off + frd_size - 1;

            /* NOTE: end_offset points to the last byte-offset that will be accessed.
             * e.g., if start_offset is 0 and 100 bytes to be read, end_offset is 99. */
            if ( off + frd_size < disp + flat_file->indices[j] + 
                                flat_file->blocklens[j] + 
                                (ADIO_Offset) n_filetypes*filetype_extent ) {
                off += frd_size;
            }
            else {
                j = (j+1) % flat_file->count;
                n_filetypes += (j == 0)?1:0;
                while ( flat_file->blocklens[j] == 0 ) {
                    j = (j+1) % flat_file->count;
                    n_filetypes += (j == 0)?1:0;
                }
                off = disp + flat_file->indices[j] + (ADIO_Offset) n_filetypes*filetype_extent;
                frd_size = MPL_MIN(flat_file->blocklens[j], bufsize-i);
            }
        }

        /* Update file pointer. */
        if ( file_ptr_type == ADIO_INDIVIDUAL )
            fd->fp_ind = off;
    }
}

void ADIOI_LUSTRE_Calc_file_domains(ADIO_File fd,
                                    int nprocs,
                                    int myrank,
                                    ADIOI_LUSTRE_Access *my_access,
                                    ADIOI_LUSTRE_file_domain *file_domain)
{
   /* Divide the I/O workload among "cb_nodes" processes, named aggregators.
      For most file systems, the division is done evenly and aligned with
      file stripe boundaries. The aggregate access region of this collective
      I/O call is (logically) divided into file domains (FDs) among
      aggregators; each aggregator may directly access only its own file
      domain.

      For Lustre, we use group-cyclic stripe-based file domain partitioning
      methods based on the following paper.
      Wei-keng Liao and Alok Choudhary, "Dynamically Adapting File Domain
      Partitioning Methods for Collective I/O Based on Underlying Parallel
      File System Locking Protocols", SC08.

      The output of this call is file_domain. Each process, both aggregators
      and non-aggregators must possess all the file domain info in order to
      avoid all possible communication cost, such as asking such info from
      aggregators at th run time.
   */

    int i, j, cb_nodes, striping_unit, AAR_num_stripes;
    int first_agg, last_agg, cyclic_nprocs, last_group_id;
    ADIO_Offset cyclic_gap, min_st_offset, max_end_offset;
    ADIO_Offset AAR_st_end_offsets[2];
    ADIO_Offset prev_off;
    int rem_stripes;
    int rem_groups;
    int num_cyclic;
    int num_groups;
    int pivot;
    int buffer_size;

    /* the group-cyclic FD method relies on these 2 values */
    assert(fd->hints->striping_unit   > 0);
    assert(fd->hints->striping_factor > 0);

    striping_unit = fd->hints->striping_unit;
    cb_nodes      = fd->hints->cb_nodes;

#ifdef PIPE
    file_domain->pipelining = 1;
#else
    file_domain->pipelining = 0;
#endif

    /* [Sunwoo] When pipelining two-phase I/O, the buffer size at each round 
     * is limited to the stripe size. So, the number of pipeline stages becomes
     * cb_buffer_size / striping_unit. */
    buffer_size = file_domain->pipelining ? striping_unit : fd->hints->cb_buffer_size;    

    /* find the number of aggregators in each group */
    cyclic_nprocs = MPL_MIN(cb_nodes, fd->hints->striping_factor);

    /* find min of start offsets and max of end offsets of all processes */
#ifdef MPI_IN_PLACE
    AAR_st_end_offsets[0] = - my_access->start_offset;
    AAR_st_end_offsets[1] =   my_access->end_offset;
    MPI_Allreduce(MPI_IN_PLACE,  AAR_st_end_offsets, 2, ADIO_OFFSET,
                  MPI_MAX, fd->comm);
#else
    ADIO_Offset MAR_st_end_offsets[2];
    MAR_st_end_offsets[0] = - my_access->start_offset;
    MAR_st_end_offsets[1] =   my_access->end_offset;
    MPI_Allreduce(MAR_st_end_offsets, AAR_st_end_offsets, 2, ADIO_OFFSET,
                  MPI_MAX, fd->comm);
#endif
    min_st_offset  = -AAR_st_end_offsets[0];
    max_end_offset =  AAR_st_end_offsets[1];

    /* aggregate acess region (AAR) is from min_st_offset to max_end_offset
     * AAR_st_offset will be used only in ADIOI_LUSTRE_Calc_aggregator()
     */
    file_domain->AAR_st_offset = min_st_offset;
    file_domain->AAR_end_offset = max_end_offset;

    /* find the number of file stripes covered by AAR */
    AAR_num_stripes = max_end_offset / striping_unit
                    - min_st_offset  / striping_unit + 1;

    /* num_cyclic is the floor of average number of stripes per aggregator */
    num_cyclic = AAR_num_stripes / cb_nodes;

    /* find the rank of first aggregator of this AAR 
     * [SUNWOO] Since the overall file domain is statically assigned on 
     * the aggregators, the first aggregator is decided by the 
     * minimum access point. */
    first_agg = (min_st_offset / striping_unit) % cyclic_nprocs;

    /* build file domain info for all aggregators */
    file_domain->starts      = (ADIO_Offset*) ADIOI_Malloc(cb_nodes*sizeof(ADIO_Offset));
    file_domain->ends        = (ADIO_Offset*) ADIOI_Malloc(cb_nodes*sizeof(ADIO_Offset));
    file_domain->num_stripes = (int*)         ADIOI_Malloc(cb_nodes*sizeof(int));

    /* assign the floor of the average number of stripes to all aggregators */
    file_domain->my_agg_rank = -1;
    for (i=0; i<cb_nodes; i++) {
        /* each domain at least has this number of stripes */
        file_domain->num_stripes[i] = num_cyclic;
        /* find this process's rank among the aggregators */
        if (fd->hints->ranklist[i] == myrank)
            file_domain->my_agg_rank = i;
        file_domain->starts[i] = -1;
        file_domain->ends[i]   = -1;
    }

    /* rem_stripes is the remainder of stripes over cb_nodes */
    rem_stripes = AAR_num_stripes % cb_nodes;

    /* rem_groups is the number of groups getting extra stripes */
    rem_groups  = rem_stripes / cyclic_nprocs;

    /* all groups up to the last of rem_groups get one extra stripe */
    for (i=0; i<rem_groups * cyclic_nprocs; i++)
        file_domain->num_stripes[i]++;

    /* some aggregators in the last group of rem_groups gets one extra stripe */
    if (rem_stripes % cyclic_nprocs) {
        for (i=0; i<rem_stripes % cyclic_nprocs; i++) {
            j = (i + first_agg) % cyclic_nprocs;
            j += rem_groups * cyclic_nprocs;
            file_domain->num_stripes[j]++;
        }
    }

    /* calculate the start offsets of all file domains */
    file_domain->starts[first_agg] = min_st_offset;
    prev_off = FLOOR(min_st_offset, striping_unit);
    for (i=1; i<cyclic_nprocs; i++) { /* first group */
        j = (i + first_agg) % cyclic_nprocs;
        if (file_domain->num_stripes[j] > 0)
            file_domain->starts[j] = prev_off + striping_unit;
        prev_off += striping_unit;
    }

    /* calculate start offsets for rest of groups 
     * the start offsets of a group depend on the previous group
     */
    for (i=1; i<cb_nodes/cyclic_nprocs; i++) {
        for (j=0; j<cyclic_nprocs; j++) {
            int me = j + i * cyclic_nprocs;
            int prev = me - cyclic_nprocs;
            prev_off = FLOOR(file_domain->starts[prev], striping_unit);
            if (file_domain->num_stripes[me] > 0) {
                /* to prevent 4byte int overflow */
                ADIO_Offset gap = file_domain->num_stripes[prev];
                file_domain->starts[me] = prev_off
                                        + gap * striping_unit * cyclic_nprocs;
            }
        }
    }

    /* calculate the end offsets of all file domains */
    for (i=0; i<cb_nodes; i++)
        if (file_domain->num_stripes[i] > 0) {
            ADIO_Offset gap = file_domain->num_stripes[i] - 1;
            file_domain->ends[i] = FLOOR(file_domain->starts[i], striping_unit)
                                 + gap * striping_unit * cyclic_nprocs
                                 + striping_unit - 1;
        }

    last_agg = (max_end_offset / striping_unit) % cyclic_nprocs;
    num_groups = AAR_num_stripes / cyclic_nprocs;
    if (AAR_num_stripes % cyclic_nprocs == 0) num_groups--;
    if (first_agg + AAR_num_stripes >= cb_nodes)
        last_agg += cb_nodes - cyclic_nprocs;
    else
        last_agg += num_groups * cyclic_nprocs;
    file_domain->AAR_end_rank   = last_agg;
    file_domain->ends[last_agg] = max_end_offset;

    if (rem_stripes % cyclic_nprocs)
        rem_groups++;

    /* pivot is the first stripe ID (relative to this AAR) in of the first
       group that has no extra stripe assigned to any of aggregators in
       the group, due to unvivisible number of stripes to cb_nodes
     */
    pivot = 0;
    if (rem_stripes > 0) {
        pivot = (num_cyclic+1) * cyclic_nprocs * rem_groups;
        if (rem_stripes % cyclic_nprocs)
            pivot -= (cyclic_nprocs - AAR_num_stripes % cyclic_nprocs);
    }

    file_domain->num_stripes_per_group = num_cyclic * cyclic_nprocs;
    file_domain->pivot = pivot;
    file_domain->rem_groups = rem_groups;

    file_domain->AAR_st_rank = (min_st_offset/striping_unit) % cb_nodes;

    /* gap between two consecutive stripes in an aggregator's file domain */
    cyclic_gap = (ADIO_Offset)striping_unit * (cyclic_nprocs-1);

    /* divide the two-phase I/O into ntimes steps
       For aggregator i, each step j handles file sub-domain from
       offsets[i][j] for amount of sizes[i][j] and sizes[i][j] is
       not coalesced
     */
    file_domain->ntimes  = (int*)          ADIOI_Malloc(cb_nodes * sizeof (int));
    file_domain->offsets = (ADIO_Offset**) ADIOI_Malloc(cb_nodes * sizeof (ADIO_Offset*));
    file_domain->sizes   = (ADIO_Offset**) ADIOI_Malloc(cb_nodes * sizeof (ADIO_Offset*));

    /* for each aggregator build the sub-domains
       each process must have this info locally, same across all processes
     */
    file_domain->fd_size = 0;
    for (i=0; i<cb_nodes; i++) {
        int         ntimes;
        ADIO_Offset fd_size;

        if (file_domain->num_stripes[i] == 0) {
            /* some aggregators have zero-size file domains,
               this happens when the AAR is small
             */
            file_domain->ntimes[i] = 0;
            file_domain->starts[i] = -1;
            file_domain->ends[i]   = -1;
            continue;
        }

        /* fd_size is the size of coalesced file stripes in aggregator i's
           file domain
         */
        fd_size = file_domain->ends[i]+1 - file_domain->starts[i]
                - cyclic_gap * (file_domain->num_stripes[i]-1);

        /* keep the one for this aggregator */
        if (fd->hints->ranklist[i] == myrank)
            file_domain->fd_size = fd_size;
        /* divide fd_size by coll_bufsize to determine how many two-phase I/O */

        ntimes = (fd_size + buffer_size - 1) / buffer_size;
        file_domain->ntimes[i] = ntimes;
        if (ntimes == 0) continue; /* AAR is small */

        /* offsets[i] and sizes[i] indicate the sub-domain for iteration i of
         * ntimes two-phase I/O
         */
        file_domain->offsets[i] = (ADIO_Offset*) ADIOI_Malloc(ntimes * sizeof(ADIO_Offset));
        file_domain->sizes[i]   = (ADIO_Offset*) ADIOI_Malloc(ntimes * sizeof(ADIO_Offset));

        file_domain->offsets[i][0] = file_domain->starts[i];
        for (j=1; j<ntimes; j++) {
            ADIO_Offset rem = MPL_MIN(fd_size, buffer_size);
            fd_size -= rem;
            file_domain->offsets[i][j] = file_domain->offsets[i][j-1];
            file_domain->sizes[i][j-1] = 0;
            while (rem > 0) {
                int stripe_rem = striping_unit
                               - file_domain->offsets[i][j] % striping_unit;
                if (rem > stripe_rem) {
                    /* expand to next stripe */
                    file_domain->offsets[i][j] += cyclic_gap + stripe_rem;
                    file_domain->sizes[i][j-1] = file_domain->offsets[i][j]
                                               - file_domain->offsets[i][j-1];
                    rem -= stripe_rem;
                }
                else { /* this is the last stripe */
                    if (rem == stripe_rem) file_domain->offsets[i][j] += cyclic_gap;
                    file_domain->offsets[i][j] += rem;
                    file_domain->sizes[i][j-1] += rem;
                    rem = 0;
                }
            }
        }
        file_domain->sizes[i][ntimes-1] = file_domain->ends[i]+1 - file_domain->offsets[i][ntimes-1];
        /* file_domain->sizes[i][] is across the stripe gaps in absolute file byte offset */
    }
}

void ADIOI_LUSTRE_Get_striping_info(ADIO_File fd, int **striping_info_ptr, int mode)
{
    int *striping_info = NULL;
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

    /* [Sunwoo] If avail_cb_nodes < fd->hints->cb_nodes, 
     * fd->is_agg should also be adjusted. */
    if (avail_cb_nodes < fd->hints->cb_nodes) {
        fd->hints->cb_nodes = avail_cb_nodes;
        fd->is_agg = 0;
        for (int i=0; i<avail_cb_nodes; i++) {
            if (myrank == fd->hints->ranklist[i])
                fd->is_agg = 1;
        }
    }

    *striping_info_ptr = (int *) ADIOI_Malloc(3 * sizeof(int));
    striping_info = *striping_info_ptr;
    striping_info[0] = stripe_size;
    striping_info[1] = stripe_count;
    striping_info[2] = avail_cb_nodes;
}

int ADIOI_LUSTRE_Calc_aggregator(ADIO_File fd, ADIO_Offset off,
                                 ADIOI_LUSTRE_file_domain *file_domain,
                                 ADIO_Offset * len, int *striping_info)
{
    int rank_index, rank;
    ADIO_Offset avail_bytes;
    int stripe_size = striping_info[0];
    int striping_factor = striping_info[1];
    int avail_cb_nodes = striping_info[2];
    int pivot;
    int rem_groups;
    int stripe_id;
    int group_index;
    int num_stripes_per_group;
    ADIO_Offset min_off;
    ADIO_Offset local_off;

    if(avail_cb_nodes > striping_factor){
        pivot                   = file_domain->pivot;
        rem_groups              = file_domain->rem_groups;
        min_off                 = file_domain->AAR_st_offset;
        num_stripes_per_group   = file_domain->num_stripes_per_group;
        
        /* local_off is relative to the beginning of the AAR */
        local_off = off - FLOOR(min_off, stripe_size);

        /* stripe_id is relative to this AAR */
        stripe_id = local_off / stripe_size;

        /* use pivot to determine which group this stripe ID belongs to */
        if(stripe_id < pivot){
            group_index = stripe_id / (striping_factor + num_stripes_per_group);
        }
        else{
            assert(num_stripes_per_group > 0);
            group_index = (stripe_id - pivot) / num_stripes_per_group;
            group_index += rem_groups;
        }

        /* make local_off relative to the server's stripe_size */
        local_off %= (striping_factor * stripe_size);

        /* local stripe id */
        rank_index = local_off / stripe_size;
        
        /* make the stripe id aligned with the first stripe id */
        rank_index += (min_off / stripe_size) % striping_factor;
        rank_index %= striping_factor;

        /* add the group id back */
        rank_index += group_index * striping_factor;
    }
    else{
        /* fall back to static-cyclic method if avail_cb_nodes <= striping_factor */
        rank_index = (int) ((off / stripe_size) % avail_cb_nodes);
    }

    /* we index into fd_end with rank_index, and fd_end was allocated to be no
     * bigger than fd->hins->cb_nodes.   If we ever violate that, we're
     * overrunning arrays.  Obviously, we should never ever hit this abort
     */
    if (rank_index >= avail_cb_nodes)
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
                              ADIOI_LUSTRE_file_domain *file_domain,
                              ADIOI_Access ** my_req_ptr, ADIO_Offset *** buf_idx_ptr)
{
    /* Nothing different from ADIOI_Calc_my_req(), except calling
     * ADIOI_Lustre_Calc_aggregator() instead of the old one */
    int *count_my_req_per_proc, count_my_req_procs;
    int i, l, proc;
    ADIO_Offset avail_len, rem_len, curr_idx, off, **buf_idx;
    ADIOI_Access *my_req;

    *count_my_req_per_proc_ptr = (int *) ADIOI_Calloc(nprocs, sizeof(int));
    count_my_req_per_proc = *count_my_req_per_proc_ptr;
    /* count_my_req_per_proc[i] gives the no. of contig. requests of this
     * process in process i's file domain. calloc initializes to zero.
     * I'm allocating memory of size nprocs, so that I can do an
     * MPI_Alltoall later on.
     */

    buf_idx = (ADIO_Offset **) ADIOI_Malloc(nprocs * sizeof(ADIO_Offset *));

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
        proc = ADIOI_LUSTRE_Calc_aggregator(fd, off, file_domain, &avail_len, striping_info);
        count_my_req_per_proc[proc]++;

        /* figure out how many data is remaining in the access
         * we'll take care of this data (if there is any)
         * in the while loop below.
         */
        rem_len = len_list[i] - avail_len;

        while (rem_len != 0) {
            off += avail_len;   /* point to first remaining byte */
            avail_len = rem_len;        /* save remaining size, pass to calc */
            proc = ADIOI_LUSTRE_Calc_aggregator(fd, off, file_domain, &avail_len, striping_info);
            count_my_req_per_proc[proc]++;
            rem_len -= avail_len;       /* reduce remaining length by amount from fd */
        }
    }

    /* buf_idx is relevant only if buftype_is_contig.
     * buf_idx[i] gives the index into user_buf where data received
     * from proc 'i' should be placed. This allows receives to be done
     * without extra buffer. This can't be done if buftype is not contig.
     */

    /* initialize buf_idx vectors */
    for (i = 0; i < nprocs; i++) {
        /* add one to count_my_req_per_proc[i] to avoid zero size malloc */
        buf_idx[i] = (ADIO_Offset *) ADIOI_Malloc((count_my_req_per_proc[i] + 1)
                                                  * sizeof(ADIO_Offset));
    }

    /* now allocate space for my_req, offset, and len */
    *my_req_ptr = (ADIOI_Access *) ADIOI_Malloc(nprocs * sizeof(ADIOI_Access));
    my_req = *my_req_ptr;

    count_my_req_procs = 0;
    for (i = 0; i < nprocs; i++) {
        if (count_my_req_per_proc[i]) {
            my_req[i].offsets = (ADIO_Offset *)
                ADIOI_Malloc(count_my_req_per_proc[i] * 2 * sizeof(ADIO_Offset));
            my_req[i].lens = my_req[i].offsets + count_my_req_per_proc[i];
            my_req[i].mem_ptrs = (MPI_Aint *) ADIOI_Malloc(count_my_req_per_proc[i] * sizeof(MPI_Aint));
            count_my_req_procs++;
        }
        my_req[i].count = 0;    /* will be incremented where needed later */
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
        proc = ADIOI_LUSTRE_Calc_aggregator(fd, off, file_domain, &avail_len, striping_info);

        l = my_req[proc].count;

        ADIOI_Assert(l < count_my_req_per_proc[proc]);
        buf_idx[proc][l] = curr_idx;
        my_req[proc].mem_ptrs[l] = curr_idx;
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
            proc = ADIOI_LUSTRE_Calc_aggregator(fd, off, file_domain, &avail_len, striping_info);

            l = my_req[proc].count;
            ADIOI_Assert(l < count_my_req_per_proc[proc]);
            buf_idx[proc][l] = curr_idx;
            my_req[proc].mem_ptrs[l] = curr_idx;

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
                        l, my_req[i].offsets[l], l, my_req[i].lens[l]);
            }
        }
    }
#endif

    *count_my_req_procs_ptr = count_my_req_procs;
    *buf_idx_ptr = buf_idx;
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

void ADIOI_LUSTRE_Calc_others_req(ADIO_File      fd,
                                  int            nprocs,
                                  int            myrank,
                                  int           *count_my_req_per_proc,
                                  ADIOI_Access  *my_req, 
                                  ADIOI_Access **others_req_ptr)  
{
    int *count_others_req_per_proc;
    int i, j, total_count;
    ADIOI_Access *others_req;
#ifdef _USE_ALLTOALLV_
    int *send_count, *recv_count, *sdispls, *rdispls;
#else
    MPI_Request *requests;
    MPI_Status *statuses;
#endif

    /* first find out how much to send/recv and from/to whom */

    /* count_others_req_per_proc[i] indicates how many non-contiguous
       requests of process i lie in this process's file domain. */
    count_others_req_per_proc = (int *) ADIOI_Malloc(nprocs*sizeof(int));

#define USE_ALLTOALL
    /* On Hopper@NERSC, using MPI_Alltoall for an integer below is much better
     * than isend/irecv, particularly when running 10K or more processes, even
     * though this communication is actually an all-to-some pattern
     */
#ifdef USE_ALLTOALL
    MPI_Alltoall(count_my_req_per_proc, 1, MPI_INT,
		 count_others_req_per_proc, 1, MPI_INT, fd->comm);
#else
    for (i=0; i<nprocs; i++) count_others_req_per_proc[i] = 0;

    requests = (MPI_Request *) ADIOI_Malloc((nprocs+fd->hints->cb_nodes)*sizeof(MPI_Request));

    /* only I/O aggregators post irecv from all processes */
    j = 0;
    if (fd->is_agg) {
        for (i=0; i<nprocs; i++) /* get from proc i on how much this aggregator will receive */
            MPI_Irecv(&count_others_req_per_proc[i], 1, MPI_INT, i, i+myrank, fd->comm, &requests[j++]);
    }

    /* all processes send their own request info to I/O aggregators */
    for (i=0; i<fd->hints->cb_nodes; i++) {
        int aggRank = fd->hints->ranklist[i];
        MPI_Isend(&count_my_req_per_proc[aggRank], 1, MPI_INT, aggRank, aggRank+myrank, fd->comm, &requests[j++]);
    }

    statuses = (MPI_Status *) ADIOI_Malloc(j * sizeof(MPI_Status));
    MPI_Waitall(j, requests, statuses);
    ADIOI_Free(statuses);
    ADIOI_Free(requests);
#endif

    /* array size IS nprocs as all other processes in the open communicator */
    *others_req_ptr = (ADIOI_Access *) ADIOI_Malloc(nprocs*sizeof(ADIOI_Access)); 
    others_req = *others_req_ptr;

    total_count = 1; /* avoid zero size malloc */
    for (i=0; i<nprocs; i++) {
        others_req[i].count  = count_others_req_per_proc[i];
        total_count         += count_others_req_per_proc[i];
    }

    others_req[0].offsets  = (ADIO_Offset *) ADIOI_Malloc(total_count*sizeof(ADIO_Offset));
    others_req[0].lens     = (ADIO_Offset *) ADIOI_Malloc(total_count*sizeof(ADIO_Offset)); 
    others_req[0].mem_ptrs = (MPI_Aint *)    ADIOI_Malloc(total_count*sizeof(MPI_Aint)); 
    for (i=1; i<nprocs; i++) {
        others_req[i].offsets  = others_req[i-1].offsets  + others_req[i-1].count;
        others_req[i].lens     = others_req[i-1].lens     + others_req[i-1].count;
        others_req[i].mem_ptrs = others_req[i-1].mem_ptrs + others_req[i-1].count;
    }
    
/* now send the calculated offsets and lengths to respective processes */
#ifdef _USE_ALLTOALLV_
    /* On Hopper@NERSC, using MPI_Alltoallv below is worse than
     * isend/irecv, particularly when running 10K or more processes
     */
    send_count = (int*) ADIOI_Malloc(4 * nprocs * sizeof(int));
    recv_count = send_count + nprocs;
    sdispls    = recv_count + nprocs;
    rdispls    = sdispls    + nprocs;

    send_count[0] =     my_req[0].count;
    recv_count[0] = others_req[0].count;
    sdispls[0] = rdispls[0] = 0;
    for (i=1; i<nprocs; i++) {
        send_count[i] =     my_req[i].count;
        recv_count[i] = others_req[i].count;
        sdispls[i] = sdispls[i-1] + send_count[i-1];
        rdispls[i] = rdispls[i-1] + recv_count[i-1];
    }

    MPI_Alltoallv(    my_req[0].offsets, send_count, sdispls, ADIO_OFFSET,
                  others_req[0].offsets, recv_count, rdispls, ADIO_OFFSET, fd->comm);
    MPI_Alltoallv(    my_req[0].lens,    send_count, sdispls, ADIO_OFFSET,
                  others_req[0].lens,    recv_count, rdispls, ADIO_OFFSET, fd->comm);
/* wkliao: again, this is not truely all-to-all, as the only non-zero length
   receivers are aggregators
*/

    ADIOI_Free(send_count);
#else
    total_count = 1; /* +1 to avoid a 0-size malloc */
    for (i=0; i<nprocs; i++) {
        if (i != myrank) {
            if (others_req[i].count) total_count++;
            if (    my_req[i].count) total_count++;
        }
    }

#define COMBINE_OFFSETS_LENS
#ifndef COMBINE_OFFSETS_LENS
    total_count *= 2;
#endif
    requests = (MPI_Request *) ADIOI_Malloc(total_count*sizeof(MPI_Request));

    j = 0;
    for (i=0; i<nprocs; i++) {
        if (i == myrank) {
            if (others_req[i].count) {
                memcpy(others_req[i].offsets, my_req[i].offsets,
                       others_req[i].count * sizeof(ADIO_Offset));
                memcpy(others_req[i].lens,    my_req[i].lens,
                       others_req[i].count * sizeof(ADIO_Offset));
            }
        }
        else if (others_req[i].count) {
#ifdef COMBINE_OFFSETS_LENS
            MPI_Aint addr1, addr2, disps[2];
            MPI_Datatype buf_type;
            int blocklengths[2];

            /* construct a derived data type to combine two receives */
            blocklengths[0] = others_req[i].count * sizeof(ADIO_Offset);
            blocklengths[1] = others_req[i].count * sizeof(ADIO_Offset);
            MPI_Get_address(others_req[i].offsets, &addr1);
            MPI_Get_address(others_req[i].lens,    &addr2);
            disps[0] = 0;
            disps[1] = addr2 - addr1;
            MPI_Type_create_hindexed(2, blocklengths, disps, MPI_BYTE, &buf_type);
            MPI_Type_commit(&buf_type);
            MPI_Irecv(others_req[i].offsets, 1, buf_type,
                      i, i+myrank, fd->comm, &requests[j++]);
            MPI_Type_free(&buf_type);
#else
            MPI_Irecv(others_req[i].offsets, others_req[i].count,
                      ADIO_OFFSET, i, i+myrank, fd->comm, &requests[j]);
            j++;
            MPI_Irecv(others_req[i].lens, others_req[i].count,
                      ADIO_OFFSET, i, i+myrank+1, fd->comm, &requests[j]);
            j++;
#endif
        }
    }

    for (i=0; i<nprocs; i++) {
        if (i != myrank && my_req[i].count) {
#ifdef COMBINE_OFFSETS_LENS
            MPI_Aint addr1, addr2, disps[2];
            MPI_Datatype buf_type;
            int blocklengths[2];

            /* construct a derived data type to combine two sends */
            blocklengths[0] = my_req[i].count * sizeof(ADIO_Offset);
            blocklengths[1] = my_req[i].count * sizeof(ADIO_Offset);
            MPI_Get_address(my_req[i].offsets, &addr1);
            MPI_Get_address(my_req[i].lens,    &addr2);
            disps[0] = 0;
            disps[1] = addr2 - addr1;
            MPI_Type_create_hindexed(2, blocklengths, disps, MPI_BYTE, &buf_type);
            MPI_Type_commit(&buf_type);
            MPI_Isend(my_req[i].offsets, 1, buf_type,
                      i, i+myrank, fd->comm, &requests[j++]);
            MPI_Type_free(&buf_type);
#else
            MPI_Isend(my_req[i].offsets, my_req[i].count,
                      ADIO_OFFSET, i, i+myrank, fd->comm, &requests[j]);
            j++;
            MPI_Isend(my_req[i].lens, my_req[i].count,
                      ADIO_OFFSET, i, i+myrank+1, fd->comm, &requests[j]);
            j++;
#endif
        }
    }

    if (j) {
        statuses = (MPI_Status *) ADIOI_Malloc(j * sizeof(MPI_Status));
        MPI_Waitall(j, requests, statuses);
        ADIOI_Free(statuses);
    }

    ADIOI_Free(requests);
#endif
    ADIOI_Free(count_others_req_per_proc);
}
