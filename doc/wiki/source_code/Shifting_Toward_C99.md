# Shifting Towards C99

## Motivating reasons

- Third party components are shifting to C99.

    - `hwloc` - Required in MPICH and it requires C99.
    - `ucx` - Optional in MPICH and it requires C99.
    - `libfabric` -Optional in MPICH and it requires C99.
    - `ch4` - Requires either ucx or libfabric.

- C99 requirement simplifies portability burden.
    - `stdbool.h`, `stdint.h`, `complex.h`
    - Restrict pointers and inline functions
    - `__VA_ARGS__`
- C99 allows better code readability
    - Declaring variables close to its context
    - Defining loop variables within the loop syntax
- While we are looking at C99, certain features in C11 are also quite
  attractive:
    - Static assertions
    - Anonymous structures and unions

## Potential impact

The main concern of shifting to C99 is that certain platforms with
absent or less-complete C99 compiler support will not be able to build
MPICH.

Reference [compiler quirks](Compiler_Quirks.md) page for the
status of compiler support.

## MPICH specific C99 features

Even as we shifting to C99 requirement, we are not opening the flood
gate to allow all C99 features into the codebase. The full support of
C99 features may put unnecessary restrictions on compilers. By limiting
allowed C99 features, we intend to support as many compilers as we can.
In addition, even when certain feature may not be supported by a
particular compiler, the limited feature allowance may still provide
feasible path for back porting.

The standardized data type support -- stdbool.h, stdint.h, complex.h --
are in. These features can be tested by configure and worked around with
portability libraries; however, with growing code base, inconsistency in
the portablity handling can creep in. Requiring minimum C99 support can
greatly simplify the code.

Mixing variable declarations and code are case dependent. In general, we
prefers to have variables all declared at the top of scope. However, for
temporary variables that are only meaningful over partial code scope, we
would like to have them defined where they are being used.

We generally prefer loop variables defined in the `for-loop` line, as
they will have no context outside the loop.

We still prefer C block comments. C++ style inline comments do not bring
more readability other than save on typing.

## C99 feature compiler matrix

C99 feature list:

  - `inline`
  - `restrict`
  - `__func__`
  - in-code variable declaration
  - for-loop variable declaration
  - single-line comment
  - `bool` in `stdbool.h`
  - `stdint.h`
  - variadic macro
  - compound literal
  - designated initialization
  - flexible last array member in structure
  - `snprintf`

**Important Note:** MPICH requires support for fixed-width integer
types, which are technically optional C99 features. Platforms which
cannot offer these fixed-width types (e.g. `uint64_t, uintptr_t`) are
not supported by MPICH.

Tested compilers (with `-O2` for inline to work):

  - gcc-4.7.2 (debian-7): <span style="color:#2d2">All pass</span>.
    Requires `-std=c99` for `for-loop variable declaration` and
    `restrict`.
  - gcc-7.3.0 (ubuntu-18.04): <span style="color:#2d2">All pass</span>.
  - clang-3.4 (ubuntu-14.04): <span style="color:#2d2">All pass</span>.
    Requires `-std=c99` for `for-loop variable declaration` and
    `restrict`.
  - clang (MacOS Mojave): <span style="color:#2d2">All pass</span>.
  - pgcc (16.9): <span style="color:#2d2">All pass</span>.
  - icc (17.0): <span style="color:#2d2">All pass</span>. Requires
    `-std=c99` for `for-loop variable declaration` and `restrict`.
  - xlc (16.1.1): <span style="color:#2d2">All pass</span>.

Following compilers we do not officially support:

  - Microsoft Visual Studio 2017 (cl ver.19.16):
    <span style="color:#dd2">Mostly pass</span>. `restrict` keyword is
    not recognized.
