# MPI ABI Build Interface

This document describes how to build MPICH with the standard MPI ABI, how
`mpicc` (and `mpicxx`) handles ABI selection and optional feature flags,
and how the Fortran binding and the test suite fit in.

MPI 5.0 defines a standard application binary interface (the "MPI ABI"):
a fixed `mpi_abi.h` with standard handle types, constant values, and a
standard library name, `libmpi_abi.so`. An application compiled against
the MPI ABI can run on any MPI implementation that provides it, without
recompiling.

MPICH can build two ABIs:

| ABI | Header | Library | Notes |
|---|---|---|---|
| MPICH ABI | `mpi.h` | `libmpi.so.12` (releases) | The traditional MPICH ABI, shared by MPICH derivatives (Intel MPI, Cray MPICH, MVAPICH, ...). See [ABI Compatibility Initiative](../testing/ABI_Compatibility_Initiative.md). |
| MPI ABI | `mpi_abi.h` (via `mpi.h -DMPI_ABI`) | `libmpi_abi.so.1` | The MPI-5.0 standard ABI, version 1.0. |

## ABI Selection

### Configure: `--enable-mpi-abi`

The `--enable-mpi-abi` configure option controls which ABI libraries are
built:

- `no` or `mpich` (default) — build only `libmpi.so` (MPICH ABI)
- `yes` or `mpi` — build only `libmpi_abi.so` (standard MPI ABI)
- `both` or `dual` (also `mpich,mpi` or `mpi,mpich`) — build both libraries

Any other value is a configure error. The summary printed at the end of
configure shows the choice, e.g.

```
*** ABI: MPICH ABI, MPI ABI 1.0
```

The configure sets `DEFAULT_MPI_ABI`, which is substituted into the
`mpicc` script as the initial value of the `mpi_abi` variable. It is
`yes` only for an MPI-ABI-only build. In a dual build `mpicc` defaults to
the MPICH ABI. This may change in a later release, so scripts that care
should call `mpicc_abi` or `mpicc_mpich` (or pass `-mpi-abi` /
`-mpich-abi`) explicitly.

Internally, configure turns the option into `enable_mpi_abi` and
`enable_mpich_abi`, which become the automake conditionals
`BUILD_ABI_LIB` and `BUILD_MPICH_LIB`.

> **Warning: use dual builds only for development and testing.** A dual
> installation holds two MPI libraries but only one Fortran binding
> (`libmpifort`, `mpifort`, `mpi_fortran.h`), which is built on the MPICH
> ABI. Anything that links `libmpifort` from the other ABI, for example
> `mpicc_abi -mpi-fortran`, ends up loading both `libmpi_abi.so` and
> `libmpi.so` in one process. Users, and downstream packages that detect
> MPI through `mpicc`, `mpifort`, or pkg-config, can easily pick up the
> ABI they did not intend. For production installations and distribution
> packages, build one ABI per installation: `--enable-mpi-abi` or
> `--disable-mpi-abi`. If both are needed, install them into separate
> prefixes.

### `mpicc` Options

- `-mpi-abi` (or `-mpi_abi`) — select standard MPI ABI
- `-mpich-abi` (or `-mpich_abi`) — select MPICH ABI

When MPI ABI is selected, `mpicc` adds `-DMPI_ABI` to preprocessor
flags and links against `libmpi_abi.so` instead of `libmpi.so`.

In a single-ABI build, the default is already set by configure and these
options are unnecessary (but still accepted). They are primarily useful
in a dual build (`--enable-mpi-abi=both`). Selecting an ABI that was not
built (for example `mpicc -mpi-abi` in a MPICH-ABI-only build) fails at
link time, because the library is not installed.

For example, in a dual build:

```
$ mpicc -show
gcc -I$PREFIX/include -L$PREFIX/lib -Wl,-rpath -Wl,$PREFIX/lib -Wl,--enable-new-dtags -lmpi
$ mpicc_abi -show
gcc -DMPI_ABI -I$PREFIX/include -L$PREFIX/lib -Wl,-rpath -Wl,$PREFIX/lib -Wl,--enable-new-dtags -lmpi_abi
$ mpicc_abi -mpi-fortran -mpi-mpix -show
gcc -DMPI_ABI -DMPI_MPIX -DMPI_FORTRAN -I$PREFIX/include ... -Wl,--no-as-needed -lmpifort -lmpi_abi
```

A binary built with `mpicc_abi` needs only `libmpi_abi.so.1` at run time.
Because the library name and SONAME are standard, it can run against any
installation that provides `libmpi_abi.so.1`, for example by pointing
`LD_LIBRARY_PATH` at that installation's `lib` directory. `mpiexec` is
the same for both ABIs.

### Convenience Symlinks

At install time, symlinks are created:

- `mpicc_abi` — equivalent to `mpicc -mpi-abi`
- `mpicc_mpich` — equivalent to `mpicc -mpich-abi`
- `mpicxx_abi` — equivalent to `mpicxx -mpi-abi`
- `mpicxx_mpich` — equivalent to `mpicxx -mpich-abi`

Only the links for the ABIs that were built are installed: `_abi` when
the MPI ABI is built, `_mpich` when the MPICH ABI is built.

The wrapper detects the invocation name via `$0`:
```sh
case "$0" in
    *_abi)   mpi_abi=yes ;;
    *_mpich) mpi_abi=no ;;
esac
```

This runs after argument parsing, so the invocation name takes
precedence over command-line options (`mpicc_mpich -mpi-abi` still uses
the MPICH ABI).

## Optional Feature Flags

These options control optional includes and libraries. They are
available in `mpicc` and `mpicxx`.

With the MPICH ABI, `mpi.h` always provides the MPIX declarations. With
the MPI ABI, `mpi.h` includes only `mpi_abi.h`, and the flags below add
the optional parts (see `src/include/mpi.h.in`).

### `-mpi-mpix`

Defines `MPI_MPIX`, which causes `mpi.h` to include `mpix.h`. This
provides access to MPIX extension functions (e.g. `MPIX_Query_cuda_support`).

The MPIX declarations are split into a separate header because
`mpi_abi.h` does not declare them. This flag makes them available
regardless of which ABI is in use.

### `-mpi-fortran`

Defines `MPI_FORTRAN`, which causes `mpi.h` to include `mpi_fortran.h`.
This provides Fortran interoperability declarations (`MPI_Fint`,
`MPI_*_f2c`, `MPI_*_c2f`). In addition, it adds `-lmpifort` to the
linker.

This flag is only needed for C programs that call Fortran interop
routines. Pure C or pure Fortran programs do not need it. Without it,
such C code fails to compile on the MPI ABI with
`unknown type name 'MPI_Fint'`.

`-mpi-fortran` is only correct when the installed `libmpifort` was built
on the same ABI as the program. In a default dual build `libmpifort` is
built on the MPICH ABI, so `mpicc_abi -mpi-fortran` produces a program
that loads both `libmpi_abi.so` and `libmpi.so` (see the warning above).

### `-mpi-legacy`

Defines `MPI_LEGACY`, which causes `mpi.h` to include `mpi_legacy.h`.
This provides deprecated or removed MPI symbols for backward
compatibility.

## What Gets Built and Installed

### Libraries

| | MPICH ABI | MPI ABI |
|---|---|---|
| C library | `libmpi.so` (SONAME `libmpi.so.12` in releases) | `libmpi_abi.so` (SONAME `libmpi_abi.so.1`) |
| Profiling library, only on platforms without weak symbols | `libpmpi.so` | `libpmpi_abi.so` |
| ROMIO convenience library | `libromio.la` | `libromio_abi.la` |

Both libraries are compiled from the same sources. The MPI ABI library
is built with `-DMPICH_BUILD_MPI_ABI`, which makes `mpi.h` pull in the
generated `mpi_abi_internal.h`, and adds the conversion layer in
`src/binding/abi/` (`c_binding_abi.c`, `io_abi.c`, `mpi_abi_util.c`).
In a dual build every object is compiled twice, so expect the build to
take about twice as long.

The library names can be overridden with `MPIABILIBNAME` and
`PMPIABILIBNAME` (defaults `mpi_abi` and `pmpi_abi`). The SONAME versions
come from `libmpi_so_version_m4` and `libmpi_abi_so_version_m4` in
`maint/version.m4`. `libmpi_abi` is pinned to `1:0:0` so that every
implementation of ABI version 1 installs `libmpi_abi.so.1`. `libmpi` uses
`0:0:0` on the development branch and in alpha/beta releases, so a build
from `main` installs `libmpi.so.0`; stable releases restore the
`libmpi.so.12` SONAME.

