# DAME

## DAME: A Data Manipulation Engine for MPI

DAME is an efficient representation of the data movements defined by an
MPI datatype. The representation may be optimized; that is, it may be
different from the user's definition of the datatype; for example, a
struct of different datatypes may be replaced by a contiguous block.
Think of it as a simple program that can be executed efficiently by the
MPI implementation.

In the most simple implementation, an interpreter, using code that is
optimized for the platform, executes the DAME description. An additional
optimization may generate native object code at runtime that is executed
directly.

**Issues and Implications:**

1\. An MPI datatype may specify a large amount of data, larger than can
be sent in a single message. Thus the datatype implementation must
handle "partial messages". However, it is not necessary to allow
arbitrary buffer sizes (as in very small) nor is it necessary to require
that the buffer be exactly filled (need not break an int across two
buffers). An important special case is that where everything fits within
a single buffer. In this case, there should be no extra overhead.

Question: If there was a single, known-sized maximum buffer length, the
DAME "program" could be arranged at datatype commit time for that size,
simplifying the code for handling longer buffers. Is this desirable?

Another consequence of this is that it is important to have a compact
way to describe the state of a partial move, and to be able to continue
from the last position.

2\. Despite claims to the contrary, `memcpy` is \*not\* always faster than
a simple loop - anyone who claims otherwise should replace all `i=j`
with `memcpy(&i,&j,sizeof(int))` in their code. Thus, it is important
to avoid assuming that all block moves are long.

An implication of this is that it may make sense to have different
"instructions" for short and not-short block moves within the simple
instruction set. Similarly, when alignment is easily computed and
preserved by the move, that may need to be a separate instruction.

3\. Large datatypes and efficiency. It is essential to be able to handle
data larger than 2/4 GBytes. However, arithmetic with 8 byte integers
may be slower (and in the case of indexed datatypes, double the memory
pressure in the datatype description). A careful handling of sizes and
counts is needed to ensure both efficiency in the common (less than
2GByte) case and correctness when the data is larger in size.

One possible mechanism is to have short (4 byte) and long (8 byte)
versions of the data move instructions, permitting the optimization of
the move code.

4\. It is common for MPI datatypes to describe large collections of
relatively small blocks. For good performance, there must be nearly zero
overhead in dealing with the blocks. This means that the code for
executing the data moves must be very streamlined, with little runtime
overhead. Tests, as much as possible, must be hoisted out of the
execution of the moves.

5\. Error checking. The current code does a lot of checking at runtime,
every time the dataloop is used. In many cases, these may be checked at
`Type_commit` time, or used only in debugging modes. This is related to
point 4.

6\. Alignment. While user data may be required to be aligned as the
underlying type, this is not the case for `MPI_PACKED` data used with the
`MPI_Pack/MPI_Unpack` and related routines. One possibility is to
require user and internal data buffers to have matching alignment and to
add, for example, 8 bytes to the packed size, with a pack buffer
consisting of a byte indicating how many bytes (including the first) to
skip to get to the first aligned byte of a packed buffer. Question: can
packed buffers be copied by the user to different storage, which would
break this approach?

**Rough Plan:**

Define a data move language that matches both the MPI datatypes and the
capabilities of the hardware. A DAME is a representation of the moves
defined by an MPI datatype in this language. Optimizations can be
applied to this representation to produce equivalent moves but with
higher performance (the current `dataloop_optimize` performs simple
optimizations). For most MPI datatypes (all except complex ones
involving `STRUCT`), the "program" can be represented as an stack, with
each stack element preloaded with the move "instruction". Storing a
partial move requires only remember the position in the stack and the
progress within that instruction.

For the move code itself, a simple interpreter that implements a loop
over items, and where the loop has been written (possibly by an
autotuning tool) to provide good performance, should usually be
sufficient to get good performance. For more general cases, runtime
generation of native code is possible. For important special cases, we
could optimize for those cases.

Finally, given this "DAME program", it should be possible to take
advantage of hardware scatter/gather support.

### DAME design principles

The DAME design follows these principles:

