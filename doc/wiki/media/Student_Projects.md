# Student Projects

This is a brief and incomplete list of possible student projects
involving MPICH.

# Testing Infrastructure for Datatypes

The MTest testing infrastructure is limited in a number of ways for
testing datatypes. It does not allow cross testing across different
"compatible" datatypes (i.e., datatypes with the same signature), for
cases more than two multiple compatible datatypes are required (e.g.,
MPI_GET_ACCUMULATE), for cases where data checking is not a simple
"equal" relationship (e.g., MPI_ACCUMULATE). The idea of this project
is to revamp the MTEST datatype infrastructure in the following ways:

1\. Allow for data object pools.

A data object is made up of three components: one or more **core basic
datatypes**, a **count**, and a **buffer**. The object then assembles
**count** number of each of the **core basic datatypes** into a derived
datatype. The **buffer** allocated is large enough to accommodate such
derived datatype.

An object pool consists of a collection of objects with the **same set
of core basic datatypes** and the **same count**, but assembled into
different layouts (e.g., contiguous, vector, vector-of-vectors,
indexed).

A data object pool would have the following properties:

  - All objects in a pool would have the **same signature**. Thus, one
    can send using one element of the pool and receive using another
    element of the pool.
  - If two pools are created with counts of **c1** and **c2** (**c2** \>
    **c1**) with the same datatype, then all objects of the second pool
    (with counts of **c2**) can accommodate any communication using
    objects of the first pool (with counts of **c1**). In other words,
    one can send using an object of the first pool and receive using an
    object of the second pool, but not the other way around without
    truncation.

2\. Functions should be provided to allocate a bunch of pools, e.g.,
pools for each datatype. For example:

``` c
    for (i = 0; i < MTEST_DTYPE_MAX_DATATYPES; i++) {
        for (j = 0; j < MAX_COUNT; j++) {
            pool = MTEST_DTYPE_ALLOCATE_POOL(i /*core datatype*/, j /*count*/);
            pool_size = MTEST_DTYPE_GET_POOL_SIZE(pool);

            for (k = 0; k < pool_size; k++) {
                send_obj = MTEST_DTYPE_GET_POOL_OBJ(pool, k);
                MTEST_DTYPE_ALLOC_OBJ(send_obj);

                for (p = 0; p < pool_size; p++) {
                    recv_obj = MTEST_DTYPE_GET_POOL_OBJ(pool, p);
                    MTEST_DTYPE_ALLOC_OBJ(recv_obj);
                    MPI_Sendrecv(..., send_obj->dtype, ..., recv_obj->dtype, ...);
                }
            }
        }
    }
```

The implementation should try to lazily allocate the object only when
requested, instead of simply allocating all objects in the pool as soon
as the pool is created.

3\. Introspection routines. These allow one to introspect a pool object
and return the "core" datatype used inside a pool. In the case of pools
based on basic datatypes, this would return a single datatype. For pools
made up of structures, this would return all the datatypes that form the
structure.

``` c
num_dtypes = MTEST_DTYPE_DECODE_OBJ_TYPE_COUNT(pool);
for (i = 0; i < num_dtypes; i++) {
    dtype[i] = MTEST_DTYPE_DECODE_OBJ_TYPE(pool, i);
}
```

If **num_dtypes** \> 1, this is a structure consisting of
**num_dtypes** datatypes. In cases where the structure is comprised of
other structures, the **num_dtypes** value is the number of basic
datatypes used in all the structures.

The following routines should be provided to allow applications to skip
some datatypes:

  - **MTEST_DTYPE_IS_BASIC**: True if the datatype is a basic
    datatype
  - **MTEST_DTYPE_IS_PREDEFINED**: True if the datatype is a
    predefined datatype
  - **MTEST_DTYPE_OBJ_EXTENT**: Gives the extent of the datatype
    associated with the object (can be used to skip datatypes which are
    too large)

4\. Initialization routines that allow initializing a buffer using an
initial value and stride.

`MTEST_DTYPE_INIT_OBJ(obj, type_index, start_val, stride);`

**type_index** is the index of the core type used in the pool. For
pools made up of basic datatypes, the object has a single core type
(decoded using the introspection routines above). In such cases, the
only valid value for **type_index** would be 0. For pools made up of
structures containing N different datatypes, **type_index** can take
values of 0 to N-1.

An object in a pool can be initialized to all zeros using:

`MTEST_DTYPE_INIT_OBJ(obj, 0, 0, 0);`

Similarly an object can be initialized with odd integers using:

`MTEST_DTYPE_INIT_OBJ(obj, 0, 1, 2);`

For objects that are based on floating point datatypes, they can be
initialized using:

`MTEST_DTYPE_INIT_OBJ(obj, 0, 1.0, 2.0);`

If an object is made up of a structure, it can be initialized using:

