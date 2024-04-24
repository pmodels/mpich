/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <adio.h>
#include <limits.h>

/* utility function to query a datatype for its combiner,
 * convenience wrapper around MPI_Type_get_envelope[_c] */

int ADIOI_Type_get_combiner(MPI_Datatype datatype, int *combiner)
{
    int ret;
#if MPI_VERSION >= 4
    MPI_Count ni, na, nc, nt;
    ret = MPI_Type_get_envelope_c(datatype, &ni, &na, &nc, &nt, combiner);
#else
    int ni, na, nt;
    ret = MPI_Type_get_envelope(datatype, &ni, &na, &nt, combiner);
#endif
    return ret;
}

/* utility function to determine whether a datatype is predefined:
 * a datatype is predefined if its combiner is MPI_COMBINER_NAMED
 * or MPI_COMBINER_F90_{INTEGER|REAL|COMPLEX} */

int ADIOI_Type_ispredef(MPI_Datatype datatype, int *flag)
{
    int ret, combiner;
    ret = ADIOI_Type_get_combiner(datatype, &combiner);
    switch (combiner) {
        case MPI_COMBINER_NAMED:
        case MPI_COMBINER_F90_INTEGER:
        case MPI_COMBINER_F90_REAL:
        case MPI_COMBINER_F90_COMPLEX:
            *flag = 1;
            break;
        default:
            *flag = 0;
            break;
    }
    return ret;
}

/* utility function for freeing user-defined datatypes,
 * MPI_DATATYPE_NULL and predefined datatypes are ignored,
 * datatype is set to MPI_DATATYPE_NULL upon return */

int ADIOI_Type_dispose(MPI_Datatype * datatype)
{
    int ret, flag;
    if (*datatype == MPI_DATATYPE_NULL)
        return MPI_SUCCESS;
    ret = ADIOI_Type_ispredef(*datatype, &flag);
    if (ret == MPI_SUCCESS && !flag)
        ret = MPI_Type_free(datatype);
    *datatype = MPI_DATATYPE_NULL;
    return ret;
}

/* utility function for creating large contiguous types: algorithim from BigMPI
 * https://github.com/jeffhammond/BigMPI */

static int type_create_contiguous_x(MPI_Count count, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    /* to make 'count' fit MPI-3 type processing routines (which take integer
     * counts), we construct a type consisting of N INT_MAX chunks followed by
     * a remainder.  e.g for a count of 4000000000 bytes you would end up with
     * one 2147483647-byte chunk followed immediately by a 1852516353-byte
     * chunk */
    MPI_Datatype chunks, remainder;
    MPI_Aint lb, extent, disps[2];
    int blocklens[2];
    MPI_Datatype types[2];

    /* truly stupendously large counts will overflow an integer with this math,
     * but that is a problem for a few decades from now.  Sorry, few decades
     * from now! */
    ADIOI_Assert(count / INT_MAX == (int) (count / INT_MAX));
    int c = (int) (count / INT_MAX);    /* OK to cast until 'count' is 256 bits */
    int r = count % INT_MAX;

    MPI_Type_vector(c, INT_MAX, INT_MAX, oldtype, &chunks);
    MPI_Type_contiguous(r, oldtype, &remainder);

    MPI_Type_get_extent(oldtype, &lb, &extent);

    blocklens[0] = 1;
    blocklens[1] = 1;
    disps[0] = 0;
    disps[1] = c * extent * INT_MAX;
    types[0] = chunks;
    types[1] = remainder;

    MPI_Type_create_struct(2, blocklens, disps, types, newtype);

    MPI_Type_free(&chunks);
    MPI_Type_free(&remainder);

    return MPI_SUCCESS;
}

/* like MPI_Type_create_hindexed, except array_of_lengths can be a larger datatype.
 *
 * Hindexed provides 'count' pairs of (displacement, length), but what if
 * length is longer than an integer?  We will create 'count' types, using
 * contig if length is small enough, or something more complex if not */

