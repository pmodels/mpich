# Coding Standards

Many of these standards are automatically checked by the
[Coding Style Checker](Coding_Style_Checker.md).

Some of the information on this page is out of date (for example, the
use of the NMPI interface). However, most of the material here is still
relevant and provides a good guide to coding for MPICH.

## General Principles

Coding standards are intended to make a large project more successful
and enjoyable. The basic goals and some of their implications are list below.

### Easy To Maintain

The code needs to be readable by people other than the author. Printed
formats should avoid ugly line wraps, and the code should pass strict
compilation checks. The C preprocessor should be used sparingly and
carefully. Make sure that you develop, document, and discuss a design
before and while implementing it. Use the wiki to make it easy to share
the design with the group and with others.

### Robust And Reliable

Avoid making old errors (for example, avoid using routines that don't
check for buffer overflows). Avoid portability problems. Do not assume
that the whole world is gcc and gnumake; refer to the standards rather
than what you find on any one system.

### High Performance

Code that is critical to performance should be designed and made to be run fast.
Use the language features such as `const` and [`restrict`](#using-the-restrict-qualifier)
that can help the compiler generate good code. See also [Performance Tricks](#performance-tricks "wikilink").

### Flexible And Adaptable

Design a clean interface. Use information-hiding, keep the scope of
internal information as narrow as possible and  within a single file when it
makes sense.

### Easy To Navigate

There must be design documents. An overview document can be invaluable
in understanding the organization of the code. Use comments within the
code to remind the reader about particular issues. As one example,
anywhere memory is allocated there should be a comment explaining where
the memory is freed.

### Other General Guidelines

In addition, these general guidelines can help improve the code:

1.  Make sure that the code is well-documented. Each function should
    have a brief description of its function and parameters; for
    less-used functions, it is helpful to indicate what functions and/or
    files may call this function. Modules should contain and/or point to
    design documentation.
2.  Make the interface between modules and other units of the code
    small. Do not let the code see all of the internals of structures
    (see information-hiding above).
3.  Avoid "pick and put" programming. Such programming is impossible to
    maintain (how do you know that all instances remain the same after
    bug fixes?) and usually indicates a flaw in the project design,
    since duplicate functionality should be captured within a
    (documented and tested\!) function.
4.  Use type qualifiers to indicate the intended use of parameters and
    variables. For example, an input value that is only read should be
    declared `const`.
5.  Avoid overuse of `ifdef`. Depending on the issue, the following
    alternatives can be used:
    - Selecting different implementations for an operation. Instead of
      an `ifdef` chain, define a function interface for that operation
      and then invoke that function. Function pointers can be used as
      a way to allow multiple choices of implementation within a
      single executable; otherwise, just use a function call instead
      of `ifdefs`
    - Including header files. `Ifdefs` should only be used around
      optional header files, such as `stdlib.h` (optional in the sense
      that they primarily provide function prototypes and their
      omission will not cause the compilation to fail). If some part
      of the code depends on the existence of the header file, then
      instead of testing for the header file with


      ``` c
      #ifdef HAVE_FOO_H
      #include
      #endif
      ```

      (and similar lines for each header file used), check for the
      necessary prerequisites in one place, then define a single
      `ifdef` check for the block of code that will use those header
      files. Something like this might be appropriate


      ``` c
      #if defined(HAVE_FOO_H) && defined(HAVE_BAR_H) && defined(HAVE_FOOBAR_FUNC)
      #define USE_FOOBAR
      #endif
      ...
      #if defined(USE_FOOBAR)
      #include
      #include
      ...
      #endif
      ```

      Note that the use of `USE_FOOBAR` is also an example of using a
      single, common test, rather than attempting to repeat the same
      complex test in multiple places within the code

6.  Avoid gratuitous changes to the code. Do not reformat the code
    unless is violates some coding rule. Note that some tools, such as
    the document generator, expect and require certain formatting which
    is consistent with our coding standards.
7.  Understand the consequences of changes to the design before you make
    them. As an example, the original design for the error code
    generation system used the standard `printf` formats to allow the
    use of a gcc extension to check for consistent format strings in the
    error reporting code. This is particularly important because it is
    very difficult to check that the format strings are consistent
    through other means. A change to the error reporting system threw
    out this test without providing a replacement, making the current
    code unreliable. Alternative design choices were and are available
    that would have preserved the ability to exploit the gcc
    compile-time checks.
8.  Do not ignore error returns from functions. Check everything (a
    major flaw in gcc is that it has no warning mode that flags ignored
    function return values). Do not assume that buffers "will be long
    enough". In the case of system calls, make sure that you test for
    `EINTR` and handle that case appropriately.

As of this writing, the current code does not follow these principles. I
hope that the team will embrace (and improve) these standards and help
move the code to something of which we can be proud.

## Basic Source File Structure

All source files follow a similar structure. The file `maint/template.c`
can be used as an example. Important parts of this file are:

### Style header

This sets the style for the file and sets a common indentation amount
for C and C++ programs. By setting a common indentation amount, we avoid
having CVS record changes to a file that are only changes in indentation
amount. In addition, the style header is important for C++ header files
so that the style checker will know that the file is C++ instead of C.

### Copyright

This ensures that everyone knows that we wrote this file and that we did
not take it from someone else. This is very important groups that are
using MPICH as the basis of their own MPI implementation (it makes it
clear who owns the source and under what license the file is made
available).

### Header files

Header files should have their contents wrapped within an `ifndef` of
the form

``` c
 #ifndef FILENAME_H_INCLUDED
 #define FILENAME_H_INCLUDED
 ...
 #endif
```

where the `FILENAME` is the name of the file, in upper case. The exact
form of this is important because the coding style checker expects to
see this particular form (the name format is chosen to reduce the chance
that the name will conflict with a name used in a system header file).

In addition, lines within files should be limited to 80 columns; this
fits many common displays and allows most users to place two pages
side-by-side. (An absolute maximum is 100 characters; more than this,
and printing with `enscript` can become ugly.)

Do not "reflow" text or code. This produces an unnecessary change event
for CVS and can break some tools. For example, unnecessary and
gratuitous changes to the function declarations for the MPI routines
caused the code that generates the manual pages for the MPI routines to
generate unattractive and in some cases unreadable manual pages.

### Code Cleanup Script

This [script](https://github.com/pmodels/mpich/blob/master/maint/code-cleanup.bash)
can be used to cleanup whitespace, comments, and line-wrapping in
existing source files. It is a good idea to run it on any newly created
files in the git repo. Note that the script can be found in the `maint`
directory in your working copy of the MPICH repo.

## Routine Names

Routine names should be chosen in a way that (1) is descriptive of their
function and (2) is clearly distinct from any name that may be used by
another runtime library or user code. To achieve this, we have chosen to
reserve a few prefixes for the use of the MPICH implementation.

- MPI Routines and Common Implementation
    - MPI_
    - PMPI_
    - MPIR_
    - MPIU_
    - MPID_
    - MPIDI_
    - mpi_
    - pmpi_

- Process Manager Interface
    - PMI_
    - PMIU_
    - PMII_

- ROMIO Implementation of MPI-IO
    - ADIO_
    - ADIOI_

## Macro Names

Just like routine names, it is important to pick macro names to avoid
possible conflicts with macros defined in system header files.

- Internal Features of the MPICH Implementation
    - MPIR_
    - MPID_
    - MPIU_
    - MPIDU_
    - MPICH_
    - MPIO_

- Fortran Features
    - F77_

- Debugging
    - DEBUG_

- Including a Header
    - _H_INCLUDED (This *suffix* should be used as the test for including a header {`.h`} file)

- Generated By Configure and Configure Macros
    - HAVE_
    - USE_
    - NEEDS_
    - WITH_

In addition to these, the coding style checker recognizes many macro
names that are part of the standard C header files or the Unix header
files. Additional names can be added by editing the file `cppdefines.pl`
used by the style checker.

Some macro names should *never* be used. For example, the C standard
specifies that names beginning with a double underscore belong to the
runtime system and compiler and must not be changed by the user. In
addition, these are private to specific compiler implementations, and
should not be tested within user code.

In environments with gcc (such as Linux), it is tempting to define
either `__USE_xxx` (such as `__USE_MISC`) or `_BSD_SOURCE` in order to
access some element of a system header file that is guarded with one of
these macros. This is a very bad idea and is not portable. The use of a
`__USE_xxx` macro is invalid; the use of the `_BSD_SOURCE` and related
macros assumes a gcc environment and is not portable. Instead, test for
the necessary feature within configure. If it is necessary to compile
with a certain feature set when using gcc (such as `BSD_SOURCE`), then
add this definition at configure time so that a consistent compilation
feature set will be used. Note that other, non-gcc environments will not
use the `_BSD_SOURCE` and related feature macros, so code that depends
on these is not portable.

Macros used semantically like functions should have names that are all
uppercase. This makes it easy to recognize that a statement is a macro
rather than a function call. This is particularly important when
evaluating and understanding performance critical code; every function
invocation is a potential performance bug.

## Using MPI routines within the MPI implementation

**This section is out of date. The nested methods are no longer used. It
is no longer possible to use the NMPI interface to acquire trace
information my exploiting the profiling interface**

A number of the routines that implement MPI wish to use other MPI
functions; for example, the implementation of the MPI collective
functions uses MPI point-to-point functions. However, this must be done
with some care. There are two issues:

1.  Whether the MPI or PMPI routine is used
2.  How are errors handled

To deal with the first issue, routines in the MPI implementation that
call MPI routines should call `NMPI_xxx` instead of `PMPI_xxx` or
`MPI_xxx`. These names are defined in `src/include/nmpi.h`; if you need
a routine that is not defined there, make sure that you add it (in both
MPI and PMPI versions). By default, MPICH will then use the PMPI
routines, but if you configure with the option `--enable-nmpi-as-mpi`,
the MPI versions will be used instead. This can be helpful when using
profiling tools to understand the use of MPI routines within the MPICH
implementation.

The second issue (errors) is handled by indicating that an MPI call is
being made within another MPI routine; this is a *nested* call. All MPI
routines that are *used* within an routine must be surrounded by nest
increment and decrement calls:

```
MPIR_Nest_incr();
...
NMPI_Send( ... );

MPIR_Nest_decr();
```

If `MPICH_DEBUG_NESTING` is defined, additional debugging checks will be
executed by the nest increment and decrement routines to ensure that
they are properly matched; in particular, that a `MPIR_Nest_decr` is
called in the same file in which the matching `MPIR_Nest_incr` was
called.

## Argument Declarations

The MPI standard specifies the calling sequences for the MPI routines.
These specifications were originally "old-style" C and did not make best
use of the C language. As of MPI-3, the calling sequences for MPI
routines have been improved, using `const` and array (`[]`) notation.

For both best performance from optimizing compilers and for clarity in
intent, internal routines should use the following forms for
declarations. In addition, performance-sensitive routines should use
[`restrict`](#using-the-restrict-qualifier).

| Argument Usage                  | Declaration          |
| ------------------------------- | ---------------------|
| input value                     | ` const  a `         |
| input pointer to fixed value    | ` const  *a `        |
| fixed input pointer to value    | ` *const a `         |
| input array                     | ` const  const a[] ` |
| output value                    | ` *a `               |
| output in array                 | ` a[] `              |
| output array                    | ` *a[] `             |

## Global Variables

Global variables should be avoided as much as possible. When they are
needed, they should follow the same naming convention as routines (e.g.,
use one of the common prefixes). In addition, they should be
initialized. On some systems, uninitialized global variables are marked
as so-called "common" symbols. These symbols are not found when an
application is linked with a library that contains them. OS/X (and it is
reported that FreeBSD has the same behavior) uses this convention (this
seems to clearly violate the spirit if not the letter of the C
standard). To avoid problems with these systems, it is best to
initialize all global variables (while it is possible to force gcc, with
the `-fno-common` argument, to fix this, that is not necessarily
portable to other compilers).

## Returning Error Codes

### Error Code Overview

The MPI standard requires integer error codes to be returned from top
level MPI functions. Internally MPICH uses integer error codes to manage
errors and error messages as well. The MPIR_Err_create_code function
is at the core of this functionality and its prototype looks like this:

```
int MPIR_Err_create_code( int lastcode, int fatal, const char fcname[],
                          int line, int error_class, const char generic_msg[],
                          const char specific_msg[], ... )
```

If you were to use the function directly a call would look like this:

```
mpi_errno = MPIR_Err_create_code( mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME,
                                  __LINE__, MPI_ERR_OTHER, "**mpi_attr_put",
                                  "**mpi_attr_put %C %d %p", comm, keyval, attr_value);
```

The bad news is that this is largely ugly, error-prone boilerplate code.
The good news is that there are
[convenience macros](#convenience-macros-and-best-practices).
One important thing to notice about this example is that `mpi_errno` is used
by the function and then assigned the return value of the function. This
is useful because it allows errors to wrap other errors much in the same
way that you might nest exceptions in Java. If `lastcode` is
`MPI_SUCCESS` (0) then the returned error code will be the bottom of the
error stack.

### Function Error Handling Skeleton/Example

``` c
 #undef FUNCNAME
 #define FUNCNAME myfunc
 #undef FCNAME
 #define FUNCNAME MPIDI_QUOTE(FUNCNAME)
 int myfunc(int in_arg1, int in_arg2, int *out_arg1)
 {
     int mpi_errno = MPI_SUCCESS;
     MPIDI_STATE_DECL(MPID_STATE_MYFUNC);

     MPIDI_FUNC_ENTER(MPID_STATE_MYFUNC);

     /* function body */
     MPIU_ERR_CHKANDJUMP(in_arg2 == 0, mpi_errno, MPI_ERR_OTHER, "**dividebyzero");
     *out_arg1 = in_arg1 / in_arg2;

     mpi_errno = MPID_Do_something(*out_arg1);
     if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
     MPIDI_FUNC_EXIT(MPID_STATE_MYFUNC);
     /* this should be the only return statement in the function */
     return mpi_errno;
 fn_fail:
     /* any cleanup on failure occurs here, releasing acquired resources, etc */
     goto fn_exit;
 }
```

There are a few key points to notice here:

  - All but the most trivial functions should have a return type of
    `int` and return an mpi error code. Returning other values should be
    handled via "out parameters".
  - Any function that does any error handling should have
    `fn_exit/fn_fail` labels. They are expected by various macros.
  - There should usually only be one return point from the function. Use
    your best judgement to determine when to ignore this guideline, but
    remember that it can be hard to change a function later to return an
    error code back up the stack from a lower level function.
  - The `fn_fail` label provides an opportunity to cleanup after an
    error but before the function returns. It serves a very similar role
    to a `catch` block in C++ or Java.
  - Returning errors from a lower level function should be handled with
    `MPIU_ERR_POP`. We do this by hand to effectively get exception-like
    handling in a language that does not actually have exception
    handling. The `MPIU_ERR_POP` macro is essential to building the
    stack trace for the final error message.

### Convenience Macros and Best Practices

These macros all expand to an invocation of `MPIR_Err_create_code` but
are much less verbose than using that function directly. The messages
should be message keys from  and friends. For versions that take
arguments in printf style this should have a general message and a
specific message. The specific message contains format specifiers that
indicate the types of the arguments. See `**argnonpos` in the
aforementioned file for an example. These functions all require that a
`fn_fail` label is defined in the current function.

`MPIU_ERR_SETANDJUMP(error_lvalue, class, message)`

Create an error code with class `class` using the error message in
`message`. Store this error code in `error_lvalue` and jump to the
`fn_fail` label. This is typically invoked as something like this:
`MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem")`

```
MPIU_ERR_SETANDJUMP1(error_lvalue, class, general_message, specific_message, arg1)
MPIU_ERR_SETANDJUMP2(error_lvalue, class, general_message, specific_message, arg1, arg2)
MPIU_ERR_SETANDJUMP3(error_lvalue, class, general_message, specific_message, arg1, arg2, arg3)
MPIU_ERR_SETANDJUMP4(error_lvalue, class, general_message, specific_message, arg1, arg2, arg3, arg4)
```

These are similar to plain `SETANDJUMP` except they take additional
arguments that should be formatted according to the `specific_message`
argument in `snprintf` style. They are intentionally not varargs (`...`)
macros for portability reasons.

`MPIU_ERR_CHKANDJUMP(condition, error_lvalue, class, message)`

Effectively equivalent to: `if ((condition))
MPIU_ERR_SETANDJUMP((error_lvalue), (class), (message));`. Analogous to
`SETANDJUMP[1-4]` are the `CHKANDJUMP[1-4]` macros that take format
string arguments.

`MPIU_ERR_SETANDSTMT(error_lvalue, class, statement, message)`

A generalization of `SETANDJUMP`. Instead of `goto fn_fail` as the
action after creating the error code, `statement` is performed. This is
primarily used to implement the other macros, but you could use it to
break out of a loop or perform some other action. Just don't forget that
the whole reason that you used this macro was as a less verbose way to
create an error code, so make sure that you don't lose it in subsequent
actions. Also, there is a conditional version of this named
`MPIU_ERR_CHKANDSTMT`.

`MPIU_ERR_POP(error_lvalue)`

This simply calls `MPIU_ERR_SETANDJUMP(error_lvalue, MPI_ERR_OTHER,
"**fail")`. This should be used as shown in the skeleton/example in the
section above. The main reason to use it is that it inserts the current
function into the stack trace in the final error message. Simply
returning the error code to the function above will make the current
function "disappear" in the stack trace.

## Checking for Errors

Error checking should be performed on user input values. Internal
routines need not duplicate these checks (for example, once a top-level
MPI routine has verified that an `MPI_Request` handle is a valid handle,
the internal routines such as `MPID_Startall` and
`MPID_Request_release_ref` do not need to check for an invalid handle.

Error checks should either use the macros defined in
`src/include/mpierrs.h` or should be surrounded with

```
#ifdef HAVE_ERROR_CHECKING
...
#endif
```

as illustrated in the sample code template. In addition to allowing the
compile-time generation of the fastest code for debugged, production
applications, this style allows the coverage analyzer to recognize error
checking blocks and to exclude them from reports about code that is not
exercised by the test suites.

## Memory Allocation in MPICH

Memory corruption and leaks of dynamically-allocated memory is a common
and difficult-to-debug problem. To make it easier to test and diagnose
these problems in MPICH, we use a special set of memory allocation
macros. These allow us to use the regular `malloc` etc. routines when
MPICH is used in production and to turn on extensive memory tracing and
arena checking for both debugging and memory leak checks. The macro
names and their corresponding functions are:

```
MPIU_Malloc  - malloc
MPIU_Calloc  - calloc
MPIU_Free    - free
MPIU_Strdup  - strdup
MPIU_Realloc - realloc
```

The definitions are in `src/include/mpimem.h.` The `MPIU_xxx` functions
should be used everywhere; there is no cost when MPICH is built for
normal use and they enable valuable memory usage checking.

As an example of the benefit of using the '`MPIU_Malloc`' set of names,
here is an example that recently occurred. A test failed with a
`SIGSEGV`. Reconfiguring with `--enable-g=mem` and rebuilding (`make;
make install`) and then rerunning the failing test produced the
following:

```
mpiexec -n 2 spaiccreate
[0] Block at address 0x0810e5f8 is corrupted (probably write past end)
[0] Block allocated in cts/software/mpich/src/mpid/ch3/src/mpidi_pg.c[605]
[0]0:Return code = 0, signaled with Segmentation fault
```

From this, is was easy to identify the source of the problem (in this
case, a mis-computation of the buffer size).

A special case of memory allocation is `alloca`. This routine is not
available on all systems (it allocates memory from the stack,
simplifying the handling of memory needed only within a function).
Instead of providing an `MPIU_Alloca` function, MPICH provides a set of
macros to simplify the steps needed to free memory that is allocated
within a routine. These macros will use `alloca` if it is available and
will properly free memory if it is not.

Memory allocated within a routine can either be local, intended for use
only within that routine to be freed before exit, and persistent,
intended to be used after the routine exits. However, even in the case
of persistent memory, in the case of an error, it may be necessary to
free the allocated memory before exiting the routine to avoid a storage
leak. To handle these cases, MPICH provides a set of macros for
allocating one or more chunks of memory and ensuring that they are freed
properly on routine exit (for local memory) or on an error (for both
local and persistent memory).

For persistent memory, the macros are:

  - MPIU_CHKPMEM_DECL(n) - Declaration used within any routine that uses the persistent memory
    routines. The value of `n` is the max number of items that will be
    allocated
  - MPIU_CHKPMEM_MALLOC(pointer, type, nbytes, mpi_errno, name) - Sort of 
    `pointer = (type)MPIU_Malloc( nbytes ); if (!pointer) {message about name; goto fn_fail; }`
  - MPIU_CHKPMEM_COMMIT - Transfer responsibility for freeing any memory on error to someone else
  - MPIU_CHKPMEM_REAP - Free any memory not committed

For local memory:

  - MPIU_CHKLMEM_DECL(n) - Declaration used within any routine that uses the persistent memory
    routines. The value of `n` is the max number of items that will be allocated
  - MPIU_CHKLMEM_MALLOC(pointer, type, nbytes, mpi_errno, name) - Allocate memory; roughly like

    ``` c
    pointer = (type)alloca(nbytes);
    if (!pointer) { 
      message about name
      go to fn_fail;
    }
    ```

  - MPIU_CHKLMEM_FREEALL - Free all local memory

The above routines depend on the presence of a label `fn_fail` to which
these routines will jump if an error, such as a null return from
`MPIU_Malloc`, occurs. There are additional macros that allow you to
jump to a different label or to execute different code on an error; see
the source file for details.

Here is an example sketch of a routine that uses these macros. For an
example within the MPICH code, see `src/mpi/topo/cart_create.c`.

``` c
 int MPIR_foo( int n )
 {
 ... other declarations
 int *foo_ptr, *foo2_ptr;
 double *d1;
 MPIU_CHKPMEM_DECL(2);
 MPIU_CHKLMEM_DECL(1);
 ...
 MPIU_CHKPMEM_MALLOC(foo_ptr,int*,n*sizeof(int),mpi_errno,"foo_ptr");
 MPIU_CHKPMEM_MALLOC(foo2_ptr,int*,n*2*sizeof(int),mpi_errno,"foo2_ptr");
 ...
 MPIU_CHKLMEM_MALLOC(d1,double*,10*sizeof(double),mpi_errno,"d1");
 ...

 /* Free all local memory before a normal exit */
 MPIU_CHKLMEM_FREEALL;
 return mpi_errno;

 /* The code jumps to this label on an error exit */
 fn_fail:
   MPIU_CHKPMEM_REAP;
   MPIU_CHKLMEM_FREEALL;
   ...
```

Rationale for the design:

1.  All memory allocations are checked; failure causes an error return
    (i.e., set `mpi_errno` and goto `fn_fail`).
2.  If multiple memory allocations have been made and a failure occurs
    (in a memory routine or not), all allocated memory that is freed
    unless it is known to be OK (e.g., memory attached to an MPI object
    that is still valid need not be freed, but, for example, if an error
    is detected in creating an MPI Datatype, any memory allocated so far
    must be freed)
3.  Code uniformity and Conciseness - we'd like all errors to be handled
    the same way, and the block of code to be small (ideally one line).

In addition to the above, it is very important to null references to
space that is freed. Failure to do this can create difficult-to-find
bugs; for example, a bug in programs that used `MPI_Comm_disconnect` was
eventually (after several months\!) tracked down to a failure to zero
the pointer to the connection (specifically, the `vc->ch.conn` pointer)
when the connection was freed.

## Marking Source Code for Coverage Testing

MPICH makes use of automated code coverage analysis. To make this
analysis more useful, the source code can be annotated to indicate
blocks of code that should not be flagged as uncovered. Examples of such
code are sections that provide error handling and reporting,
experimental code segments, and code used only for internal debugging.
To avoid "false positives", these should be marked with

```
/* --BEGIN name-- */
 ... code that can be ignored by coverage analyzer ...
/* --END name-- */
```

where `name` is on of the following:

  - DEBUG - Used for blocks of debugging code
  - ERROR HANDLING - Used for blocks of error handling and reporting code
  - EXPERIMENTAL - Used for experimental code
  - USEREXTENSION - Used for code that provides hooks to user-provided extensions,
     and thus are not part of the regular tests.

In addition to these annotations, the coverage analyzer knows about
commonly used error reporting routines and macros; code that matches the
following regular expressions will be treated as if is it surrounded
with an "ERROR HANDLING" block:

```
FUNC_EXIT.*STATE
MPIR_Err_return_
MPIU_ERR_SET
MPIU_ERR_POP
goto\s+fn_fail
fn_fail:
MPIR_Err_create_code
```
In addition, the coverage analyzer recognizes the error checking block

```
#ifdef HAVE_ERROR_CHECKING
...
#endif
```

and will automatically treat code within this block (or to a `#else`) as
being an "ERROR HANDLING" block. These special cases for error handling
and reporting are provided to avoid requiring extra annotations for
error handling and reporting statements.

If you are not sure whether to mark a block or not, it is best to not
add any annotation until after the next coverage analysis is run; then
check the output and decide if an annotation is necessary.

## Alternative string routines

The standard string routines such as `strcpy` are very dangerous because
they do not check that the destination memory is large enough to hold
the result. No good code should use these routines; instead, routines
that ensure that no more data than will fit into the destination memory
should be used. Routines such as `strncpy` are acceptable replacements
for these older routines.

However, the "`n`" versions of the string routines have several of their
own problems. To start with, they often write *exactly* characters;
`strncpy` copies the source string and then pads the destination string
with nulls. In the routine `strncat`, the value of "`n`" is *not* length
of the destination string; instead it is the maximum number of
characters to take from the source string. This requires the user to
first compute the current length of the destination string in order to
ensure that data is not written past the end of the destination string.

MPICH provides several alternatives to the string routines that address
these issues. They are

  - `MPIU_Strncpy` - Replaces `strncpy`, with the same arguments
  - `MPIU_Strnapp` - Replaces `strncat`, except `n` is the length of the destination
    string.
  - `MPIU_Snprintf` - A partial replacement for `snprintf` and `sprintf`, it handles
    format specifiers `d`, `p`, `s`, and `x`. It is provided for  portability reasons, 
    as not all systems at this writing provide `snprintf`. 
    (MPICH will define `MPIU_Snprintf` as `snprintf` on systems where snprintf exists).

The implementation of these routines may be found in
`src/util/mem/safestr.c`.

## Printing messages

In order to maintain consistency over message formats, allow for
internationalization, and to provide for systems without a notion of
standard output, messages to the user should not be printed with the
usual puts or printf functions. Instead, there are a set of MPIU
functions in `src/util/msgs/msgprint.c` that should be used. These
functions are

  - `MPIU_Usage_printf` - Used for "usage" messages
  - `MPIU_Error_printf` - Used for error messages (user errors)
  - `MPIU_Internal_error_printf` - Used for internal error messages 
    (errors by the implementation not the user)
  - `MPIU_Internal_sys_error_printf` - Used for internal error messages; this form 
    includes the error message from a system routine (typically accessed with strerror).
  - `MPIU_Msg_printf` - Other, non-error, message

The calling sequence of these routines is usually the same as that for
`printf`. See the source file for details.

By using these routines, it is relatively easy to automatically extract
the messages and set them up for internationalization. In addition, it
is easy to extract and categorize the messages by intent.

## Using asserts in MPICH

It is often helpful for debugging to add "asserts" into code. The assert
is defined in `/usr/include/assert.h`, and can be turned into a no-op by
defining `NDEBUG`. If the assertion fails, the assert macros calls
abort.

In the case of MPICH programs, we prefer to call `MPID_Abort` because
this can help ensure that any persistent resources, such as System V
memory segments, are freed before the process exits. The `MPID_Abort`
can also help the process manager to shut down the other processes.
Finally, the use of our own macro for asserts gives us extra freedom in
determining whether the asserts are enabled or not.

There are two `MPIU` assert macros that should be used instead of the C
library `assert`:

  - MPIU_Assert - Similar to `assert`, it is present only if `NDEBUG` is not defined
    and HAVE_ERROR_CHECKING is defined.
  - MPIU_Assertp - Similar to `assert`, but always defined. This can be used for cases
    where the MPICH implementation cannot safely continue if the  assertion is false.

## Performance Tricks

Performance-critical code follows slightly different rules from other
code. For example, it is very important to use `const`, `restrict`, and
`register` to help the compiler avoid unnecessary loads and stores, and
to make best use of machine registers. Different coding styles can also
be very helpful; see for example the work on PhiPAC.

Some general suggestions:

1.  Reduce register and cache "pressure"
2.  Update all items on the same cache line in one place in the code
3.  Factor routines into smaller parts. Rather than use a branch within
    the routine, try to make each (major) branch into a separate
    routines, and then use those routines directly. Shorter routines are
    also easier on the compiler, which may find it easier to generate
    good instruction schedules. Note, however, that function calls do
    add overhead, so avoid too many function calls. Inlining by the
    compiler can help but is not yet dependable.
4.  Try to keep the argument count for performance-critical routines
    small (three or less). On many systems, these function calls may be
    faster.
5.  Aggregate tests and try to check the common case first

Note that there is a tension between measurement and modeling for
determining whether different performance choices are appropriate. For a
code with the lifetime of MPICH, decisions must not be based solely on
measurements on any one system or even on a variety of systems. Such
measurements can provide information about different approaches, but
must be considered in the context future systems. An example is use of
restrict and const in certain loops - until recently, few compilers even
supported restrict, much less exploited it. However, it was always
correct to make use of restrict where it makes semantic sense.
Similarly, measurements of overhead in the low latency (small message)
limit must be made very carefully (so that high overheads in other,
unoptimized parts of the code don't hide any performance effects and
that limitations or capabilities of current compilers and/or hardware
don't improperly specialize the code to particular system).

You can use the following `configure` tests to check for these C
features:

  - AC_C_CONST - C `const`
  - AC_C_INLINE - C `inline`
  - AC_C_RESTRICT (in newer versions of autoconf; for older versions,
    use PAC_C_RESTRICT) - C `restrict`

## Using the restrict qualifier

Pointers in C are very powerful but that very generality makes it
difficult or impossible for a compiler to perform some very important
optimizations. For example, in optimizing the loop

``` c
 int *a, *b;
 for (i=0; i<n; i++)
     b[i] = a[i];
```

a compiler might want to allow multiple load references to be issued
before the stores, e.g., something like

``` c
 int *a, *b;
 register int a0,a1;
 for (i=0; i<n; i+=2) {
     a0 = a[i];
     a1 = a[i+1];
     b[i] = a0;
     b[i+1] = a1;
 }
```

However, this is not correct C, since it is possible that the pointer b
is the same as `a+1`. The compile thus cannot perform this optimization
without checking that the uses of `a` and `b` do not overlap. In most
cases, the compiler simply assumes that they may overlap, and does not
perform these kinds of memory-hierarchy optimizations.

C90 introduced the qualifier `restrict` that tells the compiler that the
uses of a pointer describe memory that no other pointer will reference.
This is used in the same way as `const`; for example

``` c
 int *restrict a, *restrict b;
 for (i=0; i<n; i++)
     b[i] = a[i];
```

can now be optimized by a high-quality compiler.

## Updating reference counts

Many of the MPI objects have reference count semantics. It is important
that these reference counts are updating atomically, particularly in the
multi-threaded case. To ensure that the updates are atomic, and to make
it easier to keep track of such updates, there are a set of macros for
updating the reference count of an MPI object. These have names such as
`MPIU_Object_add_ref` and `MPIU_Object_release_ref`. However, these
should not be used directly in the code. The reason is that it is
important to be able to search for all of the code that updates the
reference count on a particular object, such as a communicator or
virtual connection, and this isn't easy when the general
`MPIU_Object_xxx` macros are used. Thus, there are a family of macros
for each object type, such as:

```
MPIR_Comm_add_ref
MPIR_Comm_release_ref

MPIDI_VC_add_ref
MPIDI_VC_release_ref
```

These are defined in `mpiimpl.h` or `mpidimpl.h` as appropriate (and the
prefix, MPIR or MPIDI, indicates which include file contains the
definition).

## Adding new mpich control variables

See [Tool interfaces (MPI-T), MPICH parameters and
instrumentation](../misc/Tool_Interfaces.md "wikilink")

## C and POSIX standards

MPICH assumes C99 and POSIX 2001 features for the most part with the
exception of a few later POSIX features. The idea is to support
compilers that follow these rules:

  - For free compilers: they should be provided on some supported
    distribution. We do not support old compilers that are not provided
    on any supported distribution. RHEL is our typical benchmark for
    this because of how slowly it incorporates new software releases.
  - For commercial compilers: they should be supported by the vendor.
    For some compilers, we might make an exception to pick slightly
    older versions than those supported by the vendor because some
    centers tend to provide multiple versions of the compiler, including
    old unsupported ones.

Several compiler vendors (PGI, Intel) do not provide version specific
licenses anymore. If you have a valid license, you can update to the
latest available version of the compiler. In such cases, they typically
only support compilers released in the last 2-3 years. Our model is
somewhat more conservative than this, because in such cases, we would be
trying to support even compiler versions that the vendor itself is not
supporting.

At least the following compilers should be checked before adding a
feature to this list:

  - GCC 4.1
      - First release: 4.1.0 in 2006
      - Latest release in series: 4.1.2 in 2007
      - Other notes: Most distributions have upgraded to much newer
        versions of GCC. RHEL 5 seems to be the only distribution that
        supports GCC 4.1, which hasn't yet reached end of life
        (supported till Nov. 2020)
      - Refs:
          - <https://www.gnu.org/software/gcc/releases.html>
          - <https://gcc.gnu.org/c99status.html>
          - <https://en.wikipedia.org/wiki/Red_Hat_Enterprise_Linux#Version_history>
          - <https://access.redhat.com/solutions/19458>

<!-- end list -->

  - Clang 3.0
      - Release date: 2012
      - Other notes: Clang is a special case where we often only
        consider post Clang-3.0 in our feature-set selection. The
        reasons are two fold. First, clang as a compiler itself is so
        new that the old versions of the compiler are rarely used in
        production environments, especially since they were very buggy.
        Second, it's a free compiler making sticking with an old version
        less useful for users.
      - Ref: <https://en.wikipedia.org/wiki/Clang#Status_history>

<!-- end list -->

  - ICC 10.1
      - Release date: 2007
      - Other notes: Only ICC 14 is supported by Intel, but is a rather
        new compiler (released 2013). We use ICC 10.1 even though it's
        unsupported. This is not based on a comprehensive survey, but
        based on the supercomputer centers that we have access to, this
        seems to be the oldest version around (though we did not find
        any center where this was the only version available).
      - Refs:
          - <https://en.wikipedia.org/wiki/Intel_C%2B%2B_Compiler#History_Since_2003>
          - <https://software.intel.com/en-us/articles/intel-compilers-supported-compiler-versions>

<!-- end list -->

  - PGI 9.0
      - Release date: 2009
      - Other notes: PGI 2013 is the oldest compiler that is supported
        by PGI. We are making an exception in MPICH by checking with PGI
        9.0 (which is an unsupported compiler). This is not based on a
        comprehensive survey, but based on the supercomputer centers
        that we have access to, this seems to be the oldest version
        around (though we did not find any center where this was the
        only version available).
      - Ref: <http://www.pgroup.com/support/download_releases.php>

<!-- end list -->

  - XL 10.1
      - First release: **unknown**
      - End of support: 2015
      - Refs:
          - <http://support.bull.com/ols/product/system/aix/proglang/C>
          - <http://www-01.ibm.com/support/docview.wss?uid=swg21110831>
          - <http://www-01.ibm.com/support/docview.wss?uid=swg21326972>
          - <http://www-01.ibm.com/support/docview.wss?uid=swg21375964>

<!-- end list -->

  - Sun Studio 12 (SS12)
      - Release date: 2007
      - Support from vendor: SS12 is still supported by Oracle, with no
        listed EOL date.
      - Other notes: Sun Studio added almost all C99 features back in
        2003, so the feature check on it is rather redundant, but might
        be useful as a sanity check, nevertheless. However, even though
        the compiler itself had the necessary support, the standard
        headers needed by C99 were only added in Solaris 10 (released in
        2005). Either way, we are safe to assume most C99 features in
        Sun Studio per our guidelines.
      - Refs:
          - <http://www.oracle.com/technetwork/systems/ccompare-141326.html>
          - <http://www.oracle.com/technetwork/server-storage/solarisstudio/training/index-jsp-141991.html>
          - <https://en.wikipedia.org/wiki/Oracle_Solaris_Studio#History>
          - <https://docs.oracle.com/cd/E19205-01/819-5265/bjate/index.html>

<!-- end list -->

  - Fujitsu Softune Compiler
      - Need more information on this from the Fujitsu folks. To be
        updated.

**Notes about specific compilers:**

  - The Microsoft Visual Studio compiler is no longer being considered,
    despite its notorious lack of C99 support. This is because there are
    only two derivatives of MPICH that are built on Windows. The first
    is Intel MPI, which uses the Intel compiler for the builds, and
    hence does not require any support from the visual studio compiler.
    The second is Microsoft MPI, which has not upgraded to the newer
    versions of MPICH for more than 10 years.

**Current Accepted C99 and POSIX 2001 Features in MPICH:**

The following C99 and POSIX 2001 features are assumed to exist in MPICH.
We check for these features in configure.ac and abort if any of them is
not available.

  - [`__VA_ARGS__`](https://en.cppreference.com/w/c/preprocessor/replace)
  - [Fixed-width integer
    types](https://en.cppreference.com/w/c/types/integer)
  - [Pointer-sized
    integers](https://en.cppreference.com/w/c/types/integer) (intptr_t,
    uintptr_t)
  - [Inline functions](https://en.cppreference.com/w/c/language/inline)
  - [Intermingled declarations and
    code](https://en.cppreference.com/w/c/language/declarations)

Note that MPICH **requires** C99 types which are technically optional
(e.g. `uint64_t, uintptr_t`). Platforms that do not provide these types
are not supported.
