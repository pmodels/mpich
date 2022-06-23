# CH4 Inline Mechanism

This page discusses the inlining strategy in CH4.

The goal of inlining is to avoid the function call overhead, especially for
functions with many arguments. It is the most beneficial for the latency
sensitive code path, communication. For other functions like, `Init/Finalize`,
`Comm/Win`, `Create/Destroy`, inlining is less beneficial.

## Guidelines For Inlining

1. PT2PT, Collective and RMA communication functions need to be inlined. This
   also means any function that is used in this code path needs to be inlined
   as well. For example, request creation or AM fallback for communication.

2. Any function that is not described by the first rule does not need to be
   inlined.

## Uninlining Procedure For CH4 Layer

1. Create a `src/*.c` file, include `mpidimpl.h`.
2. For functions needing to be uninlined, move them to `*.c` and create
   declarations in the `*.h` file. Note that the functions implemented the ADI
   do not need this as the prototype is already declared in `mpidch4.h`.
3. Remove `static inline` or `MPL_STATIC_INLINE_PREFIX` from the uninlined
   functions (both declarations `mpidch4.h` and definitions)
4. For uninlined functions that are only used in the `*.c` file, make them
   “private”.
    - Remove their namespace prefix
    - Create declarations in `*.c` file
    - Make them `static`.
5. Remove the `*.h` file if is empty. `ch4r_*.h` files are kept for
   the decls.
6. Keep `CVAR` where it is used.

## Uninlining Procedure For Netmod/SHM Layer

1. Create `*.c` file, include `mpidimpl.h`.
2. For functions needing to be uninlined, move them to `*.c`. Rename from
   `MPIDI_NM_*` to `MPIDI_OFI_*`. Create decls and `#define` in
   `ofi_noinline.h`
3. Remove `static inline` or `MPL_STATIC_INLINE_PREFIX` from the uninlined
   functions (both declarations `netmod.h` and definitions). Move
   wrapper functions in `netmod_impl.h` to `netmod/src/netmod_impl.c`.
4. Make internal funcs “private”.
5. Remove the `*.h` file if it is empty.
6. Keep `CVAR` where it is used.

## For netmod, why do we need to create the `#define` in `*_noinline.h`?

The problem is sort of backwards from the case when `NETMOD_INLINE` is
disabled. Both `netmod_impl.c/h` and the `ofi_init.h/c` will have their own
`MPIDI_NM_*` (the one in `netmod_impl.c/h` is the wrapper). To solve this name
collision, we have to change the name of the uninlined functions to
`MPIDI_OFI_*`. Then we run into the problem when `NETMOD_INLINE` is enabled.
The function name does not match the defined interface name anymore, hence the
`#define` solution.
