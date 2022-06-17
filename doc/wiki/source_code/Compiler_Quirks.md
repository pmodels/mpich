This page serves to document various bugs, quirks, extensions, and other
non-standards-conforming behavior of various compilers on various
platforms. It also documents whether or not the compiler supports some
or all of C99.

## GCC

Supports most of the actually useful pieces of C99 as far back as
GCC-3.0. Support is better in newer versions, although still not 100%
complete. See [the compatibility
page](http://gcc.gnu.org/c99status.html) for more information, including
supported features in versions as far back as 3.0.

### v4.3

Seems to be stricter about some warnings than previous versions of gcc.
*Need to elaborate on this.*

## GNU ld linker

### v2.15.92.0.2 20040927

Seems to have a bug where the -O2 default optimization (and even -O1)
fails.

## Intel Compiler

Reference: [official support
list](https://software.intel.com/en-us/articles/c99-support-in-intel-c-compiler).

Appears to support many C99 features at least as far back as v8,
most/all are definitely supported in v10 and later. In particular,
__VA_ARGS__ and snprintf are supported.

icc needs "`-diag-error 147`" in order to make `PAC_FUNC_NEEDS_DECL`
work correctly as of release 1.4.1p1.

## IBM XL Compiler (xlc)

### v9.0 - v13.x

Will display its man page when passed an option it doesn't understand,
including `--version`, `-v`, `-V`, and `--help`. This can lead to very
large `config.log` files when configure probes for the compiler type and
version information.

## Sun Studio Compiler

The Sun Studio 8
[documentation](http://docs.sun.com/source/817-5064/c99.app.html) claims
support for most relevant C99 features, such as variable argument list
macros. The current version as of this writing is Sun Studio 12 and it
claims to have full C99 support.

### Sun C 5.9 SunOS_sparc

Issues a warning when an invalid inline assembly instruction is given,
but finishes compiling (and reports success) omitting the assembly
instructions it couldn't understand. This means we really can't use
inline assembly instructions with this compiler because configure can't
determine which assembly instructions are valid.

## Microsoft Visual C Compiler

Seems the most useful information is in this [blog
post](https://herbsutter.com/2012/05/03/reader-qa-what-about-vc-and-c99/).
Officially, Microsoft C compiler only supports C90 (which is essentially
the same as C89). It has some C99 or even C11 support, but there is no
plan for full C99 support.

Testing with VC++ 2010's `cl` (32-bit, version 16.00.xxxxx.xx):

  - \<stdint.h\> is supported.
  - \<stdbool.h\> and \<complex.h\> are not supported.
  - `__VA_ARGS__` is supported.
  - Declaring variables in the middle of a block is not supported.
  - `//` inline comment is supported.

## Portland Group Compiler

Appears to support C99 in some form at least as far back as v8 of the
compiler (has a -c99 switch). The docs for v9 claim *full* C99 support.

### v8.0.5

Supports gcc-style inline assembly in general, but is much touchier
about which register constraints are supported in which positions. If
you are ever writing inline assembly for PGI, be careful and see mpich
tickets  and  for the final resolution on these issues.

For inline assembly, pgcc interprets output parameters with the the "m"
constraint differently if the parameter is a pointer or an int. I filed
a ticket with PGI, and they said it'll be fixed in 9.0-1.

## HP aCC Compiler (HP-UX)

(based on [docs in a link that may not stay
accessible](http://h21007.www2.hp.com/portal/site/dspp/menuitem.863c3e4cbcdc3f3515b49c108973a801/?ciid=4b080f1bace021100f1bace02110275d6e10RCRD))

### v5

Some C99 support, although exactly what is available isn't clear from
the documentation and we don't have access to this compiler.

### v6

Adds support for

  - Flexible array member
  - Designated initializers
  - Empty macro arguments

## Pathscale Compiler

This compiler has been owned by Qlogic, SiCortex, and now Cray over the
years. It uses a front-end based on GCC so it generally has the same C99
compliance as the version of GCC on which your particular version is
based. Because GCC has had usable C99 support for quite a while now,
most versions of the Pathscale C compiler should also contain usable C99
support.