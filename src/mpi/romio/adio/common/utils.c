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

/* In an MPI-4 "Large Count" world do we even need this routine any longer? */
int ADIOI_Type_create_hindexed_x(MPI_Count count,
                                 const MPI_Count array_of_blocklengths[],
                                 const MPI_Count array_of_displacements[],
                                 MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int i, ret;
    MPI_Datatype *types;
    MPI_Count *blocklens;
    int is_big = 0;

    types = ADIOI_Malloc(count * sizeof(MPI_Datatype));
    blocklens = ADIOI_Malloc(count * sizeof(*blocklens));

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
        ret = MPI_Type_create_struct_c(count, blocklens, array_of_displacements, types, newtype);
        for (i = 0; i < count; i++)
            if (types[i] != oldtype)
                MPI_Type_free(&(types[i]));
    } else {
        ret = MPI_Type_create_hindexed_c(count, blocklens, array_of_displacements, oldtype, newtype);
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

void ADIOI_Heap_merge(ADIOI_Access * others_req, MPI_Count *count,
                      ADIO_Offset * srt_off, MPI_Count *srt_len, MPI_Count *start_pos,
                      int nprocs, int nprocs_recv, MPI_Count total_elements)
{
    typedef struct {
        ADIO_Offset *off_list;
        ADIO_Offset *len_list;
        MPI_Count nelem;
    } heap_struct;

    heap_struct *a, tmp;
    int i, j, heapsize, l, r, k, smallest;

    a = (heap_struct *) ADIOI_Malloc((nprocs_recv + 1) * sizeof(heap_struct));

    j = 0;
    for (i = 0; i < nprocs; i++)
        if (count[i]) {
            a[j].off_list = &(others_req[i].offsets[start_pos[i]]);
            a[j].len_list = &(others_req[i].lens[start_pos[i]]);
            a[j].nelem = count[i];
            j++;
        }

    /* build a heap out of the first element from each list, with
     * the smallest element of the heap at the root */

    heapsize = nprocs_recv;
    for (i = heapsize / 2 - 1; i >= 0; i--) {
        /* Heapify(a, i, heapsize); Algorithm from Cormen et al. pg. 143
         * modified for a heap with smallest element at root. I have
         * removed the recursion so that there are no function calls.
         * Function calls are too expensive. */
        k = i;
        for (;;) {
            l = 2 * (k + 1) - 1;
            r = 2 * (k + 1);

            if ((l < heapsize) && (*(a[l].off_list) < *(a[k].off_list)))
                smallest = l;
            else
                smallest = k;

            if ((r < heapsize) && (*(a[r].off_list) < *(a[smallest].off_list)))
                smallest = r;

            if (smallest != k) {
                tmp.off_list = a[k].off_list;
                tmp.len_list = a[k].len_list;
                tmp.nelem = a[k].nelem;

                a[k].off_list = a[smallest].off_list;
                a[k].len_list = a[smallest].len_list;
                a[k].nelem = a[smallest].nelem;

                a[smallest].off_list = tmp.off_list;
                a[smallest].len_list = tmp.len_list;
                a[smallest].nelem = tmp.nelem;

                k = smallest;
            } else
                break;
        }
    }

    for (i = 0; i < total_elements; i++) {
        /* extract smallest element from heap, i.e. the root */
        srt_off[i] = *(a[0].off_list);
        srt_len[i] = *(a[0].len_list);
        (a[0].nelem)--;

        if (!a[0].nelem) {
            a[0].off_list = a[heapsize - 1].off_list;
            a[0].len_list = a[heapsize - 1].len_list;
            a[0].nelem = a[heapsize - 1].nelem;
            heapsize--;
        } else {
            (a[0].off_list)++;
            (a[0].len_list)++;
        }

        /* Heapify(a, 0, heapsize); */
        k = 0;
        for (;;) {
            l = 2 * (k + 1) - 1;
            r = 2 * (k + 1);

            if ((l < heapsize) && (*(a[l].off_list) < *(a[k].off_list)))
                smallest = l;
            else
                smallest = k;

            if ((r < heapsize) && (*(a[r].off_list) < *(a[smallest].off_list)))
                smallest = r;

            if (smallest != k) {
                tmp.off_list = a[k].off_list;
                tmp.len_list = a[k].len_list;
                tmp.nelem = a[k].nelem;

                a[k].off_list = a[smallest].off_list;
                a[k].len_list = a[smallest].len_list;
                a[k].nelem = a[smallest].nelem;

                a[smallest].off_list = tmp.off_list;
                a[smallest].len_list = tmp.len_list;
                a[smallest].nelem = tmp.nelem;

                k = smallest;
            } else
                break;
        }
    }
    ADIOI_Free(a);
}
