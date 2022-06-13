# MPI + Argobots

As core number of many-core processors keeps increasing, MPI+X is
becoming a promising programming model for large scale SMP clusters. It
has the potential to utilizing both intra-node and inter-node
parallelism with appropriate execution unit and granularity.

[Argobots](http://www.argobots.org/)
is a low-level threading/task infrastructure developed by a joint effort
of Argonne National Laboratory, University of Illinois at Urbana-Champaign,
University of Tennessee, Knoxville and Pacific Northwest National Laboratory.
It provides a lightweight execution model that combines low-latency thread
and task scheduling with optimized data-movement functionality.

A benefit of Argobots is providing asynchrony/overlap to MPI. The idea
is to make multiple MPI blocking calls at the same time in multiple
ULTs, if one MPI call is blocked in ULT A, MPI runtime will detect it
and context switch to another ULT to make progress on other blocking
calls. Once other ULTs finished their execution, they will switch back
to ULT A to continue its execution. In this way, we can keep the CPU
busy doing useful work instead of waiting the blocking call.

## Build MPI+Argobots

Git repos:

|                             |                                           |
| --------------------------- | ----------------------------------------- |
| Argobot read-only clone URL | <https://github.com/pmodels/argobots.git> |
| MPICH read-only clone URL   | <https://github.com/pmodels/mpich.git>    |

If you find any problems, please create
[a GitHub issue](https://github.com/pmodels/mpich/issues)
or send us a bug report
([Argobots discussion ML](mailto:discuss@argobots.org)
or
[MPICH discussion ML](mailto:discuss@mpich.org)).

### Build Argobots

Follow the instructions in
<https://github.com/pmodels/argobots/wiki/Getting-and-Building> to build
Argobots.

```
export ABT_INSTALL_PATH=/path/to/install-abt
git clone https://github.com/pmodels/argobots.git argobots
cd argobots
./autogen.sh # can be skipped if built from tarball
./configure --prefix=$ABT_INSTALL_PATH
make -j 4
make install
```

### Build Argobots-aware MPICH

MPI+Argobots has been merged into the master branch. Currently
(2020/5/10), Argobots does not work well with MPICH 3.1x-3.3x and 3.4a2,
so please use the master branch.

```
export MPICH_INSTALL_PATH=/path/to/install-mpich
git clone https://github.com/pmodels/mpich.git mpich-abt
cd mpich-abt
./autogen.sh # can be skipped if built from tarball
./configure --prefix=$MPICH_INSTALL_PATH --with-thread-package=argobots --with-argobots=$ABT_INSTALL_PATH --with-device=ch4:YOUR_DEVICE(ofi|ucx)
make -j 4
make install
```

Now MPICH considers Argobots ULTs as underlying threads when
`MPI_THREAD_MULTIPLE` is set, so MPICH uses Argobots threading
operations for scheduling. Because threading operations of Argobots
(without `--enable-feature=no-ext-thread` flag) work with Pthreads, this
Argobots-aware MPICH should work with Pthreads, but there might be a
performance degradation.

## References

1\. Argobots Home, <http://www.argobots.org/>

2\. Huiwei Lu, Sangmin Seo, and Pavan Balaji. MPI+ULT: Overlapping
Communication and Computation with User-Level Threads. The 2015 IEEE
17th International Conference on High Performance Computing and
Communications (HPCC '15), New York, USA, August 24-26, 2015. (Note that
the current MPICH does not support `MPIX_THREAD_ULT`)
