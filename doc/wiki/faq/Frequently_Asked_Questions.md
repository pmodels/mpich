# Frequently Asked Questions

## General Information

#### Q: What is MPICH?

A: MPICH is a freely available, portable implementation of MPI, the
Standard for message-passing libraries. It implements all versions of
the MPI standard including MPI-1, MPI-2, MPI-2.1, MPI-2.2, and MPI-3.

#### Q: What does MPICH stand for?

A: MPI stands for Message Passing Interface. The CH comes from
Chameleon, the portability layer used in the original MPICH to provide
portability to the existing message-passing systems.

#### Q: Can MPI be used to program multicore systems?

A: There are two common ways to use MPI with multicore processors or
multiprocessor nodes:

1.  Use one MPI process per core (here, a core is defined as a program
    counter and some set of arithmetic, logic, and load/store units).
2.  Use one MPI process per node (here, a node is defined as a
    collection of cores that share a single address space). Use threads
    or compiler-provided parallelism to exploit the multiple cores.
    OpenMP may be used with MPI; the loop-level parallelism of OpenMP
    may be used with any implementation of MPI (you do not need an MPI
    that supports MPI_THREAD_MULTIPLE when threads are used only for
    computational tasks). This is sometimes called the hybrid
    programming model.

MPICH automatically recognizes multicore architectures and optimizes
communication for such platforms. No special configure option is
required.

#### Q: Why can't I build MPICH on Windows anymore?

Unfortunately, due to the lack of developer resources, MPICH is not
supported on Windows anymore including Cygwin. The last version of
MPICH, which was supported on Windows, was MPICH2 1.4.1p1. There is
minimal support left for this version, but you can find it on the
downloads page:

<http://www.mpich.org/downloads/>

Alternatively, Microsoft maintains a derivative of MPICH which should
provide the features you need. You also find a link to that on the
downloads page above. That version is much more likely to work on your
system and will continue to be updated in the future. We recommend all
Windows users migrate to using MS-MPI.

## Building MPICH

#### Q: What are process managers?

A: Process managers are basically external (typically distributed)
agents that spawn and manage parallel jobs. These process managers
communicate with MPICH processes using a predefined interface called as
PMI (process management interface). Since the interface is (informally)
standardized within MPICH and its derivatives, you can use any process
manager from MPICH or its derivatives with any MPI application built
with MPICH or any of its derivatives, as long as they follow the same
wire protocol. There are three known implementations of the PMI wire
protocol: "simple", "smpd" and "slurm". By default, MPICH and all its
derivatives use the "simple" PMI wire protocol, but MPICH can be
configured to use "smpd" or "slurm" as well.

For example, MPICH provides several different process managers such as
Hydra, MPD, Gforker and Remshell which follow the "simple" PMI wire
protocol. MVAPICH2 provides a different process manager called "mpirun"
that also follows the same wire protocol. OSC mpiexec follows the same
wire protocol as well. You can mix and match an application built with
any MPICH derivative with any process manager. For example, an
application built with Intel MPI can run with OSC mpiexec or MVAPICH2's
mpirun or MPICH's Gforker.

MPD has been the traditional default process manager for MPICH till the
1.2.x release series. Starting the 1.3.x series, Hydra is the default
process manager.

SMPD is another process manager distributed with MPICH that uses the
"smpd" PMI wire protocol. This is mainly used for running MPICH on
Windows or a combination of UNIX and Windows machines. This will be
deprecated in the future releases of MPICH in favour or Hydra. MPICH can
be configured with SMPD using:

```
 ./configure --with-pm=smpd --with-pmi=smpd
```

SLURM is an external process manager that uses MPICH's PMI interface as
well.

#### Note that the default build of MPICH will work fine in SLURM environments. No extra steps are needed.

However, if you want to use the srun tool to launch jobs instead of the
default mpiexec, you can configure MPICH as follows:

```
 ./configure --with-pm=none --with-pmi=slurm
```

Once configured with slurm, no internal process manager is built for
MPICH; the user is expected to use SLURM's launch models (such as srun).

#### Q: Do I have to configure/make/install MPICH each time for each compiler I use?

A: No, in many cases you can build MPICH using one set of compilers and
then use the libraries (and compilation scripts) with other compilers.
However, this depends on the compilers producing compatible object
files. Specifically, the compilers must