1\. The DAME data manipulation should be able to take advantage of the
types and alignments of the lowest-level datatype elements. For example,
if the derived datatype only consists of integers, we know at type
commit time that all data movement operations can be done as assignments
of integers if so desired.

2\. All datatype manipulation operations (packing/unpacking) should be
done in as many tight loops as possible. For example, for simple
datatypes like vector-of-vector-of-integers, the resultant code should
be essentially a three-level for loop at the most (with the lowest level
for loop doing integer-wise assignments instead of memcpy for small
contiguous regions). Many of the for loop structures for simple
datatypes can be generated at compile time. This helps compilers
vectorize the data movement operations. For complex datatypes, a slower
path might be OK.

### Code structure

To be added.

**The DAME Programming Language**

Data moves are described in a simple programming language. Code in this
language can be executed by an interpreter or compiled into optimized
code. Because MPI datatypes, while general, describe only unconditional
moves, the language is very simple. We also assume that descriptions are
in terms of bytes and that no data conversions are required.

To begin with, there are these building blocks:

1.  `CONTIG` - n contiguous copies of the base type (which may be another
    of these building blocks). Contiguous is defined in terms of extent,
    not size (just as in MPI)
2.  `VECTOR` - n strided copies of the base type.
3.  `INDEXED` - Like the MPI indexed type. A Unix IOV looks like this.
4.  `BLKINDEX` - Like the MPI block indexed type.
5.  `STRUCT` - Like the MPI, a general case. Note that this is used only
    when at least one of the component types is not a simple, predefined
    type (in that case, INDEXED is sufficient).

There are several modifiers that may be attached to each of these. For
example, they may be FINAL (the base type is not another building
block). They may be in units of 4, 8, or 16 bytes, and aligned
accordingly.

A program is just a sequence of these building blocks.

An important special case is the one in which there is no occurrence of
the STRUCT type. In that case, there is a nice correspondence between
the program and a concise representation of the state of the move in the
partial pack/unpack case (i.e., the case where only some of the data has
been moved and it is necessary to stop, remember the current position in
the move, and be able to efficiently restart).op

To be specific, here are the operators and all of their fields. There
are a number of common fields:

- `Count` - Number of items in type. The meaning of item depends on the
  type, but it is basically the loop count that will be used to
  perform the data move.
- `Size` - Number of bytes moved by dataloop
- `Extent` - Extent of dataloop
- `Alignment` - Natural alignment of data (may be 1, in case it means
  general, on any byte)
- `NaturalSize` - Natural size for the moves; typically a size
  corresponding to a move instruction supported by the processor
  (e.g., 1, 2, 4, 8, 16).


- `CONTIG`
    - `Count`
    - `Size`
    - `Extent`
    - `Alignment`
    - `NaturalSize`

Note that by providing an Extent value, a CONTIG can be used for a
padded structure of bytes, or used to construct a strided move type.

- `VECTOR`
    - `Count`
    - `Size`
    - `Extent`
    - `Alignment`
    - `NaturalSize`
    - `Stride`

Note that by providing an Extent value, rather than computing it in
terms of progress through the dataloop, we can use VECTOR for the case
where the extent is not the distance from the beginning of the first
moved byte to the end of the last byte moved. This is necessary for
nested vectors, for example, when building a dataloop that represents a
face of a 3-d cube.

- `INDEXED`
    - `Count`
    - `Size`
    - `Extent`
    - `Alignment`
    - `NaturalSize`
    - `IOV` (pointer to array of iov elements)

- `BLKINDEXED`
    - `Count`
    - `Size`
    - `Extent`
    - `Alignment`
    - `NaturalSize`
    - `BlockSize`
    - `Displacements` (pointer to array of displacements)

- `STRUCT`
    - `Count`
    - `Size`
    - `Extent`
    - `Alignment`
    - `NaturalSize`
    - `ChildDataloop` array (pointer to a structure containing count,
      displacement, pointer to dataloop)

Note that these are structured so that the first five elements are
always the same.

TODO: handling of `LB, UB, TRUE_LB, TRUE_UB, EXTENT, TRUE_EXTENT`.

