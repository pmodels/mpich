# CVARS

This page will discuss `CVAR`s.

CVARs are declared in comment blocks such as:

```
/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===
cvars:
    - name        : MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select iallgather algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        ring               - Force ring algorithm
        brucks             - Force brucks algorithm

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/
```

It is in `YAML` format which means that although it looks like plain text, it
is not. The format matters here, especially the indentations. The above block
is parsed into a perl `HASH` with key `cvars` pointing to an array of items,
each is a `HASH` with keys:

```
name, category, ..., description.
```

If the type is `enum`, the possible values of the enum need be listed in the
description block. An enum entry is recognized with regex pattern

```
/^\s*(\w+)\s+-\s/m
```

i.e. a word leading the line followed by ` - `.

The above example represents (in `src/include/mpir_cvars.h`):
```
extern int MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM;
enum MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM_choice {
    MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM_auto,
    MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM_ring,
    MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM_brucks
};
```
Note the first entry in the `enum` is `0`.

`autogen.sh` runs the perl script `maint/extractcvars`, which looks through all
`.c` and `.h` files in a list of directories listed in the `@dirs` variable.

`maint/extractcvars` generates `src/include/mpir_cvars.h` and
`src/util/mpir_cvars.c`. The former is included by `mpiimpl.h`, thus every file 
declares `MPIR_CVAR_...` extern variables. The latter defines
`MPIR_T_cvar_init/finalize` functions that are called by `MPI_Init/Finalize`.
The `MPIR_T_cvar_init` function checks environment variable for each `CVAR`
variable and sets values according to the `CVAR` data type.
