# BGQ

This page describes how to build mpich from the
[main](https://github.com/pmodels/mpich/tree/main)
branch of the MPICH git [repository](https://github.com/pmodels/mpich) repository on
git.mpich.org.

## Blue Gene/Q Build Instructions

A bgq toolchain must be specified to cross compile the mpich source. The
toolchain can be specified explicitly by setting the CC, CXX, and other
environment variables to the desired compilers, or configure will detect
and use the cross compilers that are specified in the PATH environment
variable.

### Allow Configure To Determine The Compilers

The configure is simpler when specifying the $PATH, however the
generated mpi compile scripts such as `${prefix}/bin/mpicc` will also
not contain the path information to the cross compiler. This means users
of `mpicc` must also have the cross compiler in their $PATH for the
compile script to work.

The current compiler search order in mpich is specified in various m4
macro files. For example, the C compiler search path is specified in the
[confdb/aclocal_cc.m4](https://github.com/pmodels/mpich/blob/main/confdb/aclocal_cc.m4)
file.

```
AC_PROG_CC([icc pgcc xlc xlC pathcc gcc clang cc])
```

This means that on bgq the following compilers will be searched for in
the user's $PATH and the first found will be used. Care should be taken
when modifying the $PATH environment variable - especially when multiple
toolchains are specified.

1.  `powerpc64-bgq-linux-icc`
2.  `powerpc64-bgq-linux-pgcc`
3.  `powerpc64-bgq-linux-xlc`
4.  `powerpc64-bgq-linux-xlC`
5.  `powerpc64-bgq-linux-pathcc`
6.  `powerpc64-bgq-linux-gcc`
7.  `powerpc64-bgq-linux-clang`
8.  `powerpc64-bgq-linux-cc`

#### BGQ GNU Toolchain

To use the gnu toolchain installed with the BGQ system software, edit
the user's `$PATH` as below.

##### GCC Version 4.4.6

This is the default toolchain installed with the bgq system software.

```
export PATH=$PATH:/bgsys/drivers/V1R2M1/ppc64/gnu-linux/bin
```

##### GCC Version 4.7.2

The 4.7.2 version of the gnu toolchain is unsupported by IBM, however
instructions for building and installing a 4.7.2 toolchain are provided
with the system software. See the
`/bgsys/drivers/V1R2M1/ppc64/toolchain-4.7.2/README.toolchain` file for
more information.

The user's `$PATH` must be similarly modified once the 4.7.2 toolchain is
installed.

```
export PATH=$PATH:${GNU_TOOLCHAIN_4_7_2}/gnu-linux-4.7.2/bin
```

#### BGQ XL Toolchain

Beginning with V1R2M1 appropriately named symlinks to the bg xl
compilers are provided with the installed system software.

```
$ ls -lah /bgsys/drivers/ppcfloor/gnu-linux/powerpc64-bgq-linux/bin/*xl*
lrwxrwxrwx 1 root root 35 May 20 16:19 powerpc64-bgq-linux-xlc -> /opt/ibmcmp/vac/bg/12.1/bin/bgxlc_r
lrwxrwxrwx 1 root root 37 May 20 16:19 powerpc64-bgq-linux-xlC -> /opt/ibmcmp/vacpp/bg/12.1/bin/bgxlC_r
lrwxrwxrwx 1 root root 35 May 20 16:19 powerpc64-bgq-linux-xlf -> /opt/ibmcmp/xlf/bg/14 .1/bin/bgxlf_r
lrwxrwxrwx 1 root root 39 May 20 16:19 powerpc64-bgq-linux-xlf2003 -> /opt/ibmcmp/xlf/bg/14.1/bin/bgxlf2003_r
lrwxrwxrwx 1 root root 39 May 20 16:19 powerpc64-bgq-linux-xlf2008 -> /opt/ibmcmp/xlf/bg/14.1/bin/bgxlf2008_r
lrwxrwxrwx 1 root root 37 May 20 16:19 powerpc64-bgq-linux-xlf90 -> /opt/ibmcmp/xlf/bg/14.1/bin/bgxlf90_r
lrwxrwxrwx 1 root root 37 May 20 16:19 powerpc64-bgq-linux-xlf95 -> /opt/ibmcmp/xlf/bg/14.1/bin/bgxlf95_r
```

To use the bg xl compilers, the user's path must be modified as below.

```
export PATH=$PATH:/bgsys/drivers/ppcfloor/gnu-linux/powerpc64-bgq-linux/bin
```

Prior to V1R2M1 the symlinks to the bg xl compilers are not provided.
The solution is to simply create the symlinks in some other directory
and modify the user's $PATH accordingly.

```
$ ls -lah ${HOME}/bgxl/powerpc64-bgq-linux/bin | grep ^l
lrwxrwxrwx 1 johndoe johndoe   35 Jul 18 07:30 powerpc64-bgq-linux-xlc -> /opt/ibmcmp/vac/bg/12.1/bin/bgxlc_r
lrwxrwxrwx 1 johndoe johndoe   37 Jul 18 07:30 powerpc64-bgq-linux-xlC -> /opt/ibmcmp/vacpp/bg/12.1/bin/bgxlC_r
lrwxrwxrwx 1 johndoe johndoe   35 Jul 18 07:30 powerpc64-bgq-linux-xlf -> /opt/ibmcmp/xlf/bg/14.1/bin/bgxlf_r
lrwxrwxrwx 1 johndoe johndoe   39 Jul 18 07:30 powerpc64-bgq-linux-xlf2003 -> /opt/ibmcmp/xlf/bg/14.1/bin/bgxlf2003_r
lrwxrwxrwx 1 johndoe johndoe   39 Jul 18 07:30 powerpc64-bgq-linux-xlf2008 -> /opt/ibmcmp/xlf/bg/14.1/bin/bgxlf2008_r
lrwxrwxrwx 1 johndoe johndoe   37 Jul 18 07:30 powerpc64-bgq-linux-xlf90 -> /opt/ibmcmp/xlf/bg/14.1/bin/bgxlf90_r
lrwxrwxrwx 1 johndoe johndoe   37 Jul 18 07:30 powerpc64-bgq-linux-xlf95 -> /opt/ibmcmp/xlf/bg/14.1/bin/bgxlf95_r

$ export PATH=$PATH:${HOME}/bgxl/powerpc64-bgq-linux/bin
```

#### BGQ Clang Toolchain

A bgq version of the clang/llvm compilers, provided by ANL, can be
installed and similarly referenced.

See <http://www.alcf.anl.gov/user-guides/bgclang-compiler> and
<https://trac.alcf.anl.gov/projects/llvm-bgq> for more information.

```
export PATH=$PATH:/home/projects/llvm/wbin/bgclang/bin
```

### Override Configure Settings With The compiler Environment Variables

The relevant compiler environment variables can be directly specified on
the configure command. This will override any compiler settings
determined by configure.

This approach will specify the full path to the compilers in the
generated `${prefix}/bin/mpicc` script. Users will **not** be required
to have the bgq cross compilers in their `$PATH`.

```
$ CC=/soft/compilers/ibmcmp-feb2014/vac/bg/12.1/bin/bgxlc_r                 \
CXX=/soft/compilers/ibmcmp-feb2014/vacpp/bg/12.1/bin/bgxlC_r                \
F77=/soft/compilers/ibmcmp-feb2014/xlf/bg/14.1/bin/bgxlf_r                  \
FC=/soft/compilers/ibmcmp-feb2014/xlf/bg/14.1/bin/bgxlf90_r                 \
AR=/bgsys/drivers/V1R2M1/ppc64/gnu-linux/powerpc64-bgq-linux/bin/ar         \
LD=/bgsys/drivers/V1R2M1/ppc64/gnu-linux/powerpc64-bgq-linux/bin/ld         \
RANLIB=/bgsys/drivers/V1R2M1/ppc64/gnu-linux/powerpc64-bgq-linux/bin/ranlib \
configure --host=powerpc64-bgq-linux --with-device=pamid
```

### Required

Specify the bgq cross compile and pamid device

```
--host=powerpc64-bgq-linux
--with-device=pamid
```

Customize the ROMIO file system.

The latest versions of mpich include a ROMIO implementation tuned for
GPFS and further optimized for the Blue Gene environment.

```
--with-file-system=gpfs:BGQ
```

In mpich 3.1 and earlier versions the GPFS adio is unavailable and
instead included a unique Blue Gene adio implementation.

```
--with-file-system=bg+bglockless
```

### Optional

#### Customize The Required BGQ System Software Libraries

The latest installed bgq system software is used by default. The
location of the bgq system software can also be specified with the
configure option below or the `BGQ_INSTALL_DIR` environment
variable.

```
--with-bgq-install-dir=/bgsys/drivers/V1R2M0/ppc64
```

A pami installation outside of the bgq system software directory may be
specified using the `--with-pami` configure option(s). For example:

```
--with-pami=/bgsys/drivers/V1R2M0/ppc64/comm/sys
--with-pami-include=/bgsys/drivers/V1R2M0/ppc64/comm/sys/include
--with-pami-lib=/bgsys/drivers/V1R2M0/ppc64/comm/sys/lib
```

#### Customize The BGQ Cross Compile Settings

A different cross compile settings file for bgq pamid can be specified
using the `--with-cross-file` configure option. Below is the configure
option that specifies what is the default cross file for a bgq pamid
configuration.

```
--with-cross-file=src/mpid/pamid/cross/bgq8
```

#### Disable rpath

When shared libraries are installed it is recommended to also *disable*
the "wrapper rpath" configure option in order to take advantage of a
shared library load optimization on the bgq io nodes.

```
--disable-wrapper-rpath
```

When a million processes each individually read from the filesystem the
performance of the shared library load will be poor. The io node shared
library optimization is a way to "stage" shared libraries on a bgq io
node ramfs directory that is, in the absence of rpath information,
searched first by the bgq loader. Any rpath information will be searched
before this io node ramfs location and will result in a query all the
way down to the filesystem.

Shared libraries can be added to the io node ramfs directory by
packaging the libraries into a `\*.tar.gz` file and copying that file
into the `/bgsys/linux/bgfs` directory.

#### Enable Common "no debug" And "performance" Options

The *xl.ndebug* and *xl.legacy.ndebug* mpich versions installed with the
bgq system software use the following options to eliminate debug and
other error checks that would cause performance degradations.

```
--enable-fast=nochkmsg,notiming,O3
--with-assert-level=0
--disable-error-messages
--disable-debuginfo
```

#### Enable Fine Grain Locking

The *gcc*, *xl*, and *xl.ndebug* mpich versions installed with the bgq
system software use the following options to enable fine grain locking
and synchronous progress mode.

```
--enable-thread-cs=per-object
--with-atomic-primitives
--enable-handle-allocation=tls
--enable-refcount=lock-free
--disable-predefined-refcount
```

#### Specify Compiler Options

The following compiler flags are used when compiling the mpich library
to be included in the bgq system software installation and are provided
here for guidance.

##### GNU Compiler Options

```
MPICHLIB_CXXFLAGS="-Wall -Wno-unused-function -Wno-unused-label -Wno-unused-variable -fno-strict-aliasing"
MPICHLIB_CFLAGS="${MPICHLIB_CXXFLAGS} -Wno-implicit-function-declaration"
MPICHLIB_FFLAGS="${MPICHLIB_CXXFLAGS}"
MPICHLIB_F90FLAGS="${MPICHLIB_CXXFLAGS}"
```

##### XL Compiler Options

```
MPICHLIB_CXXFLAGS="-qhot -qinline=800 -qflag=i:i -qsaveopt -qsuppress=1506-236"
MPICHLIB_CFLAGS="${MPICHLIB_CXXFLAGS}"
MPICHLIB_FFLAGS="${MPICHLIB_CXXFLAGS}"
MPICHLIB_F90FLAGS="${MPICHLIB_CXXFLAGS}"
```

## Blue Gene/Q Mpich Testsuite Instructions

From a filesystem location that is accessible to the Blue Gene/Q io
nodes, for example `/bgusr/johndoe`, invoke the configure script in the
`test/mpi` directory of the mpich source.

```
$ mkdir /home/johndoe/testsuite && cd /home/johndoe/testsuite
$ /home/johndoe/mpich/test/mpi/configure --srcdir=/home/johndoe/mpich/test/mpi --with-mpi=/home/johndoe/mpich/install
```

The `--srcdir` configure option specifies the location of the testsuite
source, the `--with-mpi` configure option specifies which mpi
installation to use when compiling the tests. Other configure options
may be specified, such as `--disable-spawn`, to skip unsupported
functions.

Once configured, the tests can be compiled and executed using the 
`make testing` makefile rule. Specific make variables will need to be
specified depending on how the jobs are to be launched on a Blue Gene/Q
system.

The `MPIEXEC` variable is needed to specify the job launch mechanism,
which on Blue Gene/Q is the `runjob` command. For more information on
the `runjob` command see chapter 6, **"Submitting jobs"** in the **[IBM
System Blue Gene Solution: Blue Gene/Q System
Administration](http://www.redbooks.ibm.com/redbooks/pdfs/sg247869.pdf)**
redbook.

The `MPITEST_PROGRAM_WRAPPER` variable is needed to supply additional
information to the `runjob` command. This "wrapper" text is inserted
**after** the `$MPIEXEC` command and its arguments, such as the number
of processes in the job, and **before** the name of the test binary to
launch. At a minimum the *runjob* command needs to have the compute
block specified and the ':' separator character specified. Other
`runjob` options can be specified as well, such as `--timeout`, although
these are not required to launch the job.

### `bg_console`

Before testing with `runjob`, and directly launching the jobs on a Blue
Gene/Q system, the compute block must be allocated. Typically this is
done using the `bg_console` command shell. For more information on
`bg_console` see section **"Creating and booting I/O blocks and compute
blocks"** in the **[IBM System Blue Gene Solution: Blue Gene/Q System
Administration](http://www.redbooks.ibm.com/redbooks/pdfs/sg247869.pdf)**
redbook.

To begin testing, change to the directory where the configure command
was run (`/bgusr/johndoe/testsuite` in this example), locate the block
that was booted (`R00-M1-N06` in this example), and invoke the following
command:

```
$ cd /home/johndoe/testsuite
$ make testing MPITEST_PROGRAM_WRAPPER=" --block R00-M1-N06 : " MPIEXEC=runjob
```

### Cobalt

Before testing with `runjob`, and directly launching the jobs on a Blue
Gene/Q system, the job must be launched in "interactive" mode. This will
fork a new shell with several important environment variables set.

```
$ qsub -A aurora_app -t 120 -n 32 --mode interactive
Wait for job 294154 to start...
Opening interactive session to VST-02230-13331-32
```

Additional runjob options may be needed if the Blue Gene/Q installation
has changed the default behavior of the trace loggers

```
--verbose ibm.runjob=0 --verbose 0`
```

in this example. To begin testing, change to the directory where the 
configure command was run (`/bgusr/johndoe/testsuite` in this example) and invoke the following
command:

```
$ cd /home/johndoe/testsuite
$ make testing MPITEST_PROGRAM_WRAPPER=" --block $COBALT_PARTNAME --timeout 60 --verbose ibm.runjob=0 --verbose 0 : " MPIEXEC=runjob
```

## Blue Gene/Q development instructions

The product release branches in the
[mpich-ibm.git](http://git.mpich.org/mpich-ibm.git) git repository are
based on the mpich2 1.5 release, and for esoteric historical reasons,
the code in the repository is located in a *mpich2* subdirectory that
does not exist in the original mpich source. This extra directory makes
a simple *git cherry-pick* of a commit on a Blue Gene/Q release branch
on to another mpich branch challenging.

### How To Migrate Commits From A Previous Blue Gene/Q Release Branch

#### Use `git format-patch` To Create Patch Files For Each Commit

For example:

```
git checkout BGQ/IBM_V1R2M0
Checking out files: 100% (8574/8574), done.
Branch BGQ/IBM_V1R2M0 set up to track remote branch BGQ/IBM_V1R2M0 from origin.
Switched to a new branch 'BGQ/IBM_V1R2M0'

git format-patch HEAD~4
0001-CPS-92XKPE-remove-fortran-interface-for-MPIX_Pset_io.patch
0002-CPS-92XKPE-Do-not-use-the-MPIX_Pset_io_node-function.patch
0003-CPS-97VH5U-do-not-disable-short-synchronous-sends.patch
0004-CPS-97RGJN-PAMID-only-fix-for-multi-threaded-MPI_Ibs.patch
```

#### Use `git am` To Apply Each Commit

You may need to edit the commit message into an acceptable format using
`git commit --amend`.

  - If the patch contains the leading `mpich2/` directory then this
    directory must be removed as the patch is applied by using the
    `-p2` option; for example:

```
git am -p2 0001-CPS-92XKPE-remove-fortran-interface-for-MPIX_Pset_io.patch
```

  - The "summary" line of the commit message must not contain any IBM
    "breadcrumbs" such as "Issue 1234", "CPS WXYZ", or "D12345". These
    breadcrumbs need to be moved to the body of the commit message and
    prepended with the "(ibm)" namespace. It is good form to add the
    original commit as well. This helps when tracing the history of the
    code change via gitweb, etc. For example,

```
git log -n1 b68401e3c6ba3bbd2cc0626dac1604242a20f989 # The original commit to be migrated
commit b68401e3c6ba3bbd2cc0626dac1604242a20f989
Author: Michael Blocksome <blocksom@us.ibm.com>
Date:   Mon Apr 29 13:14:35 2013 -0500

    CPS 92XKPE: remove fortran interface for MPIX_Pset_io_node()
    
    The MPIX_Pset_io_node() function has been deprecated.

git commit --amend
git log -n1
commit ee5e30e4ed5cddd10e2ebf72087174284d8d590b
Author: Michael Blocksome <blocksom@us.ibm.com>
Date:   Mon Apr 29 13:14:35 2013 -0500

    Remove fortran interface for MPIX_Pset_io_node()
    
    The MPIX_Pset_io_node() function has been deprecated.

    (ibm) CPS 92XKPE
    (ibm) b68401e3c6ba3bbd2cc0626dac1604242a20f989
```

#### Use `git apply` To Repair Any Merge Conflicts

If `git am` fails to apply a patch it must be applied manually. The
`git am` command places the current patch in the
`.git/rebase_apply/` directory in a file named `0001`. The `git apply` 
command must be used on this patch to create "reject" files that
can be used to manually repair the files:

```
git apply .git/rebase_apply/0001 -p2 --reject
    # edit edit edit
git add FIXED_FILES
git am --resolved
```

### How To Migrate Commits From MPICH Master To Blue Gene/Q Release Branch

The process is much the same as above:

  - `git format-patch` to get the changes in question
  - `git am to apply changes`, but use the `--directory=mpich2` flag to
    indicate the Blue Gene/Q release branch tree lives one directory
    lower. No need for the `-p2` flag.
  - if there are conflicts, generate rejects with 
    `git apply --directory=mpich2 .git/rebase-apply/0001 --reject`
