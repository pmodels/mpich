# MPI ABI Build Interface

This document describes how `mpicc` (and `mpicxx`) handles ABI selection
and optional feature flags.

## ABI Selection

### Configure: `--enable-mpi-abi`

The `--enable-mpi-abi` configure option controls which ABI libraries are
built:

- `no` or `mpich` (default) — build only `libmpi.so` (MPICH ABI)
- `yes` or `mpi` — build only `libmpi_abi.so` (standard MPI ABI)
- `both` or `dual` — build both libraries

The configure sets `DEFAULT_MPI_ABI`, which is substituted into the
`mpicc` script as the initial value of the `mpi_abi` variable.

### `mpicc` Options

- `-mpi-abi` (or `-mpi_abi`) — select standard MPI ABI
- `-mpich-abi` (or `-mpich_abi`) — select MPICH ABI

When MPI ABI is selected, `mpicc` adds `-DMPI_ABI` to preprocessor
flags and links against `libmpi_abi.so` instead of `libmpi.so`.

In a single-ABI build, the default is already set by configure and these
options are unnecessary (but still accepted). They are primarily useful
in a dual build (`--enable-mpi-abi=both`).

### Convenience Symlinks

At install time, symlinks are created:

- `mpicc_abi` — equivalent to `mpicc -mpi-abi`
- `mpicc_mpich` — equivalent to `mpicc -mpich-abi`
- `mpicxx_abi` — equivalent to `mpicxx -mpi-abi`
- `mpicxx_mpich` — equivalent to `mpicxx -mpich-abi`

The wrapper detects the invocation name via `$0`:
```sh
case "$0" in
    *_abi)   mpi_abi=yes ;;
    *_mpich) mpi_abi=no ;;
esac
```

This runs after argument parsing, so the invocation name takes
precedence over command-line options.

## Optional Feature Flags

These options control optional includes and libraries. They are
available in `mpicc` and `mpicxx`.

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
routines. Pure C or pure Fortran programs do not need it.

### `-mpi-legacy`

Defines `MPI_LEGACY`, which causes `mpi.h` to include `mpi_legacy.h`.
This provides deprecated or removed MPI symbols for backward
compatibility.
