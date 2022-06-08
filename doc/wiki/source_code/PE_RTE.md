# Parallel Environment Runtime Edition (PE RTE) 
Below are instructions for configuring, building, and testing.

## MPICH Test Suite Instructions

From a mpich testsuite build directory, such as
*/home/johndoe/testsuite*, invoke the configure script in the *test/mpi*
directory of the mpich source.

```
/home/johndoe/mpich/test/mpi/configure \
    --srcdir=/home/johndoe/mpich/test/mpi \
    --with-mpi=/opt/ibmhpc/pecurrent/base
```

The `--srcdir` configure option specifies the location of the testsuite
source and the `--with-mpi` configure option specifies which mpi
installation to use when compiling the tests.

Once configured, the tests can be compiled and executed using the `make
testing` makefile rule. Specific make variables need to be specified
depending on how the jobs are to be launched.

### POE

The host list file must be specified with the environment variable
`MP_HOSTFILE` before testing starts. For more information on the host
list file see section **"Set the MP_HOSTFILE environment variable"** in
the [Parallel Environment Runtime
Edition](http://publib.boulder.ibm.com/infocenter/clresctr/vxrx/index.jsp?topic=%2Fcom.ibm.cluster.pe.v1r3.pe100.doc%2Fam102_shnf.htm)
online documentation.

```
export MP_HOSTFILE=/home/johndoe/host.list
```

To begin testing, change to the directory where the configure command
was run (*/home/johndoe/testsuite* in this example) and invoke the
following command:

```
make testing
```
