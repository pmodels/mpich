# Stand-Alone Fortran Binding

This document describes the design of the MPICH Fortran binding as a
stand-alone package that can be built against any MPI implementation.

## Overview

The Fortran binding (`libmpifort`) is structured as a separate package
under `src/binding/fortran/`. It has its own `configure.ac` and can be
built in two modes:

1. **As part of MPICH** — invoked as a sub-configure by the main MPICH
   build system (`FROM_MPICH=yes`).
2. **Stand-alone** — built independently against an existing MPI
   installation.

The package produces `libmpifort.so` and installs the `mpifort` compiler
wrapper script (with optional `mpif90` and `mpif77` symlinks).

## Stand-alone Build

### Basic Usage

```sh
configure --with-mpi=/path/to/mpi/install
make && make install
```

If `--with-mpi` is omitted, configure auto-detects by running:
```sh
echo '#include <mpi.h>' | mpicc -M -x c - | grep -oE '/[^ ]*/mpi\.h'
```
and derives the install prefix from the located `mpi.h`.

If `--enable-mpi-abi` is set, configure looks for `mpicc_abi` instead of
`mpicc` during auto-detection.

### MPI Library Discovery

Configure probes for both `libmpi` and `libmpi_abi`:

```
AC_CHECK_LIB([mpi],     [MPI_Init], ...)
AC_CHECK_LIB([mpi_abi], [MPI_Init], ...)
```

Selection logic:
- `--enable-mpi-abi=yes` — require `libmpi_abi`, error if not found.
- Default — prefer `libmpi`, fall back to `libmpi_abi`.

Configure also checks for `libpmpi` / `libpmpi_abi` (profiling libraries)
and adds them if found.

### MPI Feature Detection

Configure probes the underlying MPI for:

- **Type sizes**: `sizeof(MPI_Aint)`, `sizeof(MPI_Offset)`,
  `sizeof(MPI_Count)` — needed for Fortran `INTEGER(KIND=...)` mappings.
  When built from MPICH, these are passed via exported variables.

- **MPICH detection**: compiles a test using the `MPICH` macro defined in
  `mpi.h`. This is used to determine whether MPICH-specific `#define`s
  (e.g. `-DMPI_ABI`) are needed.

- **MPIX extensions**: probes for `MPIX_Comm_create_keyval_x` to detect
  whether MPIX callback extensions are available (see below).

### Fortran Compiler Probing

A single Fortran compiler (`FC`) is used for both F77 and F90/F08 code.
Configure probes:

- Fortran integer sizes — to determine `INTEGER_KIND`, `ADDRESS_KIND`,
  `OFFSET_KIND`, `COUNT_KIND`.
- F08 support — `PAC_FC_2008_SUPPORT` determines whether `use mpi_f08`
  bindings are built.

### Configure Options

| Option | Description |
|--------|-------------|
| `--with-mpi=DIR` | Path to MPI installation |
| `--enable-mpi-abi` | Use `libmpi_abi` instead of `libmpi` |
| `--enable-fortran-call-pmpi` | Fortran wrappers call PMPI directly (avoids double-profiling) |
| `--disable-fortran-mpix` | Skip MPIX extensions in bindings |
| `--disable-gen-bindings` | Use pre-generated binding source files |

## Proposed MPIX Extensions for Fortran

The Fortran binding requires two categories of MPIX extensions to work
correctly with any MPI implementation. Both are needed because the
standard MPI C API lacks mechanisms for proper Fortran interoperability
in user-defined callbacks and attribute access.

### Category 1: Callback Extensions (`_x` variants)

#### The Problem

Fortran uses integer handles (e.g. `INTEGER :: comm`) while C uses opaque
types (e.g. `MPI_Comm`). User-defined callbacks — attribute copy/delete
functions, reduction operations, and error handlers — receive arguments in
the caller's convention. When a Fortran program registers a callback, the
MPI library will invoke it with C-typed arguments, which the Fortran code
cannot interpret.