1.  Support the same basic datatypes with the same sizes. For example,
    the C compilers should use the same sizes for long long and long
    double.
2.  Map the names of routines in the source code to names in the object
    files in the object file in the same way. This can be a problem for
    Fortran and C++ compilers, though you can often force the Fortran
    compilers to use the same name mapping. More specifically, most
    Fortran compilers map names in the source code into all lower-case
    with one or two underscores appended to the name. To use the same
    MPICH library with all Fortran compilers, those compilers must make
    the same name mapping. There is one exception to this that is
    described below.
3.  Perform the same layout for C structures. The C language does not
    specify how structures are laid out in memory. For 100\\%
    compatibility, all compilers must follow the same rules. However, if
    you do not use any of the MPI_MIN_LOC or MPI_MAX_LOC datatypes,
    and you do not rely on the MPICH library to set the extent of a type
    created with MPI_Type_struct or MPI_Type_create_struct, you can
    often ignore this requirement.
4.  Require the same additional runtime libraries. Not all compilers
    will implement the same version of Unix, and some routines that
    MPICH uses may be present in only some of the run time libraries
    associated with specific compilers.

The above may seem like a stringent set of requirements, but in
practice, many systems and compiler sets meet these needs, if for no
other reason than that any software built with multiple libraries will
have requirements similar to those of MPICH for compatibility.

If your compilers are completely compatible, down to the runtime
libraries, you may use the compilation scripts (mpicc etc.) by either
specifying the compiler on the command line, e.g.

```
mpicc -cc=icc -c foo.c
```

or with the environment variables MPICH_CC etc. (this example assume a
c-shell syntax):

```
setenv MPICH_CC icc
mpicc -c foo.c
```

If the compiler is compatible except for the runtime libraries, then
this same format works as long as a configuration file that describes
the necessary runtime libraries is created and placed into the
appropriate directory (the "sysconfdir" directory in configure terms).
See the installation manual for more details.

In some cases, MPICH is able to build the Fortran interfaces in a way
that supports multiple mappings of names from the Fortran source code to
the object file. This is done by using the "multiple weak symbol"
support in some environments. For example, when using gcc under Linux,
this is the default.

#### Q: How do I configure to use the Absoft Fortran compilers?