int ADIOI_Type_create_hindexed_x(int count,
                                 const MPI_Count array_of_blocklengths[],
                                 const MPI_Aint array_of_displacements[],
                                 MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int i, ret;
    MPI_Datatype *types;
    int *blocklens;
    int is_big = 0;

    types = ADIOI_Malloc(count * sizeof(MPI_Datatype));
    blocklens = ADIOI_Malloc(count * sizeof(int));

    /* squashing two loops into one.
     * - Look in the array_of_blocklengths for any large values
     * - convert MPI_Count items (if they are not too big) into int-sized items
     * after this loop we will know if we can use MPI_type_hindexed or if we
     * need a more complicated BigMPI-style struct-of-chunks.
     *
     * Why not use the struct-of-chunks in all cases?  HDF5 reported a bug,
     * which I have not yet precicesly nailed down, but appears to have
     * something to do with struct-of-chunks when the chunks are small */

    for (i = 0; i < count; i++) {
        if (array_of_blocklengths[i] > INT_MAX) {
            blocklens[i] = 1;
            is_big = 1;
            type_create_contiguous_x(array_of_blocklengths[i], oldtype, &(types[i]));
        } else {
            /* OK to cast: checked for "bigness" above */
            blocklens[i] = (int) array_of_blocklengths[i];
            types[i] = oldtype;
        }
    }

    if (is_big) {
        ret = MPI_Type_create_struct(count, blocklens, array_of_displacements, types, newtype);
        for (i = 0; i < count; i++)
            if (types[i] != oldtype)
                MPI_Type_free(&(types[i]));
    } else {
        ret = MPI_Type_create_hindexed(count, blocklens, array_of_displacements, oldtype, newtype);
    }
    ADIOI_Free(types);
    ADIOI_Free(blocklens);

    return ret;
}

/* some systems do not have pread/pwrite, or requrie XOPEN_SOURCE set higher
 * than we would like.  see #1973 */
#if (HAVE_DECL_PWRITE == 0)

#include <sys/types.h>
#include <unistd.h>

ssize_t pread(int fd, void *buf, size_t count, off_t offset);
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);

ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{
    off_t lseek_ret;
    off_t old_offset;
    ssize_t read_ret;

    old_offset = lseek(fd, 0, SEEK_CUR);
    lseek_ret = lseek(fd, offset, SEEK_SET);
    if (lseek_ret == -1)
        return lseek_ret;
    read_ret = read(fd, buf, count);
    if (read_ret < 0)
        return read_ret;
    /* man page says "file offset is not changed" */
    if ((lseek_ret = lseek(fd, old_offset, SEEK_SET)) < 0)
        return lseek_ret;

    return read_ret;
}

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset)
{
    off_t lseek_ret;
    off_t old_offset;
    ssize_t write_ret;

    old_offset = lseek(fd, 0, SEEK_CUR);
    lseek_ret = lseek(fd, offset, SEEK_SET);
    if (lseek_ret == -1)
        return lseek_ret;
    write_ret = write(fd, buf, count);
    if (write_ret < 0)
        return write_ret;
    /* man page says "file offset is not changed" */
    if ((lseek_ret = lseek(fd, old_offset, SEEK_SET)) < 0)
        return lseek_ret;

    return write_ret;
}
#endif


// Helper function for mapping fp_ind to a location within a view
//
// The goal here is to find a (filetype_index, chunk_index) pair which writes
// to the byte identified as fp, and if a previous write covered part of that
// pair's chunk, set fwr_size to the remainder of the chunk.
//
// I don't think this problem is completely solvable within the current romio design,
// because of the features:
// - the flattened type currently can include 0-byte chunks (for LB/UB
//   if nothing else)
// - the LB/UB portion isn't necessarily in order the way the other chunks
//   are (from where the standard requires the type offsets be non-decreasing)
// - fp_ind is updated to be where the code thinks the next write starts, which
//   it accomplishes by walking to the next type/chunk when it finishes a chunk
//   and using whatever offset it sees there -- even though that could be a
//   LB/UB marker rather than a real data location
//
// I'm reluctant to redefine fp_ind since it's used so many places, but I'd prefer it
// was updated more carefully so it reliably represented the next byte to be written.
// I'd also be very tempted to make the flattened datatype not include any 0-byte
// chunks, but again since I'm not familiar with the full breadth of romio I'm
// reluctant to change that at a high level.
//
// Since currently fp_ind might be located on one of the 0-byte chunks, we can't
// ignore those chunks in our search for the target index pair.

