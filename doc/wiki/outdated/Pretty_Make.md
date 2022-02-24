As of [r980](https://trac.mcs.anl.gov/projects/mpi/changeset/980) a
`make` in the MPICH2 source tree will result in a newer output format.
This format is much terser than the old style and is intended to
resemble a more recent Linux kernel build.

Note that at this time MPE does not use the newer terse format. This is
because it does not use `automake`, which is where the [printing
magic](https://www.gnu.org/software/automake/manual/automake.html#index-Option_002c-_0040option_007bsilent_002drules_007d-915)
was implemented. It should be straightforward enough to add this to MPE,
someone just needs to insert the appropriate code into the `Makefile.in`
files.

    <nowiki>
    % make
    Beginning make
    make all-local
    make[1]: Entering directory `/sandbox/mpi/src/mpich2-trunk'
    make[1]: Leaving directory `/sandbox/mpi/src/mpich2-trunk'
    make[1]: Entering directory `/sandbox/mpi/src/mpich2-trunk/src'
    make[2]: Entering directory `/sandbox/mpi/src/mpich2-trunk/src/mpid'
    make[3]: Entering directory `/sandbox/mpi/src/mpich2-trunk/src/mpid/common'
    make[4]: Entering directory `/sandbox/mpi/src/mpich2-trunk/src/mpid/common/locks'
      CC              mpidu_process_locks.c
      CC              mpidu_atomic_primitives.c
      CC              mpidu_queue.c
      AR              ../../../../lib/libmpich.a mpidu_process_locks.o mpidu_atomic_primitives.o mpidu_queue.o
    ranlib ../../../../lib/libmpich.a
    date > .libstamp0
    make[4]: Leaving directory `/sandbox/mpi/src/mpich2-trunk/src/mpid/common/locks'
    make[4]: Entering directory `/sandbox/mpi/src/mpich2-trunk/src/mpid/common/datatype'
    make[5]: Entering directory `/sandbox/mpi/src/mpich2-trunk/src/mpid/common/datatype/dataloop'
      CC              dataloop.c
      CC              segment.c
      CC              segment_ops.c
      CC              dataloop_create.c
      CC              dataloop_create_contig.c
      CC              dataloop_create_vector.c
      CC              dataloop_create_blockindexed.c
      CC              dataloop_create_indexed.c
      CC              dataloop_create_struct.c
      CC              dataloop_create_pairtype.c
      CC              subarray_support.c
      CC              darray_support.c
      AR              ../../../../../lib/libmpich.a dataloop.o segment.o segment_ops.o dataloop_create.o dataloop_create_contig.o dataloop_create_vector.o dataloop_create_blockindexed.o dataloop_create_indexed.o dataloop_create_struct.o dataloop_create_pairtype.o subarray_support.o darray_support.oranlib ../../../../../lib/libmpich.adate > .libstamp0
    make[5]: Leaving directory `/sandbox/mpi/src/mpich2-trunk/src/mpid/common/datatype/dataloop'
    ...
    </nowiki>

The old format is still available by specifying `V=1` as a parameter to
`make` or environment variable:

    <nowiki>
    % make V=1
    Beginning make
    make all-local
    make[1]: Entering directory `/sandbox/mpi/src/mpich2-trunk/src'
    make[2]: Entering directory `/sandbox/mpi/src/mpich2-trunk/src/mpid'
    make[3]: Entering directory `/sandbox/mpi/src/mpich2-trunk/src/mpid/common'
    make[4]: Entering directory `/sandbox/mpi/src/mpich2-trunk/src/mpid/common/locks'
    gcc -DHAVE_CONFIG_H -I. -I. -I. -I../../../include -I/sandbox/mpi/src/mpich2-trunk/src/include -g3 -O0 -g -I/sandbox/mpi/src/mpich2-trunk/src/mpid/ch3/include -I/sandbox/mpi/src/mpich2-trunk/src/mpid/ch3/include -I/sandbox/mpi/src/mpichmpd/ch3/channels/sock/include -I/sandbox/mpi/src/mpich2-trunk/src/mpid/ch3/channels/sock/include -I/sandbox/mpi/src/mpich2-trunk/src/mpid/common/sock -I/sandbox/mpi/src/mpich2-trunk/src/mpid/common/sock -I/sandbox/mpi/src/mpich2-trunk/sr/mpid/common/sock/poll -I/sandbox/mpi/src/mpich2-trunk/src/mpid/common/sock/poll -c mpidu_process_locks.c
    gcc -DHAVE_CONFIG_H -I. -I. -I. -I../../../include -I/sandbox/mpi/src/mpich2-trunk/src/include -g3 -O0 -g -I/sandbox/mpi/src/mpich2-trunk/src/mpid/ch3/include -I/sandbox/mpi/src/mpich2-trunk/src/mpid/ch3/include -I/sandbox/mpi/src/mpichmpd/ch3/channels/sock/include -I/sandbox/mpi/src/mpich2-trunk/src/mpid/ch3/channels/sock/include -I/sandbox/mpi/src/mpich2-trunk/src/mpid/common/sock -I/sandbox/mpi/src/mpich2-trunk/src/mpid/common/sock -I/sandbox/mpi/src/mpich2-trunk/sr/mpid/common/sock/poll -I/sandbox/mpi/src/mpich2-trunk/src/mpid/common/sock/poll -c mpidu_atomic_primitives.c
    gcc -DHAVE_CONFIG_H -I. -I. -I. -I../../../include -I/sandbox/mpi/src/mpich2-trunk/src/include -g3 -O0 -g -I/sandbox/mpi/src/mpich2-trunk/src/mpid/ch3/include -I/sandbox/mpi/src/mpich2-trunk/src/mpid/ch3/include -I/sandbox/mpi/src/mpichmpd/ch3/channels/sock/include -I/sandbox/mpi/src/mpich2-trunk/src/mpid/ch3/channels/sock/include -I/sandbox/mpi/src/mpich2-trunk/src/mpid/common/sock -I/sandbox/mpi/src/mpich2-trunk/src/mpid/common/sock -I/sandbox/mpi/src/mpich2-trunk/sr/mpid/common/sock/poll -I/sandbox/mpi/src/mpich2-trunk/src/mpid/common/sock/poll -c mpidu_queue.car cr ../../../../lib/libmpich.a mpidu_process_locks.o mpidu_atomic_primitives.o mpidu_queue.oranlib ../../../../lib/libmpich.adate > .libstamp0
    make[4]: Leaving directory `/sandbox/mpi/src/mpich2-trunk/src/mpid/common/locks'
    make[4]: Entering directory `/sandbox/mpi/src/mpich2-trunk/src/mpid/common/datatype'
    make[5]: Entering directory `/sandbox/mpi/src/mpich2-trunk/src/mpid/common/datatype/dataloop'
    gcc -DHAVE_CONFIG_H -I. -I. -I./.. -I/sandbox/mpi/src/mpich2-trunk/src/include -I/sandbox/mpi/src/mpich2-trunk/src/include -g3 -O0 -g -I/sandbox/mpi/src/mpich2-trunk/src/mpid/ch3/include -I/sandbox/mpi/src/mpich2-trunk/src/mpid/ch3/inclsandboxrc/mpich2-trunk/src/mpid/ch3/channels/sock/include -I/sandbox/mpi/src/mpich2-trunk/src/mpid/ch3/channels/sock/include -I/sandbox/mpi/src/mpich2-trunk/src/mpid/common/sock -I/sandbox/mpi/src/mpich2-trunk/src/mpid/common/sock -I/sanox/mpi/src/mpich2-trunk/src/mpid/common/sock/poll -I/sandbox/mpi/src/mpich2-trunk/src/mpid/common/sock/poll -c dataloop.c
    ...
    </nowiki>