# Configuring Downstream Projects With MPICH

This page is for projects that build against an installed MPICH: how a
build system finds MPICH and gets the compile and link flags for C, C++
and Fortran. MPICH provides two ways to discover them:

- the compiler wrappers `mpicc`, `mpicxx` and `mpifort`, and
- pkg-config files.

Both give the same flags. The wrappers are always installed and work
without any other tool. pkg-config is useful when the project wants to
use its own compiler, when the wrappers cannot be run (e.g. when
cross-compiling), or when the build system already uses pkg-config.
A section at the end describes what CMake's FindMPI module needs to do
to use the pkg-config files.

## What an MPICH installation provides

| Language | Wrapper | pkg-config module | Generic module (link) |
|---|---|---|---|
| C | `mpicc` | `mpich` | `mpi-c` |
| C++ | `mpicxx` | `mpich-cxx` | `mpi-cxx` |
| Fortran | `mpifort` (also `mpif77`, `mpif90`) | `mpich-fortran` | `mpi-fort` |

`mpich.pc` is a link to the file for the ABI that the default `mpicc`
uses (see [MPI ABI builds](#mpi-abi-builds)).

- `mpicxx` is installed when MPICH is configured with a C++ compiler,
  and `mpich-cxx.pc` is always installed. Without the C++ bindings
  (`--enable-cxx`) they link only `libmpi`, which is all a C++ program
  using the C API needs.
- `mpifort` and the Fortran modules are installed only when the Fortran
  binding is built.
- `mpichversion` prints the MPICH version, device, and configure
  options.
- The `.pc` files are in `$libdir/pkgconfig` by default. `mpich-cxx`,
  `mpich-fortran`, the per-ABI files and the `mpi-*` links are available
  in MPICH 5.1 and later; earlier versions only have `mpich.pc`.

## Using the compiler wrappers

### Building with the wrappers

The simplest approach is to use the wrappers as the compilers. For an
Autoconf project:

```
./configure CC=mpicc CXX=mpicxx FC=mpifort
```

For a project built with a plain Makefile:

```
make CC=mpicc
```

The wrapper adds the MPI include path when compiling, and the MPI
libraries when linking. Any option it does not recognize is passed to the
underlying compiler.

### Querying the flags

A build system that wants to use its own compiler can ask the wrapper
for the flags:

| Option | Prints | Available in |
|---|---|---|
| `-show` | The full command the wrapper would run, including the compiler. Add `-c` to get the compile command only. | all wrappers |
| `-compile-info`, `-link-info` (also `-compile_info`, `-link_info`) | Same as `-show`. Kept for compatibility. | all wrappers |
| `-show-compile-info` | Only the compile flags (preprocessor flags, compiler flags, `-I`), without the compiler. | `mpicc`, `mpicxx`; MPICH 4.0 and later |
| `-show-link-info` | Only the link flags and libraries, without the compiler. | `mpicc`, `mpicxx`; MPICH 4.0 and later |

```
$ mpicc -show-compile-info
-I/opt/mpich/include
$ mpicc -show-link-info
-L/opt/mpich/lib -Wl,-rpath -Wl,/opt/mpich/lib -Wl,--enable-new-dtags -lmpi
$ mpifort -show
gfortran -I/opt/mpich/include -L/opt/mpich/lib -lmpifort -Wl,-rpath -Wl,/opt/mpich/lib -Wl,--enable-new-dtags -lmpi
$ mpifort -show -c
gfortran -c -I/opt/mpich/include
```

When parsing `-show` output, drop the first word (the compiler). With
`-c`, the result is the compile flags; without `-c`, it also has the link
flags. Add `-show` to the end of a real command line to see exactly what
the wrapper would do with it.

Other wrapper options that affect the output:

- `-cc=<compiler>`, `-cxx=<compiler>`, `-fc=<compiler>`, or the
  environment variables `MPICH_CC`, `MPICH_CXX`, `MPICH_FC`: use another
  compiler. It must be compatible with the one MPICH was built with (see
  the [FAQ](../faq/Frequently_Asked_Questions.md)).
- `-static-mpi`: link `libmpi.a` instead of the shared library.
- `-help`: list the options.

### What the flags contain

- The include path, the library path, and `-lmpi` (`-lmpicxx` for the
  C++ bindings when they are built, `-lmpifort` for Fortran). For
  Fortran, the include path also finds the `mpi` and `mpi_f08` modules;
  if the modules are installed in a separate directory, it is added
  too.
- A runpath for `$libdir`, unless MPICH was configured with
  `--with-wrapper-dl-type=none`. `--with-wrapper-dl-type=rpath` uses rpath
  instead of runpath.
- The external libraries that `libmpi` depends on (hwloc, libfabric,
  UCX, ...) are added only when MPICH was built without shared libraries
  (or with `--disable-interlib-deps`). Otherwise the shared library
  already records them.
- The `CFLAGS` and `LDFLAGS` used to build MPICH are not included, with
  one exception: for a static-only build, the `LDFLAGS` are added so the
  external libraries can be found. To add flags to the wrappers, set
  `MPICH_MPICC_CFLAGS`, `MPICH_MPICC_LDFLAGS`, `MPICH_MPICC_LIBS` (and
  the `MPICH_MPICXX_*` and `MPICH_MPIFORT_*` equivalents) when running
  configure.

## Using pkg-config

### Finding the files

pkg-config searches its default directories and those in
`PKG_CONFIG_PATH`. For an MPICH installed in its own prefix:

```
export PKG_CONFIG_PATH=/opt/mpich/lib/pkgconfig:$PKG_CONFIG_PATH
```

### Querying

```
$ pkg-config --cflags mpich
-I/opt/mpich/include
$ pkg-config --libs mpich
-Wl,-rpath -Wl,/opt/mpich/lib -Wl,--enable-new-dtags -L/opt/mpich/lib -lmpi
$ pkg-config --cflags --libs mpich-fortran
-I/opt/mpich/include -Wl,-rpath -Wl,/opt/mpich/lib -Wl,--enable-new-dtags -L/opt/mpich/lib -lmpifort -lmpi
$ pkg-config --static --libs mpich
-Wl,-rpath -Wl,/opt/mpich/lib -Wl,--enable-new-dtags -L/opt/mpich/lib -lmpi -L/opt/hwloc/lib -lpthread -lhwloc
$ pkg-config --modversion mpich
5.1.0
$ pkg-config --variable=libdir mpich
/opt/mpich/lib
```

- `Cflags` and `Libs` match what `mpicc -show-compile-info` and
  `mpicc -show-link-info` print for a shared build.
- `--static` adds the libraries in `Libs.private` of `mpich` and
  `mpich-cxx`: the external libraries `libmpi` depends on, and the `-L`
  paths of those given to configure with `--with-<package>=PATH`. Use it
  when linking `libmpi.a`. `mpich-fortran` gets them through
  `Requires.private: mpi-c`, the C MPI library the Fortran binding is
  built on.
- The variables `prefix`, `exec_prefix`, `libdir` and `includedir` are
  defined in every file. `mpich-fortran.pc` also defines `modincdir`, the
  directory of the Fortran modules.
- Use the `mpich*` names to require MPICH specifically, and the `mpi-*`
  names to accept any MPI implementation that provides them. A
  distribution may install the `mpi-*` names only for its default MPI, or
  not at all (see [For packagers](#for-packagers)).

### MPI ABI builds

MPICH can build its native ABI (`libmpi`), the MPI Standard ABI
(`libmpi_abi`), or both. There is one C module for each ABI that is
built, and `mpich` points to the one the default `mpicc` uses:

| Configure option | Installed | `mpich.pc` links to | Default `mpicc` |
|---|---|---|---|
| (default) | `mpich-mpich-abi.pc` | `mpich-mpich-abi.pc` | `-lmpi` |
| `--enable-mpi-abi` | `mpich-mpi-abi.pc` | `mpich-mpi-abi.pc` | `-DMPI_ABI -lmpi_abi` |
| `--enable-mpi-abi=dual` | both | `mpich-mpich-abi.pc` | `-lmpi` (`mpicc_abi`: `-DMPI_ABI -lmpi_abi`) |

`mpich-mpi-abi.pc` gives `-DMPI_ABI` and `-lmpi_abi`, the same as
`mpicc_abi`. Use `mpich-mpi-abi` to build for the MPI Standard ABI, and
`mpich` (or `mpi-c`) to follow the installation's default.
`mpich-cxx` and `mpich-fortran` use the default ABI, like `mpicxx` and
`mpifort`.

### In build systems

Autotools (`pkg.m4`):

```
PKG_CHECK_MODULES([MPI], [mpich >= 4.0])
PKG_CHECK_MODULES([MPI_FORT], [mpich-fortran])
```

Meson:

```
mpi_dep = dependency('mpich')
mpi_fort_dep = dependency('mpich-fortran')
```

CMake, without FindMPI:

```
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPICH REQUIRED IMPORTED_TARGET mpich)
target_link_libraries(app PRIVATE PkgConfig::MPICH)
```

Make:

```
CFLAGS += $(shell pkg-config --cflags mpich)
LDLIBS += $(shell pkg-config --libs mpich)
```

## CMake's FindMPI

`find_package(MPI)` with the `MPI::MPI_C`, `MPI::MPI_CXX` and
`MPI::MPI_Fortran` targets works with MPICH:

```
find_package(MPI REQUIRED COMPONENTS C Fortran)
target_link_libraries(app PRIVATE MPI::MPI_C)
```

FindMPI looks for the wrappers first, in `PATH`, under `MPI_HOME`, or as
given by `MPI_<LANG>_COMPILER`, and asks them for the flags with
`-compile-info` and `-link-info`. If it finds no working wrapper, it
uses the pkg-config modules `mpi-c`, `mpi-cxx` and `mpi-fort`. In
current CMake (checked with 3.31), the library found through pkg-config
is searched for only in CMake's default library paths, so add the MPICH
prefix to `CMAKE_PREFIX_PATH` (or its `lib` directory to
`CMAKE_LIBRARY_PATH`) when MPICH is not installed in a system directory.

## For packagers

- `--disable-generic-pkgconfig` skips installing `mpi-c.pc`,
  `mpi-cxx.pc` and `mpi-fort.pc`. Use it when several MPI
  implementations share one pkg-config directory and the distribution
  manages these names itself (e.g. through alternatives). The
  distribution must then provide `mpi-c` itself, since `mpich-fortran`
  requires it for `--cflags` and `--static`.
- The `.pc` files go to `$libdir/pkgconfig`. Set `pkgconfigdir` in the
  environment at configure time to install them elsewhere.
- Build `LDFLAGS` (e.g. hardening flags) are not added to the wrappers or
  the `.pc` files, so a distribution does not need to filter them out.

## For FindMPI maintainers

MPICH 5.1 installs `mpi-c.pc`, `mpi-cxx.pc` and `mpi-fort.pc`, so
FindMPI's existing pkg-config step now finds MPICH (#6080, #6845). No
change is required for that. The following changes would make the step
more reliable. They apply to any MPI that ships these files, not only
MPICH.

1. **Search the library in the pkg-config library directories.** The
   step passes each entry of `MPI_<LANG>_PKG_LIBRARIES` to
   `find_library` with `HINTS` set to the entry's own directory. The
   entries are plain names (`mpi`, `mpifort`), so the hint is empty and
   only CMake's default paths are searched. For an MPI outside those paths
   the result is `MPI_mpi_LIBRARY-NOTFOUND` even though pkg-config
   found it. Use `MPI_<LANG>_PKG_LIBRARY_DIRS` as the hints, or use the
   full paths that `pkg_check_modules` already resolves in
   `MPI_<LANG>_PKG_LINK_LIBRARIES` (CMake 3.12 and later).

2. **Don't put the libraries into the link flags.**
   `MPI_<LANG>_LINK_FLAGS` is set from `MPI_<LANG>_PKG_LDFLAGS`, which is
   the whole `Libs` line, so `-L` and `-lmpi` appear in it as well as in
   `MPI_<LANG>_LIB_NAMES`. Use `MPI_<LANG>_PKG_LDFLAGS_OTHER`, which has
   only the remaining flags (e.g. the runpath flags).
   Likewise, `MPI_<LANG>_COMPILE_OPTIONS` is set from
   `MPI_<LANG>_PKG_CFLAGS`, which repeats the `-I` flags already in
   `MPI_<LANG>_INCLUDE_PATH`; `MPI_<LANG>_PKG_CFLAGS_OTHER` avoids that.

3. **Fortran modules.** For `mpi-fort`, the `-I` flags in `Cflags` include
   the module directory when it differs from the include directory, so
   `MPI_Fortran_PKG_INCLUDE_DIRS` is the right search path for `mpi.mod`
   and `mpi_f08.mod`. The directory is also available as
   `pkg-config --variable=modincdir mpich-fortran`.

4. **Static linking.** `pkg_check_modules` also returns the
   `--static` results as `MPI_<LANG>_PKG_STATIC_*`. Using them when a
   static MPI is wanted adds the external libraries from `Libs.private`.

5. **Older MPICH.** Before 5.1, MPICH installs only `mpich.pc`. Its
   `Cflags` and `Libs` are correct for C, and also for C++ unless the
   deprecated C++ bindings are used. FindMPI can try `mpich` when `mpi-c`
   or `mpi-cxx` is not found. Older MPICH has no pkg-config file for
   Fortran.

6. **Wrapper interrogation.** The current `-compile-info`/`-link-info`
   step works with MPICH. `mpicc` and `mpicxx` (MPICH 4.0 and later) also
   accept `-show-compile-info` and `-show-link-info`, which print the
   flags without the compiler name; `mpifort` does not have them yet.