### Headers

| Header | Installed when | Purpose |
|---|---|---|
| `mpi.h` | always | MPICH ABI header. With `-DMPI_ABI` it includes `mpi_abi.h` instead. |
| `mpi_abi.h` | MPI ABI is built | The standard MPI ABI header (`MPI_ABI_VERSION` 1, `MPI_ABI_SUBVERSION` 0). |
| `mpi_proto.h` | MPICH ABI is built | MPICH ABI prototypes. |
| `mpix.h` | always | MPICH extensions (`MPIX_`), see `-mpi-mpix`. |
| `mpi_legacy.h` | always | Deprecated or removed MPI symbols, see `-mpi-legacy`. |
| `mpi_fortran.h` | the Fortran binding is built | Fortran interoperability for C code, see `-mpi-fortran`. |

### Tools

`mpichversion` and `mpivars` are linked with `libmpi` when the MPICH ABI
is built, and with `libmpi_abi` otherwise. So `mpichversion` reports
`MPICH ABI: MPICH ABI 0:0:0` (the `libmpi` version) in MPICH-only and dual
builds, and `MPICH ABI: MPI ABI 1:0:0` in an MPI-ABI-only build.

`mpicxx` (and `mpicxx_abi`) is installed whenever a C++ compiler is
found. The C++ binding (`libmpicxx`) only works on the MPICH ABI and is
disabled in an MPI-ABI-only build, but `mpicxx` still compiles C++ code
that uses the C API.

## Fortran Binding

The Fortran binding (`src/binding/fortran`) is a separate package with its
own configure; see
[Stand-Alone Fortran Binding](../design/Standalone_Fortran_Binding.md)
for its design. The top-level configure runs it as a subpackage unless
`--disable-fortran` is given. It builds `libmpifort`, `mpifort` (and the
`mpif77`/`mpif90` links), and installs `mpi_fortran.h`. One Fortran
binding is built against exactly one ABI:

| Top-level option | Fortran binding built on |
|---|---|
| MPICH ABI only | MPICH ABI (`libmpi`) |
| `--enable-mpi-abi=yes` | MPI ABI (`libmpi_abi`) |
| `--enable-mpi-abi=dual` | MPICH ABI (`libmpi`) |

The top-level configure tells the subpackage which ABI to use through the
`WITH_MPI` environment variable (`mpich` or `abi`) and passes the library
to link through `mpi_libs` / `mpi_dependency_libs`. `mpifort` links
`libmpifort` and the matching C library (`-lmpi` or `-lmpi_abi`), so
mixed Fortran/C programs can still be linked with `mpifort`.

On the MPI ABI, the Fortran binding does not provide the `MPIX_` routines
(`--disable-fortran-mpix` is implied) and is compiled with
`FORTRAN_BUILD_MPI_ABI`.

### Fortran on the MPI ABI in a Dual Build

In a dual build the in-tree Fortran binding uses the MPICH ABI. For
testing, a Fortran binding on the MPI ABI can be built separately against
the installed MPICH. This is what the Jenkins `dual-mpi` configuration
does:

```
./autogen.sh                    # also prepares src/binding/fortran
./configure --prefix=$PREFIX --enable-mpi-abi=dual --disable-fortran ...
make -j && make install

mkdir build-fortran && cd build-fortran
../src/binding/fortran/configure --prefix=$PREFIX --with-mpi=$PREFIX --enable-mpi-abi
make -j && make install
```

The stand-alone configure can run in `src/binding/fortran` or in a
separate build directory.

A stand-alone `mpifort` adds `-I`/`-L` only for its own prefix. It
assumes the system is already set up to find the MPI library it was
built against, as with any MPI library installed in a system location.
If the Fortran binding goes into the MPICH prefix, as above, nothing more
is needed. With a different prefix, make `libmpi_abi` findable yourself,
e.g. with `LIBRARY_PATH` and `LD_LIBRARY_PATH`.

A prefix still holds only one `libmpifort`, `mpifort`, and
`mpi_fortran.h`. After the steps above, Fortran programs and
`mpicc_abi -mpi-fortran` use the MPI ABI, and `mpicc -mpi-fortran` (MPICH
ABI) must not be used.

