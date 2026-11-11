# The `mpi.h` Header

This page describes how `src/include/mpi.h.in` is organized. One installed
`mpi.h` serves two ABIs, the MPICH ABI and the MPI ABI of MPI-5.0, and it is
also the header the MPI ABI library itself is compiled with. For the
user-facing side (configure options, `mpicc` flags, installed files), see
[MPI ABI Build Interface](../how_to/MPI_ABI_Build_Interface.md).

## The Three Modes

`mpi.h` is consumed in three modes, selected by two macros:

| Mode | Consumer | `MPI_ABI` | `MPICH_BUILD_MPI_ABI` | Set by |
|---|---|---|---|---|
| 1. MPI ABI header | user code built for `libmpi_abi` | defined | – | `mpicc` for the MPI ABI (`src/env/mpicc.def`) |
| 2. MPICH ABI header | user code built for `libmpi`, and the `libmpi` build itself | – | – | default |
| 3. MPI ABI library build | the MPICH sources compiled into `libmpi_abi` | – | defined | `abi_cppflags` in `Makefile.am` |

`MPI_ABI` is an input switch: it picks the branch of `mpi.h`, but it is not
what code should test afterwards. To tell which ABI a header provides, test
the macros the header defines:

- `MPICH` is defined in every mode, so code can tell that the header comes
  from MPICH whichever ABI it is.
- `MPI_ABI_VERSION` is defined only by the MPI ABI header (`mpi_abi.h`), as
  the MPI standard requires. MPICH's own headers, such as `mpix.h`, use it
  to branch between the two ABIs.

## Layout of mpi.h.in

```
#define MPICH                              always
#ifdef MPI_ABI                             -- mode 1
    #include "mpi_abi.h"
    #include "mpix.h"           if MPI_MPIX
    #include "mpi_legacy.h"     if MPI_LEGACY
    #include "mpi_fortran.h"    if MPI_FORTRAN
#else                                      -- modes 2 and 3
    #define MPI_MPIX                       MPIX is always on
    #ifdef MPICH_BUILD_MPI_ABI             -- mode 3 only
        #include "mpi_abi_internal.h"
    #endif
    ... MPICH native types and constants ...
    #ifndef MPICH_BUILD_MPI_ABI            -- mode 2 only
        #include "mpix.h"
        #include "mpi_proto.h"
        #include "mpicxx.h"       if C++
        #include "mpi_fortran.h"  if MPI_FORTRAN
    #endif
#endif
```

### Mode 1: MPI ABI header

`mpi.h` is only a switch: everything comes from `mpi_abi.h`, the standard
ABI header kept in sync with the MPI Forum's `mpi-abi-stubs`
(`src/binding/abi/mpi_abi.h`). The MPICH-specific parts are opt-in, because
code that uses them is no longer portable across MPI ABI implementations:

| Macro | `mpicc` flag | Header | Provides |
|---|---|---|---|
| `MPI_MPIX` | `-mpi-mpix` (alias `-mpix`) | `mpix.h` | MPIX extensions |
| `MPI_LEGACY` | `-mpi-legacy` | `mpi_legacy.h` | APIs removed from the MPI standard, mapped to their replacements (no `MPI_LB`/`MPI_UB`) |
| `MPI_FORTRAN` | `-mpi-fortran` | `mpi_fortran.h` | Fortran interop (`MPI_Fint`, f2c/c2f), see [Stand-Alone Fortran Binding](Standalone_Fortran_Binding.md) |

### Mode 2: MPICH ABI header

This is the traditional MPICH `mpi.h`. Handles are `int`s with MPICH's own
encoding. `MPI_MPIX` is always defined, so `mpix.h` is always included.
Prototypes come from the generated `mpi_proto.h`, which also declares the
APIs removed from the standard; `mpi_legacy.h` is not used in this mode.
`mpix.h` comes before `mpi_proto.h` because `mpi_proto.h` declares the MPIX
functions too, and their types (`MPIX_Stream`, `MPIX_Iov`, ...) are defined
in `mpix.h`. `mpicxx.h` is included if the C++ binding is configured.

### Mode 3: MPI ABI library build

`libmpi_abi` is built from the same sources as `libmpi`, with
`-DMPICH_BUILD_MPI_ABI`. The internals keep using MPICH's native handles,
while the public functions take MPI ABI handles and convert them. To make
this work, `mpi.h` first includes `mpi_abi_internal.h`, which
`maint/gen_abi.py` generates from `mpi_abi.h` and `mpix.h`:

| From the MPI ABI headers | In `mpi_abi_internal.h` | Why |
|---|---|---|
| Handle types and constants, e.g. `MPI_Comm`, `MPI_COMM_WORLD` | renamed `ABI_Comm`, `ABI_COMM_WORLD` | the native `MPI_Comm` and `MPI_COMM_WORLD` follow in `mpi.h` and are what the internals use |
| Integer constants, e.g. `MPI_COMM_TYPE_SHARED` | kept, with the MPI ABI value (221) | the internals are compiled with the ABI values, so these need no conversion |
| `MPI_`/`MPIX_` prototypes | handle parameters use `ABI_` types, plus `MPICH_API_PUBLIC` | these are the functions `libmpi_abi` exports |

The rest of `mpi.h` then defines the native types and handle constants as in
mode 2. The native integer constants that the MPI ABI defines differently,
such as `MPI_COMM_TYPE_SHARED` (1 in the MPICH ABI), sit inside
`#ifndef MPICH_BUILD_MPI_ABI` blocks, so the ABI values from
`mpi_abi_internal.h` win. The tail of `mpi.h` (`mpix.h`, `mpi_proto.h`,
C++, `mpi_fortran.h`) is skipped; the MPIX declarations already come from
`mpi_abi_internal.h`.

The public functions are the generated wrappers in
`src/binding/abi/c_binding_abi.c`. They convert handle arguments with the
`ABI_<Type>_to_mpi` and `ABI_<Type>_from_mpi` functions in
`src/binding/abi/mpi_abi_util.h`, and pass everything else through. The
datatype and op conversion tables come from `mpi_abi_util.c`, also generated
by `maint/gen_abi.py`.

In a dual build (`--enable-mpi-abi=dual`) every source is compiled twice, once
in mode 2 for `libmpi` and once in mode 3 for `libmpi_abi`, so a constant can
have different values in the two libraries.

ROMIO does not go through `mpi.h` in this mode: its build includes the
generated `romio_abi_internal.h`, which keeps the MPI ABI types
(`src/mpi/romio/adio/include/adio.h`).

## Rules for Editing mpi.h.in

- A native integer constant that the MPI ABI also defines goes inside an
  `#ifndef MPICH_BUILD_MPI_ABI` block, so mode 3 gets the ABI value.
- Native handle types and handle constants stay outside those blocks; mode 3
  needs them for the internals.
- MPIX constants go in `mpix.h`. Those passed through `libmpi_abi`
  unconverted and with no conflict in either ABI are defined once, for both
  ABIs. Those that differ, such as handles, are defined in the
  `#ifdef MPI_ABI_VERSION` branch of `mpix.h` for the MPI ABI and in
  `mpi.h.in` for the MPICH ABI.
- After changing a header that `maint/gen_abi.py` reads, check the generated
  `src/binding/abi/mpi_abi_internal.h` to see what mode 3 gets.
