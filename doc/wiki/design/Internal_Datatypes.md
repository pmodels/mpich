# MPICH Internal Datatypes

MPICH used to directly use MPI builtin datatype internally. There are a few issues -

* The datatype constants such as MPI_INT are decided by `configure`. This is because
  internally we use handle bits to determine the builtin type size and `configure`
  figures out the type size. Thus, the MPI datatype constants are not true
  constants in a strict sense. They change between platforms or compilers.

* Not all MPI datatypes are supported. Configure defines unsupported datatype to
  MPI_DATATYPE_NULL. This may result in compilation error when application build
  on different platforms.

* MPI datatypes explodes with addition of new language bindings and new type
  aliases. Treating every MPI datatype independently creates ever-increasing
  support burden.

* MPI ABI proposal for MPI 5 requires implementations to dynamically configuring
  datatypes. For example, MPI_Abi_set_fortran_info can set and enable Fortran
  datatypes such as MPI_INTEGER and MPI_REAL at runtime. Thus, the original
  `configure` design won't work.

To address these issues, we redesigned MPICH's builtin datatype support by using
a set of *internal types*. Internal types are defined in src/include/mpir_datatype.h, 
with names such as `MPIR_INT32`, `MPIR_INT64`, `MPIR_FLOAT32`, `MPIR_FLOAT64`,
etc. Because they are all fixed-width types, the handle bits are also fixed and
all internal code that derives type info from handle bits still work.

In contrast, MPI builtin types such as MPI_INT, are now referred to as
*external* builtin types. The support of *external* builtin types is by mapping
them to *internal types*. For example, `configure` will define `MPIR_INT_INTERNAL`
to `MPIR_INT32` and these `_INTERNAL` macros will be used to construct a
internal table - `MPIR_Internal_types[]` in `src/mpi/datatype/typeutilc` that
maps external builtin type to internal types. The table can be updated at
runtime.

For communication, we usually only need the type size and treat the data as
bytes. If you directly use *external* builtin types *internally*, it may still
work as long as the type size derived from the handle bits match. This is true
for common platforms such as x86-64 but cannot be relied on. Thus, we require
internal code always use *internal* types, e.g. `MPIR_INT_INTERNAL` rather than
`MPI_INT`. There are assertions in critical places to catch incidents when
external types are direct used internally.

To perform reduction operation, internal types need the corresponding "C" type.
This is set by configure and defined in macros such as `MPIR_INT32_CTYPE`. The
`_CTYPE` macros may be not defined if the corresponding internal type does not
have a corresponding C type or it is not supported by the platform and compiler.
For example, `MPIR_INT128_CTYPE` may be undefined (in contrast, `MPIR_INT128` is
always defined).

Yaksa kernels may also need map to basic C type to work even for communications.

## External/Internal Type Conversions

The binding layer will use the macro `MPIR_DATATYPE_REPLACE_BUILTIN` to convert
MPI builtin datatypes from user input into internal types before passing into
internal routines. Thus, internal routines only need support the finite set of
internal types. Because we don't need all handle bits to encode an internal
type, we use the last byte to encode the original MPI builtin type which it is
converted from. For example, `MPI_INT` will be converted to `0x4c810405`
althout `MPIR_INT32` is `0x4c810400`. Thus, we can recover the original
datatype when we need to, such as the case of calling user callbacks.

Because of this, if internally we need to switch and compare to internal
constants, we need mask off the index bits. There are following helper macros:

* `MPIR_DATATYPE_GET_ORIG_BUILTIN(type)` - retrieve the original builtin type
* `MPIR_DATATYPE_GET_RAW_INTERNAL(type)` - mask off the index bits

It will be good to know there is no impact to derived datatypes.

## Migration Tips

Downstream migration is simple but unfortunately tedious. Basically, you need to
find every places where *external* builtin types are directly used and replace
them by its internal macros. Commonly used internal types include - MPI_INT,
MPI_BYTE, MPI_AINT, etc. Replace them by `MPIR_INT_INTERNAL`, `MPIR_BYTE_INTERNAL`,
`MPIR_AINT_INTERNAL`. Of course, if the internal use case is fixed-size types,
you can directly use internal types such as MPIR_INT64.