In stand-alone mode, `--enable-mpi-abi` (`yes` or `mpi`) requires
`libmpi_abi`. Without the option, or with `--disable-mpi-abi` (`no` or
`mpich`), the binding is built on `libmpi` if it exists, else on
`libmpi_abi`. A Fortran binding supports only one ABI,
so the dual values (`dual`, `both`, `mpich,mpi`, `mpi,mpich`) are a
configure error in stand-alone mode. When MPICH runs the Fortran
configure as a subpackage, the option is ignored and the top-level
configure picks the ABI: the MPICH ABI in a dual build.

The Jenkins `dual-mpich` configuration builds the stand-alone binding
with `--disable-mpi-abi` instead.

## Running the Test Suite Against the MPI ABI

The test suite has its own option, which makes it compile the C tests
with `mpicc_abi` and turns off the C++ tests:

```
cd test/mpi
./configure --with-mpi=$PREFIX --enable-mpi-abi
make testing
```

When `--enable-mpi-abi` is given, the test suite also defaults to
`--enable-strictmpi`, which skips tests that use `MPIX_` functions or
other MPICH-specific behavior.

In Jenkins, the `mpi-abi`, `mpich-abi`, `dual-mpi`, and `dual-mpich`
configurations cover these builds; trigger them with `test:mpich/abi`
(see [Jenkins](../testing/Jenkins.md)). The GitHub Actions workflow
(`.github/workflows/ci-workflow.yml`) builds with
`--enable-mpi-abi=dual`, compiles a hello-world program with both `mpicc`
and `mpicc_abi`, and runs the mpi4py tests.

## pkg-config

Currently `$PREFIX/lib/pkgconfig/mpich.pc` always describes the MPICH
ABI (`-lmpi`, no `-DMPI_ABI`), even in an MPI-ABI-only build, where
`libmpi` is not installed. Until that is fixed, use `mpicc_abi -show` to
get the flags for the MPI ABI.

pmodels/mpich#8021 (open) fixes this. It adds per-language files
(`mpich.pc`, `mpich-cxx.pc`, `mpich-fortran.pc`, with `mpi-c`, `mpi-cxx`,
`mpi-fort` links) that follow the default ABI of the wrappers. In an
MPI-ABI-only build they give `-DMPI_ABI ... -lmpi_abi`; dual and default
builds give `-lmpi`.

## Recipes

MPI ABI only, for example for a distribution package of `libmpi_abi.so.1`:

```
./configure --prefix=$PREFIX --enable-mpi-abi
make -j && make install
mpicc hello.c -o hello          # MPI ABI; same as mpicc_abi
```

Both ABIs, for development or for comparing the two (Fortran on the
MPICH ABI; not for production, see the warning above):

```
./configure --prefix=$PREFIX --enable-mpi-abi=dual
make -j && make install
mpicc_mpich hello.c -o hello_mpich
mpicc_abi   hello.c -o hello_abi
mpiexec -n 2 ./hello_abi
ldd hello_abi | grep mpi        # only libmpi_abi.so.1
```

Both ABIs, with Fortran on the MPI ABI: see
[Fortran on the MPI ABI in a Dual Build](#fortran-on-the-mpi-abi-in-a-dual-build).

Check what an installation provides:

```
mpichversion | grep ABI
mpicc_abi -show
ls $PREFIX/lib/libmpi*.so* $PREFIX/include/mpi*.h
```

## Related Pull Requests

- pmodels/mpich#7949: `--enable-mpi-abi` takes `yes`/`no`/`dual`; adds
  `mpicc_mpich`, `mpi_fortran.h`, `-mpi-mpix`, `-mpi-fortran`.
- pmodels/mpich#7963: GitHub CI builds with `--enable-mpi-abi=dual`.
- pmodels/mpich#7958, pmodels/mpich#7968: f77/f90 and f08 bindings on the MPI ABI.
- pmodels/mpich#7976: stand-alone Fortran binding package.
- pmodels/mpich#7986: `mpifort` links `-lmpi` or `-lmpi_abi` explicitly.
- pmodels/mpich#7851: `mpix.h` for the ABI build.
- pmodels/mpich#7867: `libmpi_abi.so.1` SONAME.