A: You can find build instructions on the
[Absoft](http://www.absoft.com/Products/Compilers/Fortran/Linux/fortran95/MPich_Instructions.html) 
web site at the bottom of the page.


#### Q: When I configure MPICH, I get a message about FDZERO and the configure aborts.

A: FD_ZERO is part of the support for the select calls (see \`\`man
select'' or \`\`man 2 select'' on Linux and many other Unix systems) .
What this means is that your system (probably a Mac) has a broken
version of the select call and related data types. This is an OS bug;
the only repair is to update the OS to get past this bug. This test was
added specifically to detect this error; if there was an easy way to
work around it, we would have included it (we don't just implement
FD_ZERO ourselves because we don't know what else is broken in this
implementation of select).

If this configure works with gcc but not with xlc, then the problem is
with the include files that xlc is using; since this is an OS call (even
if emulated), all compilers should be using consistent if not identical
include files. In this case, you may need to update xlc.

#### Q: When I use the g95 Fortran compiler on a 64-bit platform, some of the tests fail.

A: The g95 compiler incorrectly defines the default Fortran integer as a
64-bit integer while defining Fortran reals as 32-bit values (the
Fortran standard requires that INTEGER and REAL be the same size). This
was apparently done to allow a Fortran INTEGER to hold the value of a
pointer, rather than requiring the programmer to select an INTEGER of a
suitable KIND. To force the g95 compiler to correctly implement the
Fortran standard, use the -i4 flag. For example, set the environment
variable F90FLAGS before configuring MPICH:

```
setenv F90FLAGS "-i4"
```

G95 users should note that there (at this writing) are two distributions
of g95 for 64-bit Linux platforms. One uses 32-bit integers and reals
(and conforms to the Fortran standard) and one uses 32-bit integers and
64-bit reals. We recommend using the one that conforms to the standard
(note that the standard specifies the ratio of sizes, not the absolute
sizes, so a Fortran 95 compiler that used 64 bits for both INTEGER and
REAL would also conform to the Fortran standard. However, such a
compiler would need to use 128 bits for DOUBLE PRECISION quantities).

#### Q: Make fails with errors such as these:

```
sock.c:8:24: mpidu_sock.h: No such file or directory
In file included from sock.c:9:
../../../../include/mpiimpl.h:91:21:
mpidpre.h: No such file or directory
In file included from sock.c:9:
../../../../include/mpiimpl.h:1150:
error: syntax error before "MPID_VCRT"
../../../../include/mpiimpl.h:1150: warning: no semicolon at end of struct or union
```

A: Check if you have set the environment variable CPPFLAGS. If so, unset
it and use CXXFLAGS instead. Then rerun configure and make.

#### Q: When building the ssm channel, I get this error:

```
mpidu_process_locks.h:234:2: error: \#error *** No atomic memory operation specified to implement busy locks ***
```

A: The ssm channel does not work on all platforms because they use
special interprocess locks (often assembly) that may not work with some
compilers or machine architectures. It works on Linux with gcc, Intel,
and Pathscale compilers on various Intel architectures. It also works in
Windows and Solaris environments.

This channel is now deprecated. Please use the ch3:nemesis channel,
which is more portable and performs better than ssm.

#### Q: When using the Intel Fortran 90 compiler (version 9), the make fails with errors in compiling statement that reference MPI_ADDRESS_KIND.

A: Check the output of the configure step. If configure claims that
ifort is a cross compiler, the likely problem is that programs compiled
and linked with ifort cannot be run because of a missing shared library.
Try to compile and run the following program (named conftest.f90):

```
program conftest
integer, dimension(10) :: n
end
```

If this program fails to run, then the problem is that your installation
of ifort either has an error or you need to add additional values to
your environment variables (such as LD_LIBRARY_PATH). Check your
installation documentation for the ifort compiler. See
[here](http://softwareforums.intel.com/ISN/Community/en-US/search/SearchResults.aspx?q=libimf.so)
for an example of problems of this kind that users are having with version 9 of ifort.

If you do not need Fortran 90, you can configure with `--disable-f90`.

#### Q: The build fails when I use parallel make.

A: Prior to the 1.5a1 release, parallel make (often invoked with make
`-j4`) would cause several job steps in the build process to update the
same library file (libmpich.a) concurrently. Unfortunately, neither the
ar nor the ranlib programs correctly handle this case, and the result is
a corrupted library. For now, the solution is to not use a parallel make
when building MPICH. **However, all releases since 1.5a1 now support
parallel make.** If you are using a recent version of MPICH and seeing
parallel build failures that do not occur with serial builds, please
report the bug to us.

#### Q: I get a configure error saying "Incompatible Fortran and C Object File Types\!"

A: This is a problem with the default compilers available on Mac OS: it
provides a 32-bit C compiler and a 64-bit Fortran compiler (or the other
way around). These two are not compatible with each other. Consider
installing the same architecture compilers. Alternatively, if you do not
need to build Fortran programs, you can disable it with the configure
option --disable-f77 --disable-f90.

## Compiling MPI Programs

#### Q: I get compile errors saying "SEEK_SET is \#defined but must not be for the C++ binding of MPI".

A: This is really a problem in the MPI-2 standard. And good or bad, the
MPICH implementation has to adhere to it. The root cause of this error
is that both stdio.h and the MPI C++ interface use SEEK_SET, SEEK_CUR,
and SEEK_END. You can try adding:

```
#undef SEEK_SET
#undef SEEK_END
#undef SEEK_CUR
```

before `mpi.h` is included, or add the definition

```
-DMPICH_IGNORE_CXX_SEEK
```

to the command line (this will cause the MPI versions of SEEK_SET etc.
to be skipped).

#### Q: I get compile errors saying "error C2555: 'MPI::Nullcomm::Clone' : overriding virtual function differs from 'MPI::Comm::Clone' only by return type or calling convention".

A: This is caused by buggy C++ compilers not implementing part of the
C++ standard. To work around this problem, add the definition:

```
-DHAVE_NO_VARIABLE_RETURN_TYPE_SUPPORT
```

to the CXXFLAGS variable or add a:

```
#define HAVE_NO_VARIABLE_RETURN_TYPE_SUPPORT 1
```

before including `mpi.h`.

## Running MPI Programs

#### Q: I don't like <WHATEVER> about mpd, or I'm having a problem with mpdboot, can you fix it?

A: Short answer: no.

Longer answer: For all releases since version 1.2, we recommend
[using the hydra process manager](../how_to/Using_the_Hydra_Process_Manager.md)
instead of mpd. The mpd process manager has many problems, as well as an
annoying `mpdboot` step that is fragile and difficult to use correctly.
The mpd process manager is deprecated at this point, and most reported
bugs in it will not be fixed.

#### Q: Why did my application exited with a BAD TERMINATION error?

A: If the user application terminates abnormally, MPICH displays a
message such as the following:

```
=====================================================================================
=   BAD TERMINATION OF ONE OF YOUR APPLICATION PROCESSES
=   EXIT CODE: 11
=   CLEANING UP REMAINING PROCESSES
=   YOU CAN IGNORE THE BELOW CLEANUP MESSAGES
==================================================== =================================
APPLICATION TERMINATED WITH THE EXIT STRING: Segmentation fault (signal 11)
```

This means that the application has exited with a segmentation fault.
This is typically an error in the application code, not in MPICH. We
recommend debugging your application using a debugger such as "ddd",
"gdb", "totalview" or "padb" if you run into this error. See the FAQ
entry on debugging for more details.

#### Q: When I build MPICH with the Intel compilers, launching applications shows a libimf.so not found error

A: When MPICH (more specifically mpiexec and its helper minions, such as
hydra_pmi_proxy) is built with the Intel compiler, a dependency is
added to libimf.so. When you execute mpiexec, it expects the library
dependency to be resolved on each node that you are using. If it cannot
find the library on any of the nodes, the following error is reported:

```
hydra_pmi_proxy: error while loading shared libraries: libimf.so: cannot open shared object file: No such file or directory
```

This typically is a problem in the user environment setup. Specifically,
your LD_LIBRARY_PATH is not setup correctly either for interactive
logins or noninteractive logins, or both.

```
% echo $LD_LIBRARY_PATH
/home/balaji/software/tools/install/lib:/software/mvapich2-intel-psm-1.9.5/lib:/soft/intel/13.1.3/lib/intel64:/soft/intel/13.1.3/composer_xe_2013.5.192/mpirt/lib/intel64:/soft/intel/13.1.3/ipp/lib/intel64:/soft/intel/13.1.3/tbb/lib/intel64/gcc4.4:/soft/lcrc/lib:/usr/lib64:/usr/lib
```

```
% ssh b2 echo \$LD_LIBRARY_PATH
/home/balaji/software/tools/install/lib:
```

The above example shows that /soft/intel/13.1.3/lib/intel64 is the
library path for interactive logins, but not for noninteractive logins,
which can cause this error.

A simple way to fix this is to add the above path to libimf.so to your
LD_LIBRARY_PATH in your shell init script (e.g., .bashrc).

#### Q: How do I pass environment variables to the processes of my parallel program?

A: The specific method depends on the process manager and version of
mpiexec that you are using. See the appropriate specific section.

#### Q: How do I pass environment variables to the processes of my parallel program when using the mpd, hydra or gforker process manager?

A: By default, all the environment variables in the shell where mpiexec
is run are passed to all processes of the application program. (The one
exception is LD_LIBRARY_PATH when using MPD and the mpd's are being
run as root.) This default can be overridden in many ways, and
individual environment variables can be passed to specific processes
using arguments to mpiexec. A synopsis of the possible arguments can be
listed by typing:

```
mpiexec -help
```

and further details are available in the Users Guide here:
<http://www.mcs.anl.gov/research/projects/mpich2/documentation/index.php?s=docs>.

#### Q: What determines the hosts on which my MPI processes run?

A: Where processes run, whether by default or by specifying them
yourself, depends on the process manager being used.

If you are using the Hydra process manager, the host file can contain
the number of processes you want on each node. More information on this
can be found [here](../how_to/Using_the_Hydra_Process_Manager.md).

If you are using the gforker process manager, then all MPI processes run
on the same host where you are running mpiexec.

If you are using mpd, then before you run mpiexec, you will have
started, or will have had started for you, a ring of processes called
mpd's (multi-purpose daemons), each running on its own host. It is
likely, but not necessary, that each mpd will be running on a separate
host. You can find out what this ring of hosts consists of by running
the program mpdtrace. One of the mpd's will be running on the
local machine, the one where you will run mpiexec. The default
placement of MPI processes, if one runs

```
mpiexec -n 10 a.out
```

is to start the first MPI process (rank 0) on the local machine and then
to distribute the rest around the mpd ring one at a time. If there are
more processes than mpd's, then wraparound occurs. If there are more
mpd's than MPI processes, then some mpd's will not run MPI processes.
Thus any number of processes can be run on a ring of any size. While one
is doing development, it is handy to run only one mpd, on the local
machine. Then all the MPI processes will run locally as well.

The first modification to this default behavior is the -1 option to
mpiexec (not a great argument name). If -1 is specified, as in

```
mpiexec -1 -n 10 a.out
```

then the first application process will be started by the first mpd in
the ring after the local host. (If there is only one mpd in the ring,
then this will be on the local host.) This option is for use when a
cluster of compute nodes has a head node where commands like
mpiexec are run but not application processes.

If an mpd is started with the --ncpus option, then when it is its turn
to start a process, it will start several application processes rather
than just one before handing off the task of starting more processes to
the next mpd in the ring. For example, if the mpd is started with

```
mpd --ncpus=4
```

then it will start as many as four application processes, with
consecutive ranks, when it is its turn to start processes. This option
is for use in clusters of SMP's, when the user would like consecutive
ranks to appear on the same machine. (In the default case, the same
number of processes might well run on the machine, but their ranks would
be different.)

(A feature of the --ncpus=\[n\] argument is that it has the above effect
only until all of the mpd's have started n processes at a time once;
afterwards each mpd starts one process at a time. This is in order to
balance the number of processes per machine to the extent possible.)

Other ways to control the placement of processes are by direct use of
arguments to mpiexec. See the Users Guide here:
<http://www.mcs.anl.gov/research/projects/mpich2/documentation/index.php?s=docs>

#### Q: My output does not appear until the program exits.

A: Output to stdout may not be written from your process immediately
after a printf or fprintf (or PRINT in Fortran) because, under Unix,
such output is buffered unless the program believes that the output is
to a terminal. When the program is run by mpiexec, the C standard I/O
library (and normally the Fortran runtime library) will buffer the
output. For C programmers, you can either use a call fflush(stdout) to
force the output to be written or you can set no buffering by calling:

```
#include <stdio.h>
setvbuf( stdout, NULL, _IONBF, 0 );
```

on each file descriptor (stdout in this example) which you want to send
the output immediately to your terminal or file.

There is no standard way to either change the buffering mode or to flush
the output in Fortran. However, many Fortrans include an extension to
provide this function. For example, in g77,

```
call flush()
```

can be used. The xlf compiler supports

```
call flush_(6)
```

where the argument is the Fortran logical unit number (here 6, which is
often the unit number associated with PRINT). With the G95 Fortran 95
compiler, set the environment variable G95_UNBUFFERED_6 to cause
output to unit 6 to be unbuffered.

In C stderr is not buffered. So, alternatively, you can write your
output to stderr instead.

#### Q: Fortran programs using stdio fail when using g95.

A: By default, g95 does not flush output to stdout. This also appears to
cause problems for standard input. If you are using the Fortran logical
units 5 and 6 (or the \* unit) for standard input and output, set the
environment variable G95_UNBUFFERED_6 to yes.

#### Q: How do I run MPI programs in the background?

A: With the default Hydra process manager, you can just do:

```
mpiexec -n 4 ./a.out &
```

When using MPD, you need to redirect stdin from /dev/null. For example:

```
mpiexec -n 4 a.out < /dev/null &
```

#### Q: How do I use MPICH with slurm?

A: MPICH's default process manager, Hydra, internally detects and
functions correctly with SLURM. No special configuration is needed.

However, if you want to use SLURM's launchers (such as srun), you will
need to configure MPICH with:

```
./configure --with-pmi=slurm --with-pm=no
```

In addition, if your slurm installation is not in the default location,
you will need to pass the actual installation location using:

```
./configure --with-pmi=slurm --with-pm=no --with-slurm=[path_to_slurm_install]
```

#### Q: All my processes get rank 0.

A: This problem occurs when there is a mismatch between the process
manager (PM) used and the process management interface (PMI) with which
the MPI application is compiled.

MPI applications use process managers to launch them as well as get
information such as their rank, the size of the job, etc. MPICH
specified an interface called the process management interface (PMI)
that is a set of functions that MPICH internals (or the internals of
other parallel programming models) can use to get such information from
the process manager. However, this specification did not include a wire
protocol, i.e., how the client-side part of the PMI would talk to the
process manager. Thus, many groups implemented their own PMI library in
ways that were not compatible with each other with respect to the wire
protocol (the interface is still common and as specified). Some examples
of PMI library implementations are: (a) simple PMI (MPICH's default PMI
library), (b) smpd PMI (for linux/windows compatibility; will be
deprecated soon) and (c) slurm PMI (implemented by the slurm guys).

MPD, Gforker, Remshell, Hydra, OSC mpiexec, OSU mpirun and probably many
other process managers use the simple PMI wire protocol. So, as long as
the MPI application is linked with the simple PMI library, you can use
any of these process managers interchangeably. Simple PMI library is
what you are linked to by default when you build MPICH using the default
options.

srun uses slurm PMI. When you configure MPICH using --with-pmi=slurm, it
links with the slurm PMI library. Only srun is compatible with this
slurm PMI library, so only that can be used. The slurm folks came out
with their own "mpiexec" executable, which essentially wraps around
srun, so that uses the slurm PMI as well.

So, in some sense, mpiexec or srun is just a user interface for you to
talk in the appropriate PMI wire protocol. If you have a mismatch, the
MPI process will not be able to detect their rank, the job size, etc.,
so all processes think they are rank 0.

#### Q: How do I control which ports MPICH uses?

A: The MPIR_CVAR_CH3_PORT_RANGE environment variable allows you to
specify the range of TCP ports to be used by the process manager and the
MPICH library. Set this variable before starting your application with
mpiexec. The format of this variable is <low>:<high>. For example, to
allow the job launcher and MPICH to use ports only between 10000 and
10100, if you're using the bash shell, you would use:

```
export MPIR_CVAR_CH3_PORT_RANGE=10000:10100
```

#### Q: Why does my MPI program run much slower when I use more processes?

A: The default channel in MPICH (starting with the 1.1 series) is
`ch3:nemesis`. This channel uses busy polling in order to improve
intranode shared-memory communication performance. The downside to this
is that performance will generally take a dramatic hit if you
oversubscribe your nodes. Oversusbscription is the case where you run
more processes on a node than there are cores on the node. In this
scenario, you have a few choices:

1.  Just don't run with more processes than you have cores available.
    This may not be an option depending on what you are trying to
    accomplish.
2.  Run your code on more nodes or on the same number of nodes but with
    larger per-node core counts. That is, your job size should not
    exceed the total core count for the system on which you are running
    your job. Again, this may not be an option for you, since you might
    not have access to additional computers.
3.  Configure your MPICH installation with `--with-device=ch3:sock`.
    This will use the older `ch3:sock` channel that does not busy poll.
    This channel will be slower for intra-node communication, but it
    will perform much better in the oversubscription scenario.

#### Q: My MPI program aborts with an error saying it cannot communicate with other processes

A: Such failures occur with the following type of output:

```
Process 0 of 4 is on node02
Process 2 of 4 is on node02
Fatal error in PMPI_Bcast: Other MPI error, error stack:
PMPI_Bcast(1306)......................: MPI_Bcast(buf=0x7fff8a02ebb8, count=1, MPI_INT, root=0, MPI_COMM_WORLD) failed
MPIR_Bcast_impl(1150).................:
MPIR_Bcast_intra(990).................:
MPIR_Bcast_scatter_ring_allgather(840):
MPIR_Bcast_binomial(187)..............:
MPIC_Send(66).........................:
MPIC_Wait(528)........................:
MPIDI_CH3I_Progress(335)..............:
MPID_nem_mpich_blocking_recv(906).....:
MPID_nem_tcp_connpoll(1830)...........: Communication error with rank 1:
Process 1 of 4 is on node01
Process 3 of 4 is on node01
APPLICATION TERMINATED WITH THE EXIT STRING: Hangup (signal 1)
```

There are several possible reasons for this, all of them related to your
networking setup. Try out the following:

- Can you ssh from "node01" to "node02"?
- Can you ssh from "node02" to "node01"?
- Make sure the firewalls are turned off on **all** machines. If you
  don't want to control the firewalls, see above on how you can open a
  few ports in the firewall, and ask MPICH to use those ports. To
  check if firewall is enabled on any node, you can do:

On Ubuntu/Debian:

```
sudo ufw status
```

On Fedora/Redhat:

```
sudo iptables status
```

- Is your /etc/hosts file consistent across all nodes? Unless you are
  using an external DNS server, the /etc/hosts file on every machine
  should contain the correct IP information about all hosts in the
  system.

#### Q: How do I use MPICH in Amazon EC2?

[Using MPICH in Amazon EC2](../how_to/Using_MPICH_In_Amazon_EC2.md)

## Debugging MPI Programs

#### Q: How do I use Totalview with MPICH?

A: Totalview allows multiple levels of debugging for MPI programs. If
you need to debug your application without any information from the
MPICH stack, you just need to compile your program with mpicc -g (or
mpif77 -g, etc) and run your application as:

```
totalview mpiexec -a -f machinefile ./foo
```

The `-a` is a totalview specific option that is not interpreted by
mpiexec.

Totalview also allows you to peep into the internals of the MPICH stack
to query information that might sometimes be helpful for debugging. To
allow MPICH to expose such information, you need to configure MPICH as:

```
./configure --enable-debuginfo
```

#### Q: Can I use "ddd" or "gdb" to debug my MPI application?

A: Yes. But "ddd" and "gdb" are not parallel debuggers like Totalview.
They are serial debuggers. So, you will need to launch a separate ddd or
gdb window for each MPI process.

For ddd, you can use:

```
mpiexec -np 4 ddd ./foo
```

For gdb, you can use:

```
mpiexec -np 4 xterm -e gdb ./foo
```

## Troubleshooting

#### Q: Why am I getting so many unexpected messages?

A: If a process is receiving too many unexpected messages,your
application may fail with a message similar to this:

```
Failed to allocate memory for an unexpected message. 261894 unexpected messages queued.
```

Receiving too many unexpected messages is a common bug people hit. This
is often caused by some processes "running ahead" of others.

First, what is an unexpected message? An unexpected message is a message
which has been received by the MPI library for which a receive hasn't
been posted (i.e., the program has not called a receive function like
`MPI_Recv` or `MPI_Irecv`). What happens is that for small messages
("small" being determined by the particular MPI library and/or
interconnect you're using) when a process receives an unexpected
message, the library stores a copy of the message internally. When the
application posts a receive matching the unexpected message, the data is
copied out of the internal buffer and the internal buffer is freed. The
problem occurs when a process has to store too many unexpected messages:
eventually it will run out of memory. This problem may happen even if
the program has a matching receive for every message being sent to it.

Consider a program where one process receives a message then does some
computation on it repeatedly in a loop, and another process sends
messages to the other process, also in a loop. Because the first process
spends some time processing each message it'll probably run slower than
second, so the second process will end up sending messages faster than
the first process can receive them.

The receiving process will end up with all of these unexpected messages
because it hasn't been able to post the receives fast enough. Note that
this can happen even if you're using blocking sends: Remember that
`MPI_Send` returns when the send buffer is free to be reused, and not
necessarily when the receiver has received the message.

Another place where you can run into this problem of unexpected messages
is with collectives. People often think of collective operations as
synchronizing operations. This is not always the case. Consider reduce.
A reduction operation is typically performed in a tree fashion, where
each process receives messages from it's children performs the operation
and sends the result to it's parent. If a process which happens to be a
leaf of the tree calls `MPI_Reduce` before it's parent process does, it
will result in an unexpected message at the parent. Note also that the
leaf process may return from `MPI_Reduce` before it's parent even calls
`MPI_Reduce`. Now look what happens if you have a loop with `MPI_Reduce`
in it. Because the non-leaf nodes have to receive messages from several
children perform a calculation and send the result, they will run slower
than the leaf nodes which only have to send a single message, so you may
end up with a "unexpected message storm" similar to the one above.

So how can you fix the problem? See if you can rearrange your code to
get rid of loops like the ones described above. Otherwise, you'll
probably have to introduce some synchronization between the processes
which may affect performance. For loops with collectives, you can add an
`MPI_Barrier` in the loop. For loops with sends/receives you can use
synchronous sends (`MPI_Ssend`, and friends) or have the sender wait for
an explicit ack message from the receiver. Of course you could optimize
this where you're not doing the synchronization in every iteration of
the loop, e.g., call `MPI_Barrier` every 100th iteration.

#### Q: My MPD ring won't start, what's wrong?

A: MPD is a temperamental piece of software and can fail to work
correctly for a variety of reasons. Most commonly, however, networking
configuration problems are the root cause. In general, we recommend
[Using the Hydra Process Manager](../how_to/Using_the_Hydra_Process_Manager.md)
instead of MPD. Hydra is the default process manager starting release 1.3. If you need
to continue using MPD for some reason, here are some suggestions to try out.

If you see error messages that look like any of the following, please
try the troubleshooting steps listed in Appendix A of the 
[MPICH Installer's Guide](http://www.mcs.anl.gov/research/projects/mpich2/documentation/files/mpich2-1.2.1-installguide.pdf):

```
mpdboot_hostname (handle_mpd_output 374): failed to ping mpd on node1; recvd output={}

[1]+  Done                    mpd
```

```
Case 1: start mpd on n1, OK.
Case 2: start “mpd --trace" on n2, stuck at:
            n2_60857: ENTER mpd_sockpair in
/home/sl/mpi/bin/mpdlib.py at line 213; ARGS=()
Case 3: start "mpdboot -n 2 -f mpd.hosts" on n1, OK. mpdtrace on each
node shows "n1, n2"
           mpdringtest OK.
           mpiexec -n 2 date   // Fail, mpiexec_n1 (mpiexec 392): no msg recvd from mpd when expecting ack of request

Case 4: start "mpdboot -n 2 -f mpd.hosts" on n2, failed with nothing output.
```

In particular, make sure to try the steps that involve the `mpdcheck`
utility.

If you are using MPD on CentOS Linux and `mpdboot` hangs and then
elicits the following messages upon CTRL-C:

```
^[[CTraceback (most recent call last):
 File "/nfs/home/atchley/projects/mpich2-mx-1.2.1..6/build/shower/bin/mpdboot", line 476, in ?
   mpdboot()
 File "/nfs/home/atchley/projects/mpich2-mx-1.2.1..6/build/shower/bin/mpdboot", line 347, in mpdboot
   handle_mpd_output(fd,fd2idx,hostsAndInfo)
 File "/nfs/home/atchley/projects/mpich2-mx-1.2.1..6/build/shower/bin/mpdboot", line 385, in handle_mpd_output
   for line in fd.readlines():    # handle output from shells that echo stuff
KeyboardInterrupt
```

then you should see . This is a known issue that we have yet to resolve.

#### Q: Why MPI_Put raises SIGBUS error inside docker?

A: Short answer: this is due to docker by default limiting shared memory
size to 64MB. You can increase that limit by using the `--shm-size`
option for `docker run` or `docker build`.

Explanation: when multiple MPI processes are on a same node, mpich uses
shared memory as often as possible for performance reasons. This
includes `MPI_Win_allocate`, which will allocate using shared memory so
that most of the one-sided operations can be reduced to as simple as
`memcpy`. On linux, typically share memory are not limited -- only the
limit of physical memory itself. However, docker by default put a much
smaller limit, for obvious reasons. When the shared memory is limited to
64MB and when its usages exceed that limit, while the memory space is
still valid, kernel will refuse to page in the actual memory, resulting
in a SIGBUS error.

The work-around depends on your applications. Increase the limit is an
obvious one. The newer versions of docker has a `--shm-size` option to
set an alternate limit. For example, `docker run -it --shm-size=512m
image cmd` will set the limit to 512 MB.

If you are running the application inside docker just for testing
purpose, i.e. performance is not critical for you, then you may also
consider to disable shared memory usage in MPICH altogether. This can be
achieved by telling MPICH to treat each MPI process as if running on
different nodes. You can achieve this via environment variable
`MPIR_CVAR_NOLOCAL=1` (for older version of MPICH, e.g. v3.2 and
earlier, use `MPIR_CVAR_CH3_NOLOCAL=1`). When "NOLOCAL" option is set,
all communication will go through the network stack.