// Regarding implementation, Wei-keng apparently had an optimization over in
// ad_read_str.c but it didn't deal with the weird LB/UB cases. I use the same
// idea here, but not quite as aggressively, so I still have a loop, but the
// loop starts at a higher location rather than blindly starting at 0.

// Update: this last arg prefer_non_zero_chunks lets us approach fp_ind
// piecemeal. I'm causing writestrided to update fp_ind in what I'd consider
// the 'correct' manner so this lookup can be used in a way that expects
// not to find the fp_ind sitting on a zero-sized chunk. Meanwhile it still
// allows for that to happen since the rest of the code likely still does that.

void
ADIOI_fptr_to_view_index(ADIO_Offset fp, ADIOI_Flatlist_node * flat_filetype,
                         MPI_Aint filetype_extent, ADIO_Offset file_view_disp,
                         ADIO_Offset * filetype_index, int *chunk_index,
                         ADIO_Offset * fwr_size, int prefer_non_zero_chunks)
{
    int flag;
    int dt_idx, ch_idx;
    ADIO_Offset a, b;
    ADIO_Offset lowest_offset_in_dt;
    ADIO_Offset highest_offset_in_dt;
    int high_low_offset_unset;
    ADIO_Offset lowest_data_in_dt;
    ADIO_Offset highest_data_in_dt;
    int high_low_data_unset;

    *fwr_size = 0;

    high_low_offset_unset = 1;
    high_low_data_unset = 1;
    for (ch_idx = 0; ch_idx < flat_filetype->count; ch_idx++) {
        a = flat_filetype->indices[ch_idx];
        if (flat_filetype->blocklens[ch_idx]) {
            b = a + flat_filetype->blocklens[ch_idx] - 1;
        } else {
            b = a;
        }

        if (high_low_offset_unset) {
            lowest_offset_in_dt = a;
            highest_offset_in_dt = b;
            high_low_offset_unset = 0;
        }
        if (a < lowest_offset_in_dt) {
            lowest_offset_in_dt = a;
        }
        if (b > highest_offset_in_dt) {
            highest_offset_in_dt = b;
        }

        if (flat_filetype->blocklens[ch_idx]) {
            if (high_low_data_unset) {
                lowest_data_in_dt = a;
                highest_data_in_dt = b;
                high_low_data_unset = 0;
            }
            if (a < lowest_data_in_dt) {
                lowest_data_in_dt = a;
            }
            if (b > highest_data_in_dt) {
                highest_data_in_dt = b;
            }

        }
    }
    if (high_low_data_unset) {
        lowest_data_in_dt = lowest_offset_in_dt;
        highest_data_in_dt = highest_offset_in_dt;
        high_low_data_unset = 0;
    }
// The naive version might start at dt_idx = -1, but we can at least
// moderately short-circuit the loop using Wei-keng's optimization
//
// Stepping through the logic of the below formula, we want the first
// i that could touch the specified byte.  Element i touches a max
// byte of i * extent + highest_data_in_dt so we want the lowest i
// such that i * extent + highest_data_in_dt >= addr
// so i >= (addr - highest_data_in_dt) / extent (could be rounded up)
// eg i >= (addr - highest_data_in_dt + extent - 1) / extent
    if (prefer_non_zero_chunks) {
        dt_idx = (fp - file_view_disp - highest_data_in_dt + filetype_extent - 1)
            / filetype_extent;
    } else {
        dt_idx = (fp - file_view_disp - highest_offset_in_dt + filetype_extent - 1)
            / filetype_extent;
    }
    dt_idx -= 1;        /* the search below starts with ++ */
    if (dt_idx < -1) {
        dt_idx = -1;
    }

    flag = 0;
    while (!flag) {
        dt_idx++;
        for (ch_idx = 0; !flag && ch_idx < flat_filetype->count; ch_idx++) {
            ADIO_Offset chunk_off, chunk_len;
            chunk_off = file_view_disp + dt_idx * (ADIO_Offset) filetype_extent
                + flat_filetype->indices[ch_idx];
            chunk_len = flat_filetype->blocklens[ch_idx];
// The first half of the below 'if' is the correct part if fp_ind
// had a better meaning. But since it could also be the location of
// a 0-byte chunk we allow for that here too.
            if ((fp >= chunk_off && fp < chunk_off + chunk_len)
                || (fp == chunk_off && chunk_len == 0 && !prefer_non_zero_chunks)) {
                *fwr_size = chunk_off + chunk_len - fp;
//printf("  found dt_idx %d and ch_idx %d (bytes left %d)\n", dt_idx, ch_idx, fwr_size);
                flag = 1;
                break;
            }
// The below checks if we've definitively walked beyond any hope of finding
// a match, just to avoid any possibility of an infinite loop
            if (file_view_disp + dt_idx * (ADIO_Offset) filetype_extent + lowest_offset_in_dt > fp) {
                if (prefer_non_zero_chunks) {
                    // go through again, this time allow 0-byte
                    prefer_non_zero_chunks = 0;
                    dt_idx = (fp - file_view_disp - highest_offset_in_dt +
                              filetype_extent - 1) / filetype_extent;
                    if (dt_idx < -1) {
                        dt_idx = -1;
                    }
                } else {
// This probably should be a fatal error
                    flag = -1;
                }
            }
        }
    }

    *filetype_index = dt_idx;
    *chunk_index = ch_idx;
}

