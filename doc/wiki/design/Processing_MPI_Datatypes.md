# Processing MPI Datatypes

This page contains the beginning of information about using the MPICH
Segment code to handle MPI Datatypes.

## Pack and Unpack

(The following was extracted from a message from Rob Ross about
`MPID_Segment_pack` and `MPID_Segment_unpack`).

The first and last parameters correspond to locations in a logical
stream of bytes that is the serialized data. The `tmpbuf` is a pointer
to the beginning of that data, which is assumed to be in a contiguous
region. It is not a pointer to the beginning of the logical stream of
bytes, because you might not have all the bytes at any given time.

So to properly handle the noncontig case, you basically need to (a)
receive the data into some number of buffers, (b) call
`MPID_Segment_init()` once, and (c) call `MPID_Segment_unpack()` once
for each contiguous temporary buffer that you received data into, using
the right first and last parameters to describe what part of the logical
stream of bytes are contained in that particular temporary buffer.

To enable more extensive debugging output, you will definitely need to
go into `mpid_segment.c` and define `MPID_SU_VERBOSE` and
`MPID_SP_VERBOSE` at the top of the file to enable messages about
pack/unpack. They will have the following fields:

```
contig unpack: do=%d, dp=%x, bp=%x, sz=%d, blksz=%d

do = offset from start of data
dp = pointer to data
bp = buffer pointer
sz = size of a dataloop element (single type)
blksz = count of dataloop elements
```

Vector unpacks will be similar, but will add:

```
str = stride (distance from start of first block of elements to start of next block, in bytes)
blks = number of blocks of elements
```

The blockindexed and indexed routines don't currently have debugging
statements. You can change `MPID_Segment_blkidx_pack_to_buf` and
`MPID_Segment_index_pack_to_buf` to `NULL` in `MPID_Segment_pack` (or
similar in `MPID_Segment_unpack`) to eliminate the use of these
functions if you're testing with indexed, struct, or blockindexed types
and don't see debugging output. They are extremely unlikely to be the
source of your bug, and can be safely re-enabled once you've got things
working right with just vector and contig. For that matter, you could
NULL out the vector processing functions and stick with just contig
copies, for simplicity.
