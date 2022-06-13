# MPICH From Github

To checkout a new copy of the MPICH source, use

```
git clone https://github.com/pmodels/mpich.git (for non-core developers without commit rights)
git clone git@github.com:pmodels/mpich.git (for core developers with commit rights)
```

See the [Version Control System](Version_Control_System.md) page for more information about using
our current version control system.

## Setting Up The Build Environment

Doing a git clone may not be sufficient to initialize necessary git
submodules. To retrieve submodules, run

` git submodule update --init`

The git repository does not contain any of the "derived" files,
including the configure scripts and the C++ and Fortran 77 language
bindings.

To build these, run

`./autogen.sh`

Occasionally changes are made to the autoconf macros that are not
detected by the dependency tests for the configure scripts. It is always
correct to delete all of the configure scripts before running
autogen.sh:

```
find . -name configure -print | xargs rm
./autogen.sh
```

The autoconf macros and the configure.in scripts now require the
following:

  - **autoconf version 2.67 (or higher)**
  - **automake version 1.12.3 (or higher)**
  - **GNU libtool version 2.4 (or higher)**

This was done because there are incompatible differences between each
minor release of autoconf (e.g. the allowed command line arguments has
changed between 2.50 and 2.58).

You can select a particular version of autoconf and autoheader by using
the environment variables AUTOCONF and AUTOHEADER respectively.
autogen.sh will use these if they are set. However, note that for these
tools to work properly, both they and all of their data files must be
installed in the same set of directories. The easiest way to ensure this
is to use *exactly* the same configure arguments when you configure and
install these tools. For example, if you set the **prefix**, set the
prefix to exactly the same path for all three tools.

## Building The Software

Once MPICH has been bootstrapped with `autogen.sh`, you can perform the
usual three step process to build it like any other unix package:

```
./configure --prefix=INSTALLATION_PREFIX
make -j8
make -j8 install
```

Substitute `INSTALLATION_PREFIX` above with a proper directory. 
Otherwise `/usr` will be assumed as a default.

## Updating Derived Files

**Note: most of the time the automake rebuild rules will handle this
correctly for you, but not always.**

If you change one of the files that is the source for a derived file,
such as a `configure.in` file, you will need to rebuild the derived file
(e.g., the corresponding configure file). The safest way to do this is
to rerun autogen.sh:

`./autogen.sh`

(from the top-level MPICH directory). However, this can take a fair
amount of time. You can direct autogen.sh to only update certain classes
of files. For example, to update all configure files, use

`./autogen.sh -do=build_configure`

You can use multiple `-do` arguments. For example, to rebuild the
Makefile.in files and the configure files, use

`./autogen.sh -do=build_configure -do=makefiles`

Check the source of autogen.sh to see what other options are available
for `-do`.