// The args for this function are similar to the above (but with a ptr to the current
// file offset (fp) so it can be updated).
//
// This is to be called when we're stepping through a sequence of type_indexes and
// chunks for that type, and we've just finished writing a chunk and are ready to
// updated the file offset pointer to the start of the next relevant chunk. This
// would end up being how fd->fp_ind gets updated.
//
// the returned fwr_size represents what's available to write in the next chunk
// that we picked. The caller likely needs to take the min() of that value and
// whatever is available to write next.
void
ADIOI_fptr_and_view_index_advance(ADIO_Offset * fp,
                                  ADIOI_Flatlist_node * flat_filetype,
                                  MPI_Aint filetype_extent,
                                  ADIO_Offset file_view_disp,
                                  ADIO_Offset * filetype_index,
                                  int *chunk_index, ADIO_Offset * fwr_size)
{
    int dt_idx, ch_idx;
    int done = 0;
    int all_are_empty, advance_the_indexes;

// We'll be trying to advance to a non-empty chunk. But only do that if
// there exist any non-empty to advance to. I suspect it should be an error
// if such a chunk doesn't exist, but for now I just want to avoid an
// infinite loop in such a case. The code this is replacing didn't
// look at the chunk size at all.
    all_are_empty = 1;
    for (ch_idx = 0; ch_idx < flat_filetype->count; ch_idx++) {
        if (flat_filetype->blocklens[ch_idx] > 0) {
            all_are_empty = 0;
        }
    }

    dt_idx = *filetype_index;
    ch_idx = *chunk_index;

    advance_the_indexes = 1;
    while (advance_the_indexes) {
        if (ch_idx < (flat_filetype->count - 1)) {
            ch_idx++;
        } else {
            ch_idx = 0;
            dt_idx++;
        }

        if (flat_filetype->blocklens[ch_idx] > 0 || all_are_empty) {
            advance_the_indexes = 0;
        }
    }

    *fp = file_view_disp + flat_filetype->indices[ch_idx] + dt_idx * (ADIO_Offset) filetype_extent;
    *fwr_size = flat_filetype->blocklens[ch_idx];
    *filetype_index = dt_idx;
    *chunk_index = ch_idx;
}
