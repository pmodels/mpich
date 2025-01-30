			Yaksa

Yaksa is a high-performance noncontiguous datatype engine that can be
used to express and manipulate noncontiguous data.  This release is an
experimental version of Yaksa that contains features related to
packing/unpacking, I/O vectors, and flattening noncontiguous datatypes.

This README file should contain enough information to get you started
with Yaksa.  More information about Yaksa can be found at
https://github.com/pmodels/yaksa.


1. Getting Started
2. Testing Yaksa
3. Reporting Problems
4. Alternate Configure Options
5. Compiler Flags
6. Developer Builds


-------------------------------------------------------------------------------

# Getting Started


The following instructions take you through a sequence of steps to get the
default configuration of Yaksa up and running.

1. You will need the following prerequisites.

    - REQUIRED: This tar file yaksa-<version>.tar.gz

    - REQUIRED: A C compiler (gcc is sufficient)

  Also, you need to know what shell you are using since different shell has
  different command syntax.  Command "echo $SHELL" prints out the current shell
  used by your terminal program.

2. Unpack the tar file and go to the top level directory:

    tar xzf yaksa-<version>.tar.gz
    cd yaksa-<version>

3. Choose an installation directory, say /home/USERNAME/yaksa-install,
which is assumed to be non-existent or empty.

4. Configure Yaksa specifying the installation directory:

      ./configure --prefix=/home/USERNAME/yaksa-install 2>&1 | tee c.txt

5. Build Yaksa:

      make 2>&1 | tee m.txt

  This step should succeed if there were no problems with the preceding step.
  Check file m.txt.  If there were problems, do a "make clean" and then run
  make again with V=1.

    make V=1 2>&1 | tee m.txt   (for bash and sh)

  Then go to step 3 below, for reporting the issue to the Yaksa developers
  and other users.

6. Install Yaksa:

      make install 2>&1 | tee mi.txt

  This step collects all required executables and scripts in the bin
  subdirectory of the directory specified by the prefix argument to configure.

-------------------------------------------------------------------------------

# Testing Yaksa

To test Yaksa, we package the Yaksa test suite in the Yaksa
distribution.  You can run the test suite in the test directory using:

     make testing

If you run into any problems on running the test suite, please follow
step 3 below for reporting them to the Yaksa developers and other
users.

-------------------------------------------------------------------------------

# Reporting Problems

If you have problems with the installation or usage of Yaksa, please follow
these steps:

1. First visit the Frequently Asked Questions (FAQ) page at
https://github.com/pmodels/yaksa/wiki/FAQ
to see if the problem you are facing has a simple solution.

2. If you cannot find an answer on the FAQ page, look through
previous issues filed (https://github.com/pmodels/yaksa/issues).  It
is likely someone else had a similar problem, which has already been
resolved before.

3. If neither of the above steps work, please send an email to
yaksa-users@lists.mcs.anl.gov.  You need to subscribe to this list
(https://lists.mcs.anl.gov/mailman/listinfo/yaksa-users) before
sending an email.

Your email should contain the following files.  ONCE AGAIN, PLEASE COMPRESS
BEFORE SENDING, AS THE FILES CAN BE LARGE.  Note that, depending on which step
the build failed, some of the files might not exist.

    yaksa-<version>/c.txt (generated in step 4 of ["Getting Started"])
    yaksa-<version>/m.txt (generated in step 5 of ["Getting Started"])
    yaksa-<version>/mi.txt (generated in step 6 of ["Getting Started"])
    yaksa-<version>/config.log (generated in step 4 of ["Getting Started"])

    DID WE MENTION? DO NOT FORGET TO COMPRESS THESE FILES!

Finally, please include the actual error you are seeing when running
the application.  If possible, please try to reproduce the error with
a smaller application or benchmark and send that along in your bug
report.


-------------------------------------------------------------------------------

# Alternate Configure Options

Yaksa has a number of other features.  If you are exploring Yaksa as part
of a development project, you might want to tweak the Yaksa build with the
following configure options.  A complete list of configuration options can be
found using:

    ./configure --help

-------------------------------------------------------------------------------

# Compiler Flags

By default, Yaksa automatically adds certain compiler optimizations to
CFLAGS.  The currently used optimization level is -O2.

This optimization level can be changed with the --enable-fast option passed to
configure.  For example, to build Yaksa with -O3, one can simply do:

    ./configure --enable-fast=O3

Or to disable all compiler optimizations, one can do:

    ./configure --disable-fast

For more details of --enable-fast, see the output of "./configure --help".

For performance testing, we recommend the following flags:

    ./configure --enable-fast=O3

-------------------------------------------------------------------------------

# Developer Builds

For Yaksa developers who want to directly work on the primary version control
system, there are a few additional steps involved (people using the release
tarballs do not have to follow these steps).  Details about these steps can be
found here: https://github.com/pmodels/yaksa/wiki/Getting-and-Building
