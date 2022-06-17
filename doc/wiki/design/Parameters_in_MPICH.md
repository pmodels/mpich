# Parameters In MPICH

## Parameter Handling within MPICH

The following is a *draft* of a proposed design to provide uniformity
and adaptability to the current ad hoc mechanisms within MPICH

Goals:
1.  Uniform handling of parameters used to tune or otherwise control
    MPICH
2.  Allow (where sensible) updates during run (e.g., to permit
    autotuning experiments)
3.  Integrate with documentation (no hidden parameter codes)
4.  Constraints as below.

Constraints:
1.  Simple use for integer-valued parameters
2.  Optimize for compile-time and run-time cases
3.  Single initialization (not each time the routine is called);
    thread-safe initialization
4.  Ability to have error checking and reporting (i.e., incorrect values
    provided by the user are identified and reported).
5.  Markup to indicate scope (local or collective) and whether it can be
    updated at runtime; what to do on modification (if allowed)
6.  Ability to specify parameter values through several mechanisms,
    including but not limited to environment variables
7.  Ability to have run-time changes (subject to the constraints in 5)

Implications:

1 & 2 imply the option to use a constant or an integer variable for the
value

3, 4, 5, and 6 imply (in the runtime case) an initialization call that
can access multiple sources and check that the input is valid (note that
much of the current code will accept "2k" as an integer value, but will
neither return 2048 nor indicate an error)

7\. Implies an interface to permit programmer-caused parameter changes
and to ensure that any values that depend on these are recomputed. It
also implies that the initialization step may need to be re-executed
when a value changes.

### Design

1\. Make all parameters have consistent naming strategy

Use a common prefix and regular pattern

``` c
MPIR_PARAM_<type>_<module>_<name>
```

e.g.,

``` c
MPIR_PARAM_INT_ALLTOALL_MEDIUM_MSG
```

This can be used directly in the code (e.g., if (msgsize ==
MPIR_PARAM_xxx) )

2\. Initialization. Note that this can't be done (in call cases) within
MPI_Init/Init_thread because of the lazy linking / modularization
approach, though that should be an option.

In all cases, the relevant code contains an initialization call; this is
a macro of the form

``` c
    MPIR_PARAM_INIT_INT( varname, envname, default-value, err );
```

Note in the compile-time case, this may expand into a no-op.

  - Issues: indicate scope, what to do if the value is changed during
    execution, documentation. Should we instead have


``` c
    MPIR_PARAM_INIT_INT( varname, envname, default-value, changeable?, comm, updatefcn, err );
```

where changeable? is no (init-time only), yes-local (each process can
change independently), yes-collective (all processes in the same
communicator (the comm argument) must agree on the value); comm is the
scope of the parameter (may be null/empty); updatefcn is a function that
is called if the value is changed (signature to be defined); and err is
an error variable (should this be, or should it also include, a label to
which to jump?).

  - Note that this still doesn't include documentation. Perhaps that
    should have a structured comment:

``` c
 /* -- PARMDESC --
    ENVNAME: environment name
    TYPE: integer (etc.)
    DESC: short description (terminate with an empty line or with the next field name (DEFAULT:))
    DEFAULT: default value
  */
```

The default value could instead be extracted from the
MPIR_PARAM_INIT_xxx macro, or it could be compared with it for
consistency (which would help identify description that may be
out-of-date, as their default value may be out-of-date).

### Usage Examples

The cases of compile-time-only and runtime have different enough
requirements that some code will be conditionally included based on the
value of `MPIR_PARAM_IS_RUNTIME` (set to `1` if parameters may be
changed at runtime and `0` if not).

These are *macros*.

Expansions include these:

For the case where parameters are defined at compile time (i.e., not
defined at runtime), the declaration of the variable must be made
explicitly. For example, in the source file that needs the parameter
value,

``` c
#if MPIR_PARAM_IS_RUNTIME
MPIR_PARAM_DECL_INT(ALLTOALL_MEDIUM_MSG,1024);
#else
#define MPIR_PARAM_ALLTOALL_MEDIUM_MSG 1024
#endif
```

The definitions of these macros might look something like the following

``` c
#if ! MPIR_PARAM_IS_RUNTIME
     #define MPIR_PARAM_INIT_INT( _varname, _envname, _defval, _err )
     #define MPIR_PARAM_DECL_INT( _varname, _defval ) \
     static int MPIR_PARAM_dummy_##_varname = _defval;
#else
     #define MPIR_PARAM_INIT_INT( _varname, _envname, _defval, _err ) \
     MPIU_THREADSAFE_INIT_STMT(MPIR_PARAM_NOTSET_##_varname,\
               MPIR_Param_init_int( _envname, &MPIR_PARAM_##_varname ))
     /* volatile needed only if multithreaded */
     #define MPIR_PARAM_DECL_INT( _varname, _defval ) \
     MPIU_THREADSAFE_INIT_DECL(MPIR_PARAM_NOTSET_##_var);\
     static int MPIR_PARAM_##_varname = _defval;
#endif
```