The standard MPI API does not provide a mechanism to attach "extra state"
to callbacks in a way that allows a proxy layer to translate between C and
Fortran conventions.

For example, `MPI_Op_create(fn, commute, &op)` stores the function
pointer directly. When the library calls `fn(invec, inoutvec, &len,
&datatype)`, the `datatype` argument is a C `MPI_Datatype` (an opaque
pointer in MPI ABI), not a Fortran integer — so Fortran code cannot use
it.

Similarly, `MPI_Comm_create_keyval(copy_fn, delete_fn, &keyval, state)`
needs a proxy to translate the `comm` handle argument from C to Fortran,
but the proxy needs its own state (the original Fortran function pointer),
and there is no standard way to free that state when the keyval is freed.

#### The MPIX Solution

MPICH provides extended `_x` variants of callback-registration functions
that accept an extra state pointer and a free callback:

- `MPIX_Comm_create_keyval_x(copy_fn, delete_fn, free_fn, &keyval, state)`
- `MPIX_Win_create_keyval_x(...)`, `MPIX_Type_create_keyval_x(...)`
- `MPIX_Op_create_x(op_fn, free_fn, commute, state, &op)`
- `MPIX_Comm_create_errhandler_x(fn, &errhandler, state)`
- `MPIX_Win_create_errhandler_x(...)`, `MPIX_File_create_errhandler_x(...)`,
  `MPIX_Session_create_errhandler_x(...)`

These extensions enable the Fortran binding to:

1. Allocate a proxy state struct containing the original Fortran function
   pointer.
2. Register a C proxy function that receives the state, translates handles
   (using `MPI_Comm_toint()` etc.), and calls the Fortran function.
3. Register a free callback to deallocate the proxy state.

#### Implementation

The proxy logic lives in `src/binding/fortran/mpif_h/user_proxy.c`,
controlled by the `HAS_MPIX_CALLBACKS` compile-time flag.

Example for attribute keyvals:
```c
MPIX_Comm_create_keyval_x(F77_Comm_attr_copy_proxy,
                           F77_Comm_attr_delete_proxy,
                           F77_keyval_free, keyval_out, state);
```

Example for user-defined ops:
```c
MPIX_Op_create_x(F77_op_proxy, F77_op_free, commute, state, &op);
```

### Category 2: Attribute Access (`_as_fortran` variants)

#### The Problem

The MPI standard specifies different behavior for attribute get/set
depending on the calling language. In C, `MPI_Comm_set_attr` stores a
`void *` pointer value, and `MPI_Comm_get_attr` returns the pointer.
In Fortran, `MPI_Comm_set_attr` stores an `INTEGER(KIND=MPI_ADDRESS_KIND)`
value, and `MPI_Comm_get_attr` returns that integer value. For built-in
attributes like `MPI_TAG_UB`, C returns a pointer to the value (`int *`)
while Fortran returns the value itself.

The standard C `MPI_Comm_get_attr` / `MPI_Comm_set_attr` functions have
no parameter to indicate which language convention the caller expects.
When the Fortran binding calls the C function, the implementation needs
a way to know that Fortran value semantics should be used.

#### The MPIX Solution

MPICH provides `_as_fortran` variants that use Fortran value semantics:

- `MPIX_Comm_get_attr_as_fortran(comm, keyval, &val, &flag)`
- `MPIX_Comm_set_attr_as_fortran(comm, keyval, val)`
- `MPIX_Type_get_attr_as_fortran(...)`, `MPIX_Type_set_attr_as_fortran(...)`
- `MPIX_Win_get_attr_as_fortran(...)`, `MPIX_Win_set_attr_as_fortran(...)`

Internally, these call the same `comm_get_attr` / `comm_set_attr`
implementation with `as_fortran = true`, which changes how built-in
attribute values are returned (value instead of pointer-to-value) and how
user-defined attribute values are stored and retrieved.