``` c
num_dtypes = MTEST_DTYPE_DECODE_OBJ_TYPE_COUNT(pool);
for (i = 0; i < num_dtypes; i++) {
    dtype = MTEST_DTYPE_DECODE_OBJ_TYPE(pool, i);
    if (dtype == MPI_INT || dtype == MPI_UINT32_T ...)
        MTEST_DTYPE_INIT_OBJ(obj, i, 0, 1);
    else if (dtype == MPI_DOUBLE || dtype == MPI_FLOAT ...)
        MTEST_DTYPE_INIT_OBJ(obj, i, 1.0, 1.5);
    ...
}
```

5\. Check routines that check if a buffer has specific values. There are
two forms of check routines. One form is a mirror copy of the
initialization routines.

`MTEST_DTYPE_CHECK_OBJ(obj, type_index, start_val, stride);`

This routine simply checks if the object is as if it was initialized
with **start_val** and **stride**.

Another type is for checking specific elements:

`MTEST_DTYPE_CHECK_OBJ_ELEMENT(obj, type_index, offset, val);`

This routine checks if the **offset** element in the object has the
value **val**. This method can be used if the values cannot be
represented in a simple **start_val**/**stride** format, e.g., when you
do an ACCUMULATE with MPI_PROD.

# Unbalanced Tree Search with MPI-3 RMA

Unbalanced Tree Search (UTS) has been implemented with one-sided
communication models such as UPC. However, currently no good
implementations exist for UTS using MPI-3 one-sided communication or
remote memory access (RMA). This project involves multiple parts.

1\. Implementation of UTS using MPI two-sided communication. This model
works with MPI-1, but relies on the target process making MPI receive
calls to process incoming requests. While, at first thought, this might
sound inefficient, this approach might not be too bad. In fact, when the
steal required at the target process is complex, multiple round-trip
operations might be required to successfully steal a task, while a
single round-trip operation would be required with MPI two-sided
communication. However, this model can be expensive if the tasks have
some computation associated with them thus introducing some noise in the
response operations.

2\. Implementation of UTS using MPI RMA. The simplest implementation of
UTS with MPI RMA would be to use a lock/unlock model (exclusive mode)
where a process would lock the target process and then search for
available tasks before it can steal. This model is simple, but would
require multiple round-trip operations making it inefficient compared to
MPI two-sided communication.

3\. Improved implementation of UTS using MPI RMA. Let's assume that all
tasks are available in MPI remote accessible memory in an array. Two
pointers are provided: **current_task** and **last_task**. A process
would do a FETCH_AND_ADD on **current_task** and an atomic GET on
**last_task**. If **current_task** is more than **last_task**, there
are no tasks to be stolen. Otherwise, we can fetch the actual tasks
using a second RMA operation. This requires two round-trip operations,
but can be faster than two-sided communication if the target process is
busy with some computation. One gotcha in this approach is with
enqueueing new tasks. In this model, enqueueing can be done using the
following steps:

  - If **current_task** \< **last_task**
      - Increment **current_task** to a large number
      - Add new tasks at **last_task**
      - Do COMPARE_AND_SWAPs on **current_task** till we can bring it
        back to its original position
  - If **current_task** \>= **last_task**
      - Increment **current_task** to a large number
      - Add new tasks at **last_task**
      - Do COMPARE_AND_SWAPs on **current_task** till we can bring it
        to the original value of **last_task**

This approach makes enqueueing new tasks expensive since this could
potentially lead to unlimited compare-and-swaps. But since the enqueue
is local, we expect that the local compare-and-swaps would be faster
than the remote compare-and-swaps making the number of attempts we need
to do small.

4\. The above approach assumes a single queue for tasks, thus making the
memory requirement large when there can be a lot of tasks that need to
be enqueued. The final approach could be to use multiple queues, that
are allocated on-demand, that can help reduce this memory requirement.

# False sharing in MPI_THREAD_MULTIPLE

Several data structures might be subject to false sharing issues between
threads as cache line alignment is often not performed. This project's
goal is to investigate the presence and extent of false sharing in MPICH
and the underlying messaging layers (e.g., OFI). If false sharing is a
significant issue, the next step would be to study the performance vs.
memory consumption trade-off of properly aligning the relevant data
structures to cache line boundaries.

# Optimizing Memory Management in Process-based Asynchronous Progress Library

[Casper](http://www.mcs.anl.gov/project/casper) is a process-based
asynchronous progress model for MPI RMA communication on multicore and
many-core architectures. It is implemented as an external library of MPI
through the PMPI interface, thus being able to link with various MPI
implementations on different platforms. There are two issues existing in
current implementation of Casper.

  - The current Casper implementation only works with MPI_Win_allocate
    without **alloc_shared_noncontig=true**, because the ghost process
    locally calculates the window-offset of each user process. This
    should be fixed.
  - For symmetric window allocation (i.e., every process allocates the
    same size of window and gets the same address of allocated memory on
    all processes ), the internal structures of Casper can be more
    scalable. For example, we do not need store the address and offset
    of all processes on every process, thus eliminating O(P) data
    structure.
