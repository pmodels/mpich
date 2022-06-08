# Testing MPICH2

## Using The MPI Test Suites With MPICH2

Basic tests can be performed using the MPICH2 tests

```
make testing
```

The results are in `test/summary.xml`. The XML style file for these
results is in `test/TestResults.xsl`.

To run the full set of tests and add the results to the [results web
page](http://www.mcs.anl.gov/research/projects/mpich2/nightly/old/index.htm%7Ctest),
simply run `basictests` . This will not update the web page however. Run
`updatesum` to update the test results web page.

The tests are, at this writing, in
`/mcs/web/research/projects/mpich2/nightly/cron/old` . The master
versions of these files are stored in an svn repository; changes should
be made by updating the svn copies. On the rest of this page, where a
command script such as `basictests` is used, a full path should be used
instead (since that path has changed in the past and may change in the
future, we don't show a potentially obsolete and incorrect path).

To customize the tests run by `basictests`, the following command-line
options may be used:

  - `-tables=pm-device`:

Selects the combinations of process managers and ADI implementations to
test. The default is

    hydra-ch3:nemesis mpd-ch3:nemesis hydra-ch3:sock smpd-ch3:nemesis gforker-ch3:nemesis random

  - `-cc=name, -cxx=name, -fc=name, -f90=name`:

Specifies different compilers for C, C++, Fortran 77, and Fortran 90

  - `-arch=name`:

Sets the "architecture name". This option should be used to ensure that
the results are given a unique output name. For example

```
-arch=Linuxgcc31
```

for IA32 Linux using gcc version 3.1 . The file name for the results is
made up of the arch, process manager, device, and date. This option is
particularly important because the columns of the web page are
determined by this option.

  - `-soft=name`:

Add the "soft" option. For example, to use the PGI compilers at MCS, you
need to execute the command

```
soft add +pgi
```

This option causes `basictests` to perform this step

  - `-mpich2dir=path`:

Sets an alternate location for the source of MPICH2 (the default is
`/home/MPI/testing/mpich2/mpich2`).

  - `-datadir=path`:

Sets an alternate location for the output files. The default is
`/mcs/web/research/projects/mpich2/nightly/old/runs`.

  - `-tests=list`:

Selects the tests to run. The default is to run all four test sets and
is equivalent to specifying `-tests=mpich:mpicxx:intel:mpich2`.

For example, to run the tests on your own copy of MPICH2, storing the
data in a private directory, for just the mpd process manager and ch3
device, use

```
basictests -mpich2dir=/home/me/mympich2 \
                             -datadir=/home/me/myresults \
                             -tables=mpd
```

To do: add information on using all five test suites, including the MPI
I/O test suite.

### Example Uses Of `basictests`

  - Basic use:

Run the tests and let `basictests` choose the compilers and the test
name (used to label a column on the web page)

```
basictests
```

  - Alternate Compilers:

This example shows how to select the Portland Group compilers. Note the
use of the `-soft` option to ensure that the correct paths are set up
for using Portland Group compilers and the use of the `-arch` option to
give this test a unique name

```
basictests -soft=pgi-5.2 -cc=pgcc -fc=pgf77 -cxx=pgCC -f90=pgf90 -arch=IA32-Linux-PG
```

## Changing The Default Tests

The tests that are run by default are chosen in the script
`basictests`. To add a new architecture to the tests, simply run
`basictests` on a new platform; the results will be automatically added
to the web page
[1](http://www.mcs.anl.gov/research/projects/mpich2/nightly/old/).

To add a new device and/or process manager, you need to edit two files.
In `basictests`, find the line

```
tables="mpd-ch3 mpd-ch3:ssm mpd-ch3:shm mpd-ch3:sshm gforker-ch3 random"
```

Add the process manager and device pair that you want to test, following
the form `processmanager-device`. For a device that has additional
options, add these with a colon after the device name. For example, to
add testing with the `gforker` process manager for the shared memory
version of the ch3 device, add `gforker-ch3:shm` to `tables`:

```
tables="mpd-ch3 mpd-ch3:ssm mpd-ch3:shm mpd-ch3:sshm gforker-ch3 gforker-ch3:shm random"
```

You must also update `updatesum` to add the new test to the web page.
Add the same entry to the `@tables` array in `updatesum`. As this is a
perl script, the syntax is slightly different. Continuing with the
example above, we add `"gforker-ch3:shm"` to `@tables`:

```
@tables = ( "mpd-ch3",
            "mpd-ch3:ssm", "mpd-ch3:shm", "mpd-ch3:sshm",
            "gforker-ch3",
            "gforker-ch3:shm",
            "random" );
```

That's it. The next time `basictests` runs, the new process manager and
device pair will be added to the tests, and the web page will reflect
the new tests.

## Location Of Test Output

The output of the tests run by `basictests` is copied into the directory
`/mcs/web/research/projects/mpich2/nightly/old/runs` in a file with a
name that contains the architecture, process manager, device, date, and
test name. For example, the files for IA32 under Linux, with the GNU
compilers, using the MPD process manager and the ch3 device, and started
on April 1st, 2005 at 2200 hours, are

```
IA32-Linux-GNU-mpd-ch3-2005-04-01-22-00.xml
IA32-Linux-GNU-mpd-ch3-make-2005-04-01-22-00.htm
IA32-Linux-GNU-mpd-ch3-2005-04-01-22-00-testsumm.xml
IA32-Linux-GNU-mpd-ch3-2005-04-01-22-00-testsumm-fail.xml
IA32-Linux-GNU-mpd-ch3-2005-04-01-22-00-testsumm-cxx.xml
IA32-Linux-GNU-mpd-ch3-2005-04-01-22-00-testsumm-cxx-fail.xml
IA32-Linux-GNU-mpd-ch3-2005-04-01-22-00-testsumm-intel.xml
IA32-Linux-GNU-mpd-ch3-2005-04-01-22-00-testsumm-intel-fail.xml
IA32-Linux-GNU-mpd-ch3-2005-04-01-22-00-testsumm-mpich2.xml
IA32-Linux-GNU-mpd-ch3-2005-04-01-22-00-testsumm-mpich2-fail.xml
```

The first of these is the entire output file. The second contains only
the output of make that was might contain warnings or errors about the
make. The remaining eight files contain the test results and the
failure-only results (the files with the `-fail` extension).

If more details are required, you will need to check the directory in
which the tests were run. By default (see `basictests`), this is in
`/sandbox/USER/cb`, where `USER` is the name of the person that ran the
tests.

## Running The Special Tests

The [special tests](http://www.mcs.anl.gov/research/projects/mpich2/todo/specialtests)
are run with the script `specialtest -dir=rundir`, where `rundir` is the
directory in which `specialtest` writes the log files. The `-help`
option to `specialtest` describes the options that can be used to run a
subset of tests or to select different combinations of process managers
or devices.

For example, to run the tests on your own copy of MPICH2 and place the
results in a private location, use

```
/mcs/mpich/cron/tests/specialtest -srcdir=/home/me/mympich2 \
                              -dir=/sandbox/me \
                              -webdir=/home/me/mytests
```

The results can then be viewed by opening `/home/me/mytests/index.htm`
in a web browser.

To update the special tests output on the general web page, simply run

```
specialtest
```

This will use the MPICH2 source in `/home/MPI/testing/mpich2/mpich2`.

Always run this test in a separate directory; do **not** run it in
`/home/MPI/nightly`.

## Running The Configure Tests

The [configure tests](http://www.mcs.anl.gov/mpi/mpich/todo/testoptions.htm) are run
with the script `testoptions`.

To update the special tests output on the general web page, simply run

```
testoptions
```

This will use the MPICH2 source in `/home/MPI/testing/mpich2/mpich2`.

## Running The Coding Checks

To run the coding checks, change to a directory that contains MPICH2 and
execute the following command:

```
/home/gropp/projects/software/buildsys/src/codingcheck -conf=mpich2 \
    -skipfiles=src/mpid/globus,src/mpe2,src/mpid/rdma \
    -checktest src examples test
```

(This assumes that you are running on one of the MCS division Linux
systems; you can install the buildsys tools and using `codingcheck` from
your own installation if you need to run the coding check on a different
system). The report of problems detected is sent to standard output. The
above command will produce a relatively comprehensive report. See the
use of the `codingcheck` command in `updatetodo` for the current command
use to update the web page; this command may skip additional directories
and may use additional options (such as `-rmchecks=cppdefines`) to tune
the output.

An updated report is generated every morning as one of the steps
performed by `updatetodo` and placed into
`/mcs/web/research/projects/mpich2/todo/coding-problems.txt` (accessible
as [2](http://www.mcs.anl.gov/mpi/mpich/todo/coding-problems.txt)).

An explanation of some of the rules that are applied by the style
checker are in the [Coding Standards](Coding_Standards "wikilink").

### Finding Specific Classes Of Problems

Currently, the coding checker produces a unified report about all
problems. To find specific problems, you can use `grep` or view the
report (it is a text file) in your favorite editor. Some simple examples
are:

  - Find errors:

These are uses that are viewed as erroneous. In many cases, these are
uses of either MPI or PMPI routines where NMPI names must be used (NMPI
routines ensure that errors are properly handled).

```
grep 'Error:' /mcs/web/research/projects/mpich2/todo/coding-problems.txt
```

  - Find non-conforming ifdef names:

These find C preprocessor names that may conflict with other system
header files

```
grep Warning: /mcs/web/research/projects/mpich2/todo/coding-problems.txt | egrep 'ifn?def'
```

  - Find missing style headers:

Each file should have a style header to avoid unnecessary reformatting
when the file is edited (e.g., to maintain a consistent level of
indentation).

```
grep 'style header missing' /mcs/web/research/projects/mpich2/todo/coding-problems.txt
```

  - Find suspicious function usage:

There are a number of functions, such as `strcat`, that should never
appear in good code because of the potential for memory overwrites (even
if the code is surrounded by length checks, any reader of the code must
take the time to ensure that those checks are correct; it is better to
avoid them). Other routines, such as `malloc`, interfere with debugging
tools that are built into the code. Yet other functions, such as
`setvbuf` or `snprintf`, are either not portable to all operating
systems or are only part of C99 or later, and may pose portability
problems for users with older (e.g., C90) compilers.

```
grep 'Warning: found ' /mcs/web/research/projects/mpich2/todo/coding-problems.txt
grep 'Caution: found ' /mcs/web/research/projects/mpich2/todo/coding-problems.txt
```

  - Find missing Copyright statements:

To get a quick list of files that are missing a copyright statement:

```
grep 'Copyright statement missing' /mcs/web/research/projects/mpich2/todo/coding-problems.txt
```

  - Find files that use a particular function:

For example, to find all files that use `malloc`

```
grep malloc /mcs/web/research/projects/mpich2/todo/coding-problems.txt | grep -v Warning:
```

## Running The Global Symbols Checks

To update the web page that reports on the global symbols, run

```
checkglobs
```

Like the other tests, this works with the source files in
`/home/MPI/testing/mpich2/mpich2`. The program itself is a simple perl
script; if you wish to change the tests for yourself, make a copy and
update the values of the following variables, found near the top of the
file:

  - `mpich2src`:

Location of MPICH2 source

  - `builddir`:

Directory to be used to build MPICH2 as part of identifying global
symbols

  - `reportdir`:

Direction into which the result web page is placed

  - `checksym`:

Location of program that is used to check for global symbols, along with
any command-line arguments

  - `includeLogFiles`:

Set to `1` if the configure and make log files should be copied to
`$reportdir`

There are several easy ways to get a quick look at the global symbols.
The easiest is to use the `nm` program on `libmpich.a`. However, you
will need to filter the output to find any non-conforming names. For
example, under Linux,

```
nm libmpich.a | grep " C " | grep -v mpipriv
```

will show the names of any uninitialized global variables.

Finding non-conforming names is harder, primarily because the list of
allowed prefixes is fairly large. Instead of trying to list all prefixes
and using `grep -v`, you can use

```
/homes/gropp/projects/software/buildsys/src/checkforglobs -mpich2 libmpich.a
```

When testing for global symbols, make sure that you include tests with
weak symbols disabled to ensure that any internal routines that are
static only when weak symbols are supported have conforming names.

## Running And Understanding The Coverage Tests

The easiest way to run the coverage tests is with the script
`getcoverage`. This uses the version of MPICH2 that is updated every
night in `/home/MPI/testing/mpich2/mpich2`, and uses the MPICH2 test
suite and the three additional test suites that are in
`/home/MPI/testing/tsuites`. Run this script with

```
getcoverage -updateweb
```

The best way to view the coverage data for the entire project is through
the <a
href="http://www-unix.mcs.anl.gov/mpi/mpich2/todo/coverage/index.htm">web
page</a>. Code with a blue background has been ignored in counting
covered and uncovered lines; this is typically error handling and
reporting code or experimental code that is not expected to be executed.
Code with a red background is code that should have been executed by the
tests but was not (in some cases, this code was not marked as error
handling or reporting code; such code should be fixed by annotating the
source code to mark the code as error handling).

To update the coverage information for a single file, the easiest
approach is to use `getcoverage` to create the correct versions of the
libraries and get the initial coverage results. To add to the results,
simply compile a program with the coverage-enabled libraries (by
default, this is in `/sandbox/$LOGNAME/mpich2-cov`) and run it. This
will cause the coverage data files (files with extensions `da`) to be
updated. To generate a text file describing the coverage, you can
normally use

```
gcov foo.c
```

in the directory that contains the file `foo.da` (you'll also see the
files `foo.bb` and `foo.bbg`). This will produce a file `foo.c.gcov`.
Lines that are marked with `#####` have not be executed (see `man`
`gcov` for more information on the `gcov` tool).

When running the `getcoverage` tool from a cron job, the following
options are useful:

  - `-quiet` - Suppresses all output
  - `-logfile=name` - Sends all output to the named file.

If `-updateweb` is also selected, then the `-logfile` option also copies
the contents of the logfile into `logfile.txt` in the designated web
directory.

## Troubleshooting The Tests

### Stopping The Tests

Several of the test suites allow you to stop them by creating a file.
When the test suite detects the existence of the file, the tests are
aborted. A good way to create these files is with `date` so that it is
easy to see when the file was originally created.

  - mpich:

`$HOME/.stopmpichtests`

  - mpich2:

`.stoptest` in the top-level testing directory (e.g., `mpich2/test/mpi`

## Sources For The Test Suites

The test suites should be accessible through the
<a href="http://www.mcs.anl.gov/mpi/mpi-test/tsuite.html">test suite web
page</a>. Local to Argonne, the test suites are available from the CVS
repository:

<table>

<tr>

<th>

Test Suite

</th>

<th>

CVS Repository

</th>

<th>

CVS Module

</th>

</tr>

<tr>

<td>

Intel

</td>

<td>

/home/MPI/cvsMaster

</td>

<td>

IntelMPITEST

</td>

</tr>

<tr>

<td>

C++

</td>

<td>

/home/MPI/cvsMaster

</td>

<td>

mpicxxtest

</td>

</tr>

<tr>

<td>

LLNL I/O

</td>

<td>

/home/MPI/cvsMaster

</td>

<td>

Testmpio

</td>

</tr>

<tr>

<td>

MPICH

</td>

<td>

/home/MPI/cvsMaster

</td>

<td>

mpich/examples/test

</td>

</tr>

<tr>

<td>

MPICH2

</td>

<td>

/home/MPI/cvsMaster

</td>

<td>

mpich2-01/test/mpi

</td>

</tr>

</table>

## Running The LLNL I/O Test

To run the LLNL I/O test, do the following:

1.  Get a current version. For example,


```
cvs -d /home/MPI/cvsMaster export -D now Testmpio
```

1.  Update the `Makefile`. For MPICH2, all you should need to

do is to reset the value of the variable `MPIHOME`.

1.  Make a `t1` subdirectory in the test directory:


```
mkdir t1
```

Without this step, the tests will fail with "invalid file name"
messages.

1.  Run the test with `make testing`. If you have trouble with

this (for example, it fails if you direct the output into a file), run
the test manually as follows (assuming a C-shell):

```
setenv MPIO_USER_PATH `pwd`/t1
mpiexec -n 4 testmpio 1 |& tee test.log
```

1.  Interpret the results. This test does not provide a pass/fail
    summary,

so you will need to examine the output for problems.