#### Implementation

The generated Fortran bindings (both `mpif.h` and `use mpi_f08`) route
attribute calls through these MPIX functions when available. For example,
in the f77 binding generator (`binding_f77.py`):
```python
if has_mpix:
    c_func_name = "MPIX_Comm_%s_attr_as_fortran" % get_or_set
else:
    c_func_name = "MPI_Comm_%s_attr" % get_or_set
```

## Fallback Without MPIX

When building against an MPI implementation that does not provide these
MPIX extensions (or with `--disable-fortran-mpix`), the binding falls
back to standard MPI functions with degraded behavior:

| Feature | With MPIX | Fallback (without MPIX) |
|---------|-----------|------------------------|
| Attr keyval | Proxy translates handles; state freed via `free_fn` | Proxy still translates handles, but state **leaks** (no `free_fn`) |
| User op | Proxy converts `MPI_Datatype` to `MPI_Fint` via `MPI_Type_toint` | Fortran function called directly by MPI; `datatype` argument is a C pointer, **unusable in Fortran** |
| Error handler | Proxy converts handle to `MPI_Fint` | Fortran function called directly; handle argument is a C pointer, **unusable in Fortran** |
| Attr get/set | Uses `MPIX_*_as_fortran` with correct Fortran value semantics | Uses `MPI_*_get_attr` / `MPI_*_set_attr` with C pointer semantics (see notes below) |

The fallback is functional for programs that do not inspect callback
arguments (e.g. error handlers that only print the error code, or ops
that ignore the datatype).

**Attribute fallback details:**
- **Built-in attributes** (e.g. `MPI_TAG_UB`): handled by
  `MPII_Attr_convert_builtin()` in `user_proxy.c`, which detects built-in
  keyvals at runtime (using ABI-specific heuristics on `MPI_TAG_UB`) and
  converts the pointer-to-value to a plain value.
- **Single-language usage**: set/get from a single language works because
  the library consistently treats the data as a pointer value. Fortran
  treats the pointer value as an integer, but this is consistent within
  the same language.
- **Inter-language usage**: setting an attribute in C and getting it in
  Fortran (or vice versa) will **not** work correctly without the MPIX
  extensions, because C stores a pointer while Fortran expects an integer
  value.

### Detection

Configure probes for the extensions:
```
AC_CHECK_FUNCS([MPIX_Comm_create_keyval_x],
    [has_mpix_callbacks=yes], [has_mpix_callbacks=no])
```

If not found, `HAS_MPIX_CALLBACKS` is not defined and the Python binding
generators receive `-skip-mpix` to omit MPIX function wrappers from the
generated Fortran bindings.

Users can explicitly disable with `--disable-fortran-mpix`.

## Fortran Initialization: `MPIX_Init_fortran`

The Fortran binding needs to register Fortran-specific type information
(sizes of `MPI_INTEGER`, `MPI_REAL`, etc.) with the MPI library. This is
done by `MPIX_Init_fortran()` in `setbot.c`, which:
- Calls `MPI_Abi_get_fortran_info` / `MPI_Info_create` /
  `MPI_Abi_set_fortran_info` to pass type size information via an `MPI_Info`
  object.
- Calls `MPI_Abi_set_fortran_booleans` to register Fortran `.TRUE.` and
  `.FALSE.` values (using `F77_TRUE_VALUE` / `F77_FALSE_VALUE`) along with
  `MPI_LOGICAL_SIZE`.

`MPIX_Init_fortran` is called in two ways:
- From the Fortran binding's own initialization (`mpirinitf_` in
  `setbot.c`), invoked on first use of `mpif.h` or `use mpi`.
- From MPICH's `MPI_Init` via `dlsym(RTLD_DEFAULT, "MPIX_Init_fortran")`
  — if `libmpifort` is linked, the symbol is found and called
  automatically.
