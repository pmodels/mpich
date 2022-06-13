# Summit

This page describes how to build MPICH on the *Summit* machine at Oak
Ridge. Summit is a POWER9-based machine with Infiniband interconnect.
The `UCX` device works best here. Performance is on par with the IBM
SpectrumScale MPI (based on OpenMPI).

## Summit Build Instructions

Released versions of mpich (as of 3.2) do not know how to deal with the
unusually large business cards generated on this platform. You'll need a
release containing `commit 2bb15c6e875386f`. The following configure
instructions have been also tested with MPICH/main branch (`commit
219a9006`), Summit default gcc (gcc/6.4.0), cuda (cuda/10.1.243), and
spectrum-mpi (spectrum-mpi/10.3.1.2-20200121) modules.

### Prerequisite

#### Install CUDA-aware UCX

MPICH by default uses the system default ucx which is not CUDA-aware. It
is recommended to install your own UCX library. 
[UCX-1.10.0 Release 1](https://github.com/openucx/ucx/releases/tag/v1.10.0)
has been tested with MPICH/main branch (`commit 219a9006`).

```
module load gcc cuda
./configure CC=gcc  CXX=g++ --build=powerpc64le-redhat-linux-gnu --host=powerpc64le-redhat-linux-gnu --with-cuda=$CUDA_DIR \
--disable-logging --disable-debug --disable-assertions

# $CUDA_DIR is set by the cuda module
```
#### A Note About Darshan

The default darshan module loaded on summit is compiled against
OpenMPI-based spectrum-mpi. Unload the `darshan-runtime` module before
running your own MPICH executables. If you are going to use Darshan to
collect I/O statistics, building your own is straightforward once you
have built MPICH.

### Build MPICH with jsrun and disable CUDA support

#### Configuration

```
module load gcc spectrum-mpi
./configure --with-device=ch4:ucx ---with-ucx=$ucxdir -with-pm=none --with-pmix=$MPI_ROOT --with-hwloc=embedded CFLAGS=-std=gnu11

# $MPI_ROOT is set by the spectrum-mpi module; Summit-customized pmix is available here.
```

#### Execution

```
module unload darshan-runtime
# Launch two ranks each on a separate node
jsrun -n 2 -r 1 ./cpi
```


### Build MPICH With `jsrun` and Enable CUDA support

#### Configuration

```
module load gcc spectrum-mpi cuda
./configure --with-device=ch4:ucx --with-ucx=$ucxdir --with-pm=none --with-pmix=$MPI_ROOT --with-cuda=$CUDA_DIR --with-hwloc=embedded CFLAGS=-std=gnu11

# $CUDA_DIR is set by the cuda module. $MPI_ROOT is set by the spectrum-mpi module; Summit-customized pmix is available here.
```

#### Execution

```
module unload darshan-runtime
# Launch two ranks each on a separate node and a separate GPU
jsrun -n 2 -r 1 -g 1 --smpiargs="-disable_gpu_hooks" ./test/mpi/pt2pt/pingping \
    -type=MPI_INT -sendcnt=512 -recvcnt=1024 -seed=78 -testsize=4  -sendmem=device -recvmem=device
```

For more *jsrun* options, please check 
[Summit User Guide - Job Launcher](https://docs.olcf.ornl.gov/systems/summit_user_guide.html#job-launcher-jsrun)

### Build MPICH With Hydra And Enable CUDA Support

#### Configuration

```
module load gcc cuda
./configure --with-device=ch4:ucx --with-ucx=$ucxdir --with-cuda=$CUDA_DIR --with-hwloc=embedded CFLAGS=-std=gnu11

# $CUDA_DIR is set by the cuda module.
```

#### Execution

```
module unload darshan-runtime

# Adjust -n for different number of nodes. Example gets hostname of two nodes.
jsrun -n 2 -r 1 hostname > ~/hostfile

export LD_LIBRARY_PATH=$(jsrun -n 1 -r 1 echo $LD_LIBRARY_PATH)
mpiexec -np 2 -ppn 2 --launcher ssh -f <hostfile> -gpus-per-proc=1 ./test/mpi/pt2pt/pingping \
    -type=MPI_INT -sendcnt=512 -recvcnt=1024 -seed=78 -testsize=4  -sendmem=device -recvmem=device
```

Also see Common issues 5 (see below).

### Common issues

1. Watch out for system ucx vs mpich built-in ucx. I got some undefined
symbols in ucx routines because MPICH was configured to use its own
mpich but was picking up system UCX (thanks to spack setting
LD_LIBRARY_PATH)

2. "Invalid communicator" error at PMPI_Comm_size caused by
*darshan-runtime* (see error below). Please run "module unload
darshan-runtime" before execution

```
Abort(671694341) on node 1 (rank 1 in comm 0): Fatal error in PMPI_Comm_size: Invalid communicator, error stack:
PMPI_Comm_size(100): MPI_Comm_size(comm=0xca12a0, size=0x2000000b049c) failed
PMPI_Comm_size(67).: Invalid communicator
```

3. CUDA hook error reported when launching with `jsrun` and use GPU
buffers in MPI communication call (see error below). Please add
`--smpiargs="-disable_gpu_hooks` for `jsrun`.

```
CUDA Hook Library: Failed to find symbol mem_find_dreg_entries, /autofs/nccs-svm1_home1/minsi/git/mpich.git.main/build-ucx-g-cuda10.1.243/test/mpi/pt2pt/pingping: undefined symbol: __PAMI_Invalidate_region
```

4. `Abort at src/util/mpir_pmi.c line 1105` when launching with
`jsrun`. It is an MPICH/PMIx bug. Please watch out
[4](https://github.com/pmodels/mpich/issues/4815) for temporary
workaround and final fix.

5\. `hydra_pmi_proxy: error while loading shared libraries:
libcudart.so.10.1: cannot open shared object file: No such file or
directory` when launching with `mpiexec`. It is because CUDA library
path is not set on each compute node by default, but set by `jsrun`.
Workaround is to manually set CUDA library path in `~/.bashrc`. Note
that passing `LD_LIBRARY_PATH` to `mpiexec` cannot solve this issue,
because `hydra_pmi_proxy` is launched before transferring environment
variables.

```
# write into ~/.bashrc, here is example for cuda/10.1.243
export LD_LIBRARY_PATH=/sw/summit/cuda/10.1.243/lib64:$LD_LIBRARY_PATH
```

6. Cannot find any library linked to executable when launching with
`mpiexec`. Manually transfer `LD_LIBRARY_PATH` to compute node by
setting `LD_LIBRARY_PATH=$(jsrun -n 1 -r 1 echo $LD_LIBRARY_PATH`
before `mpiexec`.

7`. Yaksa error `Caught signal 11 (Segmentation fault: invalid
permissions for mapped object at address 0x2000fbe00008)` when launching
with `mpiexec` and using GPU buffer in MPI communication. You may also
say the warning from Yaksa (see below). The reason is that Summit by
default sets `EXCLUSIVE_PROCESS` compute node (see
[5](https://docs.olcf.ornl.gov/systems/summit_user_guide.html#gpu-compute-modes))
but does not set "CUDA_VISIBLE_DEVICES". The solution is to use
`mpiexec` GPU binding option `-gpus-per-proc` (e.g., set
`-gpus-per-proc=1` will bind one GPU for each rank)

```
[yaksa] ====> Disabling CUDA support <====
[yaksa] CUDA is setup in exclusive compute mode, but CUDA_VISIBLE_DEVICES is not set
[yaksa] You can silence this warning by setting CUDA_VISIBLE_DEVICES
```

8. UCX error "Invalid parameter" reported at MPI init. This error is
reported when using MPICH/main with UCX 1.9.0. Confirmed UCX 1.10.0 or
an older UCX version does not cause this issue.

```
Abort(271159951) on node 0 (rank 0 in comm 0): Fatal error in PMPI_Init_thread: Other MPI error, error stack:
PMPI_Init_thread(103).......: MPI_Init_thread(argc=0x7ffff9c22640, argv=0x7ffff9c22648, required=0, provided=0x7ffff9c2243c) failed
MPII_Init_thread(196).......:
MPID_Init(475)..............:
MPID_Init_world(626)........:
MPIDI_UCX_mpi_init_hook(275):
init_worker(71).............:  ucx function returned with failed status(ucx_init.c 71 init_worker Invalid parameter)
```
