# MPICH

MPICH is a high-performance and widely portable implementation of the Message Passing Interface (MPI) standard from the Argonne National Laboratory. This implementation provides all MPI functions and features required by the standard with comprehensive support for parallel computing applications.

MPICH is widely adopted by multiple vendor partners including Intel, Hewlett Packard Enterprise (HPE), ParTec, Ohio State University (X-ScaleSolutions), and has been deployed on many Top 500 HPC systems.

This README file should contain enough information to get you started with MPICH. More extensive installation and user guides can be found in the doc/installguide/install.pdf and doc/userguide/user.pdf files respectively in the release tarball, or the doc/wiki in the source code.

[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/10611/badge)](https://www.bestpractices.dev/projects/10611)

## Table of Contents

1. [Getting Started](#1-getting-started)
2. [CH4 Netmods](#2-ch4-netmods)
3. [GPU Support](#3-gpu-support)
4. [Threading Supports](#4-threading-supports)
5. [Testing the MPICH Installation](#5-testing-the-mpich-installation)
6. [Reporting Installation or Usage Problems](#6-reporting-installation-or-usage-problems)
7. [Developer Builds](#7-developer-builds)
8. [Compiler Flags](#8-compiler-flags)
9. [Multiple Fortran Computer Support](#9-multiple-fortran-computer-support)
10. [ABI Compatibility](#10-abi-compatibility)
11. [Citing MPICH](#11-citing-mpich)

---

## 1. Getting Started

**Note:** this guide assumes you are building MPICH from a release tarball. If you are starting from a git checkout, you will need a few additional steps. Please refer to the [wiki page](https://github.com/pmodels/mpich/blob/main/doc/wiki/Index.md).

The following instructions take you through a sequence of steps to get the default configuration (CH4 device with OFI network module, Hydra process management) of MPICH up and running.

### Prerequisites

You will need the following prerequisites:

- **REQUIRED**: The MPICH source tarball
- **REQUIRED**: Perl
- **REQUIRED**: A C compiler (C99 support is required. See [Shifting Toward C99](https://github.com/pmodels/mpich/blob/main/doc/wiki/source_code/Shifting_Toward_C99.md), C11 is preferred for C11 Atomics)

- **OPTIONAL**: A C++ compiler, if C++ applications are to be used (g++, etc.). If you do not require support for C++ applications, you can disable this support using the configure option `--disable-cxx` (configuring MPICH is described in step 1(d) below).
- **OPTIONAL**: A Fortran compiler, if Fortran applications are to be used (gfortran, ifort, etc.). If you do not require support for Fortran applications, you can disable this support using `--disable-fortran` (configuring MPICH is described in step 1(d) below).
- **OPTIONAL**: Python 3. Python 3 is needed to generate Fortran bindings.

**Note:** For users of csh and tcsh shells, please refer to [Using csh and tcsh Shells](doc/wiki/how_to/Using_csh_tcsh_shells.md) for shell-specific examples.

### Installation Steps

#### (a) Unpack the tar file and go to the top level directory:

```bash
tar xzf mpich-<version>.tar.gz
cd mpich-<version>
```

#### (b) Choose an installation directory

Choose an installation directory, say `/home/<USERNAME>/mpich-install`, which is assumed to non-existent or empty. It will be most convenient if this directory is shared by all of the machines where you intend to run processes. If not, you will have to duplicate it on the other machines after installation.

#### (c) Configure MPICH

Configure MPICH specifying the installation directory and device:

```bash
./configure --prefix=/home/<USERNAME>/mpich-install 2>&1 | tee c.txt
```

The configure will try to determine the best device (the internal network modules) based on system environment. You may also supply a device configuration. E.g.

```bash
./configure --prefix=... --with-device=ch4:ofi |...
```

or:

```bash
./configure --prefix=... --with-device=ch4:ucx |...
```

Refer to section below -- [CH4 Netmods](#2-ch4-netmods) -- for more details.

If a failure occurs, the configure command will display the error. Most errors are straight-forward to follow. For example, if the configure command fails with:

> "No Fortran compiler found. If you don't need to build any
> Fortran programs, you can disable Fortran support using
> --disable-fortran. If you do want to build Fortran programs,
> you need to install a Fortran compiler such as gfortran or
> ifort before you can proceed."

... it means that you don't have a Fortran compiler :-). You will need to either install one, or disable Fortran support in MPICH (with `--disable-fortran` option).

If you are unable to understand what went wrong, please go to Section (6) below, for reporting the issue to the MPICH developers and other users.

#### (d) Build MPICH:

```bash
make 2>&1 | tee m.txt
```

This step should succeed if there were no problems with the preceding step. Check file m.txt. If there were problems, do a `make clean` and then run make again with V=1 to get the verbose output.

```bash
make V=1 2>&1 | tee m.txt
```

#### (e) Install the MPICH commands:

```bash
make install 2>&1 | tee mi.txt
```

This step collects all required executables and scripts in the bin subdirectory of the directory specified by the prefix argument to configure.

#### (f) Add the bin subdirectory to your path

Add the bin subdirectory of the installation directory to your path in your startup script (.bashrc for bash, etc.):

```bash
PATH=/home/<USERNAME>/mpich-install/bin:$PATH ; export PATH
```

Check that everything is in order at this point by doing:

```bash
which mpicc
which mpiexec
```

These commands should display the path to your bin subdirectory of your install directory.

**IMPORTANT NOTE**: The install directory has to be visible at exactly the same path on all machines you want to run your applications on. This is typically achieved by installing MPICH on a shared NFS file-system. If you do not have a shared NFS directory, you will need to manually copy the install directory to all machines at exactly the same location.

#### (g) Run an MPI job

MPICH uses a process manager for starting MPI applications. The process manager provides the "mpiexec" executable, together with other utility executables. MPICH comes packaged with multiple process managers; the default is called Hydra.

Now we will run an MPI job, using the mpiexec command as specified in the MPI standard. There are some examples in the install directory, which you have already put in your path, as well as in the examples directory of your MPICH installation. One of them is the classic CPI example, which computes the value of pi by numerical integration in parallel.

To run the CPI example with 'n' processes on your local machine, you can use:

```bash
mpiexec -n <number> ./examples/cpi
```

Test that you can run an 'n' process CPI job on multiple nodes:

```bash
mpiexec -f machinefile -n <number> ./examples/cpi
```

The 'machinefile' is of the form:

```
host1
host2:2
host3:4   # Random comments
host4:1
```

'host1', 'host2', 'host3' and 'host4' are the hostnames of the machines you want to run the job on. The ':2', ':4', ':1' segments depict the number of processes you want to run on each node. If nothing is specified, ':1' is assumed.

More details on interacting with Hydra can be found at [Using the Hydra Process Manager](https://github.com/pmodels/mpich/blob/main/doc/wiki/how_to/Using_the_Hydra_Process_Manager.md)

If you have completed all of the above steps, you have successfully installed MPICH and run an MPI example.

---

## 2. CH4 Netmods

MPICH uses the CH4 device by default, which supports different network modules (netmods) for communication. This section covers the available CH4 netmods and their configuration options.

**Note:** For information about alternate devices and channels (including legacy CH3), please refer to [Alternate Channels and Devices](doc/wiki/how_to/Alternate_Channels_and_Devices.md).

### OFI Netmod

The ofi netmod provides support for the OFI network programming interface. To enable, configure with the following option:

```bash
--with-device=ch4:ofi[:provider]
```

If the OFI include files and libraries are not in the normal search paths, you can specify them with the following options:

```bash
--with-libfabric-include= and --with-libfabric-lib=
```

... or the if lib/ and include/ are in the same directory, you can use the following option:

```bash
--with-libfabric=
```

If specifying the provider, the MPICH library will be optimized specifically for the requested provider by removing runtime branches to determine provider capabilities. Note that using this feature with a version of the libfabric library older than that recommended with this version of MPICH is unsupported and may result in unexpected behavior. This is also true when using the environment variable FI_PROVIDER.

Please refer to the MPICH documentation for the currently recommended version of libfabric.

### UCX Netmod

The ucx netmod provides support for the Unified Communication X library. It can be built with the following configure option:

```bash
--with-device=ch4:ucx
```

If the UCX include files and libraries are not in the normal search paths, you can specify them with the following options:

```bash
--with-ucx-include= and --with-ucx-lib=
```

... or the if lib/ and include/ are in the same directory, you can use the following option:

```bash
--with-ucx=
```

By default, the UCX library throws warnings when the system does not enable certain features that might hurt performance.  These are important warnings that might cause performance degradation on your system.  But you might need root privileges to fix some of them.  If you would like to disable such warnings, you can set the UCX log level to "error" instead of the default "warn" by using:

```bash
UCX_LOG_LEVEL=error
export UCX_LOG_LEVEL
```

### Capability Sets for CH4 OFI Netmod

Capability sets simplify CH4 OFI netmod configuration by providing optimized presets for specific OFI providers.

#### Compile-time Configuration (Recommended for Performance)
For best performance, specify the OFI provider during configuration:

```bash
--with-device=ch4:ofi:sockets
```

This optimizes MPICH for the sockets provider and sets compile-time constants that cannot be changed at runtime.

#### Runtime Configuration (More Flexible)
For flexibility, configure without specifying the provider:

```bash
--with-device=ch4:ofi
```

Then set the provider and options at runtime:

```bash
export FI_PROVIDER=sockets
export MPIR_CVAR_CH4_OFI_ENABLE_ATOMICS=0  # Example: disable atomics
```

#### Configuration Variables
Common capability set variables:

- `MPIR_CVAR_CH4_OFI_ENABLE_ATOMICS` - Enable/disable provider atomics
- `MPIR_CVAR_CH4_OFI_ENABLE_TAGGED` - Enable/disable tagged messaging
- `MPIR_CVAR_CH4_OFI_ENABLE_RMA` - Enable/disable RMA operations

See `README.envvar` for the complete list of configuration variables.

---

## 3. GPU Support

MPICH automatically detects and enables GPU support for CUDA, Intel Level Zero (ZE), and HIP runtimes during configuration.

### Automatic Detection
GPU support is automatically enabled if any supported GPU runtime is detected during the configure process.

### Manual Configuration
To specify custom GPU runtime locations:

```bash
--with-cuda=<path>     # For NVIDIA CUDA
--with-ze=<path>       # For Intel Level Zero
--with-hip=<path>      # For AMD HIP
```

For separate lib/ and include/ directories:

```bash
--with-cuda-include=<path> --with-cuda-lib=<path>
```

### Disabling GPU Support
To explicitly disable GPU support:

```bash
--without-cuda  # Disable CUDA
--without-ze    # Disable Intel Level Zero
--without-hip   # Disable AMD HIP
```

### Runtime Control
GPU support can be disabled at runtime to avoid initialization overhead for non-GPU applications:

```bash
export MPIR_CVAR_ENABLE_GPU=0
```

---

## 4. Threading Supports

MPICH default enables the support for threading with all four MPI threading levels (SINGLE,
FUNNELED, SERIALIZED, MULTIPLE).

The supported thread level are configured by option:

```bash
--enable-threads={single,funneled,serialized,multiple}
```

The default depends on the configured device. With ch4, "multiple" is the default. Set thread level to "single" provides best performance when application does not use multiple threads. Use "multiple" to allow application to access MPI from multiple threads concurrently.

With "multiple" thread level, there are a few choices for the internal critical section models. This is controlled by configure option:

```bash
--enable-thread-cs={default,global,per-vci}
```

For CH4 device, the default is "per-vci" cs which internally uses multiple VCI (virtual communication interface) critical sections, thus can provide much better performance. To achieve the best performance, applications should try to expose as much parallel information to MPI as possible. For example, if each threads use separate communicators, MPICH may be able to assign separate VCI for each thread, thus achieving the maximum performance.

The multiple VCI support may increase the resource allocation and overheads during initialization. By default, only a single vci is used. Set

```bash
MPIR_CVAR_CH4_NUM_VCIS=<N>
```

to enable multiple vcis at runtime. For best performance, match number of VCIs to the number threads application is using.

MPICH supports multiple threading packages. The default is posix threads (pthreads), but solaris threads, windows threads, argobots and qthreads are also supported.

To configure mpich to work with argobots or qthreads, use the following configure options:

```bash
--with-thread-package=argobots \
    CFLAGS="-I<path_to_argobots/include>" \
    LDFLAGS="-L<path_to_argobots/lib>"

--with-thread-package=qthreads \
    CFLAGS="-I<path_to_qthreads/include>" \
    LDFLAGS="-L<path_to_qthreads/lib>"
```

---

## 5. Testing the MPICH Installation

To test MPICH, we package the MPICH test suite in the MPICH distribution. You can run the test suite after `make install` using:

```bash
make testing
```

The results summary will be placed in test/summary.xml.

The test suite can be used independently to test any installed MPI implementations:

```bash
cd test/mpi
./configure --with-mpi=/path/to/mpi
make testing
```

---

## 6. Reporting Installation or Usage Problems

**[VERY IMPORTANT: PLEASE COMPRESS ALL FILES BEFORE SENDING THEM TO US. DO NOT SPAM THE MAILING LIST WITH LARGE ATTACHMENTS.]**

The distribution has been tested by us on a variety of machines in our environments as well as our partner institutes. If you have problems with the installation or usage of MPICH, please follow these steps:

1. First see the [Frequently Asked Questions (FAQ) page](https://github.com/pmodels/mpich/blob/main/doc/wiki/faq/Frequently_Asked_Questions.md) to see if the problem you are facing has a simple solution. Many common problems and their solutions are listed here.

2. If you cannot find an answer on the FAQ page, look through previous email threads on the [discuss@mpich.org mailing list archive](https://lists.mpich.org/mailman/listinfo/discuss). It is likely someone else had a similar problem, which has already been resolved before.

3. If neither of the above steps work, please send an email to discuss@mpich.org. You need to [subscribe to this list](https://lists.mpich.org/mailman/listinfo/discuss) before sending an email.

Your email should contain the following files. **ONCE AGAIN, PLEASE COMPRESS BEFORE SENDING, AS THE FILES CAN BE LARGE.** Note that, depending on which step the build failed, some of the files might not exist.

- `c.txt` (generated in step 1(d) above)
- `m.txt` (generated in step 1(e) above)
- `mi.txt` (generated in step 1(f) above)
- `config.log` (generated in step 1(d) above)
- `src/mpl/config.log` (generated in step 1(d) above)
- `src/pm/hydra/config.log` (generated in step 1(d) above)

**DID WE MENTION? DO NOT FORGET TO COMPRESS THESE FILES!**

If you have compiled MPICH and are having trouble running an application, please provide the output of the following command in your email.

```bash
mpiexec -info
```

Finally, please include the actual error you are seeing when running the application, including the mpiexec command used, and the host file. If possible, please try to reproduce the error with a smaller application or benchmark and send that along in your bug report.

4. If you have found a bug in MPICH, you can report it on our [Github page](https://github.com/pmodels/mpich/issues).

---

## 7. Developer Builds

For MPICH developers who want to directly work on the primary version control system, there are a few additional steps involved (people using the release tarballs do not have to follow these steps). Details about these steps can be found here: [Github](https://github.com/pmodels/mpich/blob/main/doc/wiki/source_code/Github.md)

---

## 8. Compiler Flags

MPICH allows several sets of compiler flags to be used. The first three sets are configure-time options for MPICH, while the fourth is only relevant when compiling applications with mpicc and friends.

### Flag Types

**(a) `CFLAGS`, `CPPFLAGS`, `CXXFLAGS`, `FFLAGS`, `FCFLAGS`, `LDFLAGS` and `LIBS`** (abbreviated as `xFLAGS`): Setting these flags would result in the MPICH library being compiled/linked with these flags and the flags internally being used in mpicc and friends.

**(b) `MPICHLIB_CFLAGS`, `MPICHLIB_CPPFLAGS`, `MPICHLIB_CXXFLAGS`, `MPICHLIB_FFLAGS`, `MPICHLIB_FCFLAGS`, `MPICHLIB_LDFLAGS` and `MPICHLIB_LIBS`** (abbreviated as `MPICHLIB_xFLAGS`): Setting these flags would result in the MPICH library being compiled/linked with these flags. However, these flags will *not* be used by mpicc and friends.

**(c) `MPICH_MPICC_CFLAGS`, `MPICH_MPICC_CPPFLAGS`, `MPICH_MPICC_LDFLAGS`, `MPICH_MPICC_LIBS`, and so on for `MPICXX`, `MPIF77` and `MPIFORT`** (abbreviated as `MPICH_MPIX_FLAGS`): These flags do *not* affect the compilation of the MPICH library itself, but will be internally used by mpicc and friends.

### Flag Usage Summary

| Flag Type | MPICH Library | mpicc and friends |
|-----------|---------------|-------------------|
| `xFLAGS` | Yes | Yes |
| `MPICHLIB_xFLAGS` | Yes | No |
| `MPICH_MPIX_FLAGS` | No | Yes |

All these flags can be set as part of configure command or through environment variables.

### Default Flags

By default, MPICH automatically adds certain compiler optimizations to `MPICHLIB_CFLAGS`. The currently used optimization level is `-O2`.

**IMPORTANT NOTE**: Remember that this only affects the compilation of the MPICH library and is not used in the wrappers (mpicc and friends) that are used to compile your applications or other libraries.

This optimization level can be changed with the `--enable-fast` option passed to configure. For example, to build an MPICH environment with `-O3` for all language bindings, one can simply do:

```bash
./configure --enable-fast=O3
```

Or to disable all compiler optimizations, one can do:

```bash
./configure --disable-fast
```

For more details of `--enable-fast`, see the output of "configure --help".

For performance testing, we recommend the following flags:

```bash
./configure --enable-fast=O3,ndebug --disable-error-checking --without-timing \
            --without-mpit-pvars
```

### Examples

**Example 1:**

```bash
./configure --disable-fast MPICHLIB_CFLAGS=-O3 MPICHLIB_FFLAGS=-O3 \
      MPICHLIB_CXXFLAGS=-O3 MPICHLIB_FCFLAGS=-O3
```

This will cause the MPICH libraries to be built with `-O3`, and `-O3` will *not* be included in the mpicc and other MPI wrapper script.

**Example 2:**

```bash
./configure --disable-fast CFLAGS=-O3 FFLAGS=-O3 CXXFLAGS=-O3 FCFLAGS=-O3
```

This will cause the MPICH libraries to be built with `-O3`, and `-O3` will be included in the mpicc and other MPI wrapper script.

---

## 9. Multiple Fortran Computer Support

If the C compiler that is used to build MPICH libraries supports both multiple weak symbols and multiple aliases of common symbols, the Fortran binding can support multiple Fortran compilers. The multiple weak symbols support allow MPICH to provide different name mangling scheme (of subroutine names) required by different Fortran compilers. The multiple aliases of common symbols support enables MPICH to equal different common block symbols of the MPI Fortran constant, e.g. MPI_IN_PLACE, MPI_STATUS_IGNORE. So they are understood by different Fortran compilers.

Since the support of multiple aliases of common symbols is new/experimental, users can disable the feature by using configure option `--disable-multi-aliases` if it causes any undesirable effect, e.g. linker warnings of different sizes of common symbols, MPIFCMB* (the warning should be harmless).

We have only tested this support on a limited set of platforms/compilers.  On linux, if the C compiler that builds MPICH is either gcc or icc, the above support will be enabled by configure.  At the time of this writing, pgcc does not seem to have this multiple aliases of common symbols, so configure will detect the deficiency and disable the feature automatically.  The tested Fortran compilers include GNU Fortran compilers (gfortan), Intel Fortran compiler (ifort), Portland Group Fortran compilers (pgfortran), Absoft Fortran compilers (af90), and IBM XL fortran compiler (xlf).  What this means is that if mpich is built by gcc/gfortran, the resulting mpich library can be used to link a Fortran program compiled/linked by another fortran compiler, say pgf90, say through mpifort -fc=pgf90.  As long as the Fortran program is linked without any errors by one of these compilers, the program shall be running fine.

---

## 10. ABI Compatibility

The MPICH ABI compatibility initiative was announced at SC 2014 (http://www.mpich.org/abi).  As a part of this initiative, Argonne, Intel, IBM and Cray have committed to maintaining ABI compatibility with each other.

MPICH maintains binary (ABI) compatibility with other MPI implementations participating in this initiative. This means you can build your program with one MPI implementation and run with the other. Specifically, binary-only applications that were built and distributed with one of these MPI implementations can now be executed with the other MPI implementation.

Some setup is required to achieve this.  Suppose you have MPICH installed in /path/to/mpich and Intel MPI installed in /path/to/impi.

You can run your application with mpich using:

```bash
export LD_LIBRARY_PATH=/path/to/mpich/lib:$LD_LIBRARY_PATH
mpiexec -np 100 ./foo
```

or using Intel MPI using:

```bash
export LD_LIBRARY_PATH=/path/to/impi/lib:$LD_LIBRARY_PATH
mpiexec -np 100 ./foo
```

This works irrespective of which MPI implementation your application was compiled with, as long as you use one of the MPI implementations in the ABI compatibility initiative.

---

## 11. Citing MPICH

```bibtex
@misc{MPICH,
    title = {MPICH},
    author = {Gropp, William and Lusk, Ewing (Rusty) and Thakur, Rajeev and Balaji, Pavan and Gillis, Thomas and Guo, Yanfei and Latham, Rob and Raffenetti, Ken and Zhou, Hui},
    abstractNote = {MPICH is a high-performance and widely portable implementation of the MPI-4 standard from the Argonne National Laboratory. This release has all MPI 4 functions and features required by the standard with the exception of support for the user-defined data representations for I/O.},
    doi = {10.11578/dc.20200514.13},
    url = {https://doi.org/10.11578/dc.20200514.13},
    howpublished = {[Computer Software] \url{https://doi.org/10.11578/dc.20200514.13}},
    year = {2023},
    month = {jun}
}

@article{10.1177/10943420241311608,
    author = {Guo, Yanfei and Raffenetti, Ken and Zhou, Hui and Balaji, Pavan and Si, Min and Amer, Abdelhalim and Iwasaki, Shintaro and Seo, Sangmin and Congiu, Giuseppe and Latham, Robert and Oden, Lena and Gillis, Thomas and Zambre, Rohit and Ouyang, Kaiming and Archer, Charles and Bland, Wesley and Jose, Jithin and Sur, Sayantan and Fujita, Hajime and Durnov, Dmitry and Chuvelev, Michael and Zheng, Gengbin and Brooks, Alex and Thapaliya, Sagar and Doodi, Taru and Garazan, Maria and Oyanagi, Steve and Snir, Marc and Thakur, Rajeev},
    title = {Preparing MPICH for exascale},
    year = {2025},
    issue_date = {Mar 2025},
    publisher = {Sage Publications, Inc.},
    address = {USA},
    volume = {39},
    number = {2},
    issn = {1094-3420},
    url = {https://doi.org/10.1177/10943420241311608},
    doi = {10.1177/10943420241311608},
    journal = {Int. J. High Perform. Comput. Appl.},
    month = mar,
    pages = {283–305},
    numpages = {23},
    keywords = {Message passing interface, MPI, HPC communication, HPC network, exascale MPI}
}

@article{10.1177/10943420241263544,
    author = {Heroux, Michael and Zhou, Hui and Raffenetti, Ken and Guo, Yanfei and Gillis, Thomas and Latham, Robert and Thakur, Rajeev},
    title = {Designing and prototyping extensions to the Message Passing Interface in MPICH},
    year = {2024},
    issue_date = {Sep 2024},
    publisher = {Sage Publications, Inc.},
    address = {USA},
    volume = {38},
    number = {5},
    issn = {1094-3420},
    url = {https://doi.org/10.1177/10943420241263544},
    doi = {10.1177/10943420241263544},
    journal = {Int. J. High Perform. Comput. Appl.},
    month = sep,
    pages = {527–545},
    numpages = {19},
    keywords = {Distributed computing, message passing interface, message passing interface+threads, message passing interface+graphics processing units, MPICH}
}
```
