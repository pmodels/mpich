The Nemesis messaging subsystem (the default CH3 channel as of
mpich2-1.1) provides very high performance shared memory communication
on many platforms. However, accurately measuring this performance isn't
always easy. This page provides a number of techniques and suggestions
for obtaining accurate, high performance benchmark results on your
platform.

__TOC__

## General Caveats and Suggestions

Good benchmarking is difficult. Good benchmark results are repeatable by
yourself and others and provide expected or at least explainable
information. Many general factors can work against obtaining good
results:

  - shared computers or networks
  - background tasks that might be running on a computer being
    benchmarked
  - implicit configuration in your environment that makes it harder for
    others to reproduce your results
  - not fully understanding what you are measuring

At this time with nemesis, most of our performance work has gone into
Intel and AMD processor architectures such as IA32, IA64, and
EMT64/AMD64. Performance might vary substantially on other platforms due
to a lack of tuning. It is also worth noting that you should never
oversubscribe processor cores when using Nemesis because of the way that
Nemesis polls for shared memory. Running more Nemesis-based MPI
processes on a node than there are cores on that node will almost always
result in terrible performance.

When building MPICH2 for benchmarking, **we recommend passing the
following flag to configure:**
**`--enable-fast=O3,nochkmsg,notiming,ndebug`**. This should help to
ensure that the compiler aggressively optimizes the code and that
developer-friendly and user-friendly debugging features are not causing
a performance degradation.

## Measuring One Way Ping-Pong Latency

We typically measure one way ping-pong latency using an older, stable
version of netpipe that is included in the MPICH2 distribution. This
provides a consistent baseline for measurements over time using a
measurement methodology that is well understood and very reasonable.
This program can be found in the MPICH2 distribution in
`test/mpi/basic/netmpi.c`. You should be able to build it on most
platforms simply by changing to that directory and typing `make
netpipe`. The simplest way to run it is:

```
 (assuming your process manager, such as mpd, is already setup)
 mpiexec -n 2 ./netpipe
 ...
```

This should give you reasonable performance results in many situations.
However, there are two issues to address if we want a better benchmark.

  - perturbation at small message sizes
  - the impact of process affinity and memory hierarchy on performance

Because Nemesis has such low overhead, the performance results for very
small messages (such as 0-byte and 1-byte messages) can be greatly
affected by other processes on the same computer. Please ensure that you
are the only user of the machine and that no other CPU or disk intensive
processes are running when you are measuring your results.

[mpptest](http://www.mcs.anl.gov/mpi/mpptest) provides more reliable and
reproducible results, and includes options to measure cache effects, but
is a more complex program. It does not contain code to address process
affinity issues; when using `mpptest`, you will need to ensure correct
process affinity either with support from the MPI implementation or by
adding the appropriate, system dependent commands in the `mpptest`
source code.

### The Impact of the Memory Hierarchy

The exact memory hierarchy configuration of your multi-SMP or multi-core
node and any process affinity specified when running your benchmark
makes a very big impact on both message latency and bandwidth. For
example, on a dual quad-core Intel Xeon X5355 machine that we have at
ANL, there are at least 4 different classes of performance depending on
the way that process affinity is specified.

This table shows the values for various fields in `/proc/cpuinfo` under
Linux:

| processor | physical id | core id | (die id) |
| --------- | ----------- | ------- | -------- |
| 0         | 0           | 0       | 0        |
| 1         | 1           | 0       | 0        |
| 2         | 0           | 1       | 0        |
| 3         | 1           | 1       | 0        |
| 4         | 0           | 2       | 1        |
| 5         | 1           | 2       | 1        |
| 6         | 0           | 3       | 1        |
| 7         | 1           | 3       | 1        |

The following processor number pairs share L2 caches:

  - 0 and 2
  - 4 and 6
  - 1 and 3
  - 5 and 7

*(An externally hosted diagram of this layout [can be found
here](http://static.msi.umn.edu/calhoun/clovertown.png).)*

So, this leads to three main cases:

1.  same die, shared L2 cache (e.g. 4 and 6)
2.  same die, no shared L2 cache (e.g. 2 and 6)
3.  different dies, no shared L2 cache (e.g. 4 and 5)

Furthermore, core ID 0 typically handles interrupts for each physical
processor package, so benchmarks run on processor IDs 0 and 1 usually
greater variation from run to run and will sometimes have a slower
overall average. This leads to three more cases, which are basically the
same three cases from before, but where one of the cores is 0 or 1 and
is therefore servicing interrupts.

### Processes Binding

A typical MPI application will use all of the cores on a node, one per
core. So it is a very fair and reasonable benchmark to compare the
performance between processes on various cores, even if they are not
necessarily the pairs that will yield the best performance. However, it
is important that you intentionally bind the processes to particular
cores when taking your measurements for maximal repeatability. It is
also important that you explain which pair you selected and why if you
are publishing your results or otherwise comparing them to other
performance results.

In order to bind processes to cores on Linux, you can use the taskset
command with the mpd process manager like so:

    % mpiexec -n 1 taskset -c 4 ./netpipe : -n 1 taskset -c 6 ./netpipe
    0: intel-loaner2
    1: intel-loaner2
    Latency: 0.000000219
    Sync Time: 0.000000725
    Now starting main loop
      0:         0 bytes 4194304 times -->    0.00 Mbps in 0.000000218 sec
      1:         1 bytes 4194304 times -->   34.54 Mbps in 0.000000221 sec
    ...

With the hydra process manager (and MPICH2 configured with
`--enable-hydra-procbind`) you should be able to specify the binding
more succinctly like this:

    % mpiexec.hydra -binding user:4,6 -n 2 ./netpipe
    ...

### Example Benchmark Results

The following measurements were taken on machine at Argonne with dual
quad-core Intel Xeon X5355 2.66GHz processors with 8MB (4MB per
core-pair) of L2 cache and a 1333 MHz front-side bus. MPICH2-1.1 was
used and it was built with gcc-4.2.4 and the following configure
arguments: `--enable-hydra-procbind
--enable-fast=O3,nochkmsg,notiming,ndebug`.

| Core Pair | 0-byte Message Latency | Notes                                                                     |
| --------- | ---------------------- | ------------------------------------------------------------------------- |
| 4,6       | 209 ns                 | shared L2 cache, best performance                                         |
| 0,2       | 216 ns                 | shared L2 cache, but core 0 handles interrupts                            |
| 4,2       | 479 ns                 | same package, different die                                               |
| 4,5       | 641 ns                 | different physical packages                                               |
| 0,1       | 642 ns                 | different physical packages (comm over FSB), both cores handle interrupts |