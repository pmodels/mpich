# Tool Interfaces, MPICH Parameters, And Instrumentation

This page describes the design of the MPI Tool (MPI_T) Information
Interfaces in MPI-3. MPI-T provides a set of interfaces for users to
list, query, read and possibly write variables internal to an MPI
implementation. Each such variable represents a particular property,
setting or performance measurement from within the MPI implementation.
MPI_T classifies the variables into two parts: control variables and
performance variables. Control variables correspond to the current MPICH
parameters, through which MPICH tunes its configuration. Performance
variables correspond to the current MPICH internal instrumentation
variables, through which MPICH understands its performance.

## MPI_T Basics

Through MPI_T, a user can, for control variables (cvar),
\*Get the number of cvars by MPI_T_cvar_get_num();

  - Get attributes of each cvar, which include its name, verbosity,
    datatype, description, bind and scope;
  - Allocate a handle for a cvar;
  - Read / write a cvar through its handle.

For performance variables (pvar),

  - Get the number of cvars by MPI_T_pvar_get_num();
  - Get attributes of each pvar, which include its name, verbosity,
    datatype, description, bind, class;
  - Create a session so that accesses to pvars in different sessions
    won't conflict;
  - Allocate a handle for pvar in a specific session;
  - Start / stop / read / write / reset / readreset a pvar through its
    handle.

For cvars and pvars,

  - Know their categorization, i.e., how an MPI implementation
    categorizes its variables, which category contains which variables
    and sub-categories.

## Design Requirements

We wish to have a framework through which components of MPICH can add
their parameters and instrumentation uniformly. And it is easy to expose
variables to MPI-T interfaces and it is efficient to access variables
through MPI-T interfaces.

## Old MPICH Parameter and MPI_T Control Variable Design

The document [Parameters_in_MPICH](Parameters_in_MPICH "wikilink")
describes requirements and a potential design of MPICH parameters.
However, the current MPICH code doesn't follow this design. Currently,
all MPICH parameters are declared in mpich/src/util/param/params.yml
[1](http://git.mpich.org/mpich.git/blob/0f1067da5ffe74f6288f2767fe701d82a85ce99c:/src/util/param/params.yml#l48)
in a markup language, which then is parsed and the results are dumped
into two files: mpich_param_vals.h/c. The main data structure is a
static array *MPIR_Param_params*\[\], which is initialized to hold
info for each parameter, such as its name, data type and a pointer to
the parameter. The current code connects a cvar handle to its
corresponding element in *MPIR_Param_params*\[\], which makes MPI-T
cvar implementation straightforward.

**Problems of the Old MPI-T cvar design**

  - cvars are statically defined, thus not supporting dynamically adding
    / removing cvars through dynamic loading libraries.

<!-- end list -->

  -
    \=\> It seems this ability is not needed in MPICH.
    \=\> One place that it may come up (and does in other
    implementations of MPI) is in the dynamic loading of communication
    or file I/O code; for example, if netmods could be loaded
    dynamically or if ADIO device implementations where loaded depending
    on the file system accesses. Having the option to support these
    would be good but not essential.

<!-- end list -->

  - Do not support MPI object binding.

<!-- end list -->

  -
    \=\> Currently, all cvars have attribute MPI_T_BIND_NO_OBJECT.
    Do we want to have such object-binding cvars?
    \=\> Interestingly enough, we do have a set of control vars that
    does have a binding: the option for functions to replace the
    collective and topology routines. These are not available through
    the parameter interface, but could be considered developer-level
    control variables.

<!-- end list -->

  - Not very efficient when accessing a cvar due to datatype
    dispatching.

<!-- end list -->

  -
    For example, to read a cvar, the current code does

<!-- end list -->

``` c
int MPIR_T_cvar_read_impl(MPI_T_cvar_handle handle, void *buf)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIR_Param_t *p = handle->p;

    switch (p->default_val.type) {
        case MPIR_PARAM_TYPE_INT:
            {
                int *i_buf = buf;
                *i_buf = *(int *)p->val_p;
            }
            break;
        case MPIR_PARAM_TYPE_DOUBLE:
            {
                double *d_buf = buf;
                *d_buf = *(double *)p->val_p;
            }
            break;
    ...
}
```

  -
    \=\>Promote all cvars to either 64-bit long (MPI_INT64_T,
    MPI_DOUBLE), or 128-bit long (for a range), or anything else (e.g.,
    string), so that we can do very fast dispatching.

<!-- end list -->

  - Do not properly indicate the scope of the variable. This is related
    to deficiencies in the parameter handling implementation, which fail
    to provide this information
  - Provide no category or hierarchy information. Also a deficiency in
    the parameter handling implementation.
  - Descriptions of the control variables rely on correct text in the
    flat file describing the parameters. As there is no obvious link to
    the *use* of the control variable in the source file, these get
    out-of-sync easily. Other, more robust designs for the parameter
    definitions would keep the definitions closer together.

## Old MPICH Instrumentation and MPI_T Performance Variable Design

The document
[Internal_Instrumentation](Internal_Instrumentation "wikilink")
describes MPICH instrumentation requirements and a possible design.
Current code (src/include/mpiinstr.h) loosely implements this design.
Two types of instrumentation are defined: one is timer and the other is
counter. They are heavily used in RMA code,
ch3u_rma_sync.c[2](http://git.mpich.org/mpich.git/blob/0f1067da5ffe74f6288f2767fe701d82a85ce99c:/src/mpid/ch3/src/ch3u_rma_sync.c#l10).
However, current MPICH MPI-T pvar code neither makes use of the
instrumentation interfaces nor exposes those variables. See
MPIDI_CH3U_Recvq_init(void) in ch3u_rma_sync.c
[3](http://git.mpich.org/mpich.git/blob/0f1067da5ffe74f6288f2767fe701d82a85ce99c:/src/mpid/ch3/src/ch3u_recvq.c#l116)
to get a sense of how it works (note that some of the definitions use
counter types that are not valid according to the MPI specification). An
MPI component needs to register its pvars in a table, which in effect is
a dynamic array storing information, such as datatypes and pointers of
all pvars. An MPI_T_pvar_handle points to a pvar, so that accesses to
the pvar can be done.

**Problems of the Old MPI-T pvar design**

  - Do not support the semantics of MPI_T session.

<!-- end list -->

  -
    MPI-T standard provides a construct MPI_T_pvar_session. Users
    need to create a pvar session, then allocate pvar handles in the
    context of a session. Accesses to the same pvar in different
    sessions must not conflict. However, current MPI_T pvar code does
    not have such ability. Accesses are always made to the original
    pvars.

<!-- end list -->

  - Accessing a pvar is inefficient.

<!-- end list -->

  -
    Even though most pvars are a single element, the code uses a memcpy
    to read / write them into / out of user buffers.

## New MPI_T Control Variable Design

To make it easy to sync between descriptions and actual usages, the
centralized parameter (cvar) file is abandoned. Instead, cvar
descriptions are spread out to their corresponding source \*.h or \*c
files as special comment blocks. A script (extractcvars) then parses
source files (under specified directories specified in the script)
to extract the descriptions and generates C code
(src/util/cvar/mpich_cvars.c ) to setup cvars and categories.

A cvar description block has the following format

``` c
 <<In alltoall.c>>
/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===
categories :
   - name : COLLECTIVE
     description : A category for collective communication variables.

cvars:
   - name      : MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE
     category  : COLLECTIVE
     type      : int
     default   : 256
     class     : device
     verbosity : MPI_T_VERBOSITY_USER_BASIC
     scope     : MPI_T_SCOPE_ALL_EQ
     description : >-
        the short message algorithm will be used if the per-destination
        message size (sendcount*size(sendtype)) is <= this value

   - name      : MPIR_CVAR_ALLTOALL_MEDIUM_MSG_SIZE
     category  : COLLECTIVE
     type      : int
     default   : 32768
     class     : device
     verbosity : MPI_T_VERBOSITY_USER_BASIC
     scope     : MPI_T_SCOPE_ALL_EQ
     description : >-
         the medium message algorithm will be used if the per-destination
         message size (sendcount*size(sendtype)) is <= this value and
         larger than ALLTOALL_SHORT_MSG_SIZE
=== END_MPI_T_CVAR_INFO_BLOCK ===
*/
```

Notes:

  - Use BEGIN/END_MPI_T_CVAR_INFO_BLOCK to tag a cvar info block.
    The parsing script depends on these two tags. A file can at most
    have one such block.
  - Enclosed are descriptions for categories or cvars in Perl YML
    format, where indentaattion is used to express hierarchy.
  - Use "categories: \\newline" or "cvars: \\newline" to start a
    section. A block should at least have one section.
  - Use "-(space)" to start a new entry in a section. key-value pairs in
    an entry must be aligned, but their order is irrelevant.
  - Use "\>- \\newline" to start a multiline description.
  - Users should put cvar info blocks close to cvar usage points. cvars
    in multiple files can belong to one category. Putting the category
    description in any file is fine. The script checks if categories
    referenced by cvars exist.
  - The script puts comments in the generated .h file (i.e.,
    src/include/mpich_cvars.h )on where a cvar is found. So if
    duplicate cvar info blocks exist (and triggers compilation error),
    users can easily know where to fix.

Fields of a category:

  - name, description are mandatory.

Fields of a cvar:

  - name, category, type, default, class, verbosity, scope, description,
    class are mandatory.
      - cvar names usually have a prefix MPIR_CVAR_. Suppose there is
        a cvar with name MPIR_CVAR_XXX, during initialization, the
        generate C code will assign the cvar with its default value,
        then try to overwrite the default value by retrieving values of
        environment variables MPICH_XXX, MPIR_PARAM_XXX,
        MPIR_CVAR_XXX if any, in ascending priority order.
      - class can be device or none
      - type can be int, boolean, double, string or range.
          - For boolean type, default values can be true or false
          - For string type, default values can be NULL or a non-quoted
            string
          - For range type, default values are in format low:high, where
            low and high are integer
  - alt-env is optional, which lists alternative env names to set the
    cvar. For example,

<!-- end list -->

``` c
    - name        : MPIR_CVAR_CH3_PORT_RANGE
      alt-env     : MPIR_CVAR_PORTRANGE, MPIR_CVAR_PORT_RANGE
```

## New MPI_T Performance Variable Design

To be consistent, we call instrumentation variables and performance
variables uniformly pvars. Pvars have two clients: MPI tools (Tools) and
MPI components (Components). Interfaces exposed to Tools are defined in
mpi.h with prefix MPI_T_. Interfaces exposed to Components are defined
in mpit.h with prefix MPIR_T_, which are actually macros so that they
have zero overhead when instrumentation is disabled. Data structures
used by MPIR_T are defined in mpit_internal.h. We make two assumptions

1.  Pvars are frequently updated by Components, but much less frequently
    accessed (e.g., read) by Tools.
2.  In real environment, one pvar session (instead of multiple) is the
    most common case.

We should optimize towards these assumptions.

### Pvar Classes

MPI_T defines the following pvar classes: MPI_T_PVAR_CLASS_{STATE,
LEVEL, SIZE, PERCENTAGE, HIGHWATERMARK, LOWWATERMARK, COUNTER,
AGGREGATE, TIMER, GENERIC}. pvars of COUNTER, AGGREGATE or TIMER
represent an aggregate value during a period. Their basic operator is +.
We say these pvars are SUM. Note that instrumentation variables in the
old MPICH code are either COUNTER or TIMER. pvars of HIGHWATERMARK or
LOWWATERMARK represent the max/min value during a period. Their basic
operators are Max/Min. We say these pvars are WATERMARK. These operators
have a big impact on implementation of pvar sessions (see below).

### Pvar Metadata

MPI_T defines a rich set of properties about pvars, including their
name, description, verbosity, class, binding, category and various flags
(continuous, readonly, atomic) . Unlike the old code, we do no store
these metadata along with pvars themselves (for instance, use a big
struct to represent a pvar). The storage of metadata is separately and
dynamically allocated. Because metadata accesses are likely on cold
paths, separating pvars and their metadata can improve locality of
critical paths that access pvars (for example, when Components declare a
bunch of pvars one time, as in the current RMA code).

### Pvar datatypes

<table>
<thead>
<tr class="header">
<th><p>PVAR class</p></th>
<th><p>Types allowed by MPI3</p></th>
<th><p>Types provided by MPICH</p></th>
<th><p>Abbreviation in the code</p></th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td><p>STATE</p></td>
<td><p>MPI_INT</p></td>
<td><p>(all on left)</p></td>
<td><p>INT</p></td>
</tr>
<tr class="even">
<td><p>LEVEL, SIZE, AGGREGATE,<br />
HIGHWATERMARK, LOWWATERMARK</p></td>
<td><p>MPI_UNSIGNED, MPI_UNSIGNED_LONG,<br />
MPI_UNSIGNED_LONG_LONG, MPI_DOUBLE</p></td>
<td><p>(all on left)</p></td>
<td><p>UNSIGNED, ULONG,<br />
ULONG2, DOUBLE</p></td>
</tr>
<tr class="odd">
<td><p>PERCENTAGE</p></td>
<td><p>MPI_DOUBLE</p></td>
<td><p>(all on left)</p></td>
<td><p>DOUBLE</p></td>
</tr>
<tr class="even">
<td><p>COUNTER</p></td>
<td><p>MPI_UNSIGNED, MPI_UNSIGNED_LONG,<br />
MPI_UNSIGNED_LONG_LONG</p></td>
<td><p>(all on left)</p></td>
<td><p>UNSIGNED, ULONG,<br />
ULONG2</p></td>
</tr>
<tr class="odd">
<td><p>TIMER</p></td>
<td><p>MPI_UNSIGNED, MPI_UNSIGNED_LONG,<br />
MPI_UNSIGNED_LONG_LONG, MPI_DOUBLE</p></td>
<td><p>MPI_DOUBLE</p></td>
<td><p>DOUBLE</p></td>
</tr>
</tbody>
</table>

### Pvar Storage

Pvars can be a single element, or can be an array of elements. In the
old mpich instrumentation code (mpiinstr.h), all instrumentation
variables are declared as global variables with fixed sizes. To be more
general, in the new implementation, we extended it with two additions:
1) Support dynamic storage, i.e., pvars can be allocated at run time
with variable sizes. 2) Pvars are not necessarily contiguous. Components
provide callbacks to assemble data and return it to Tools (e.g., when
MPI_T_pvar_read() is called).

  - For statically allocated pvars, provide interfaces to declare them
    as static or extern

<!-- end list -->

``` c
#define MPIR_T_PVAR_INT_STATE_DECL_impl(name_) \
    int PVAR_STATE_##name_;
#define MPIR_T_PVAR_INT_STATE_DECL_STATIC_impl(name_) \
    static int PVAR_STATE_##name_;
#define MPIR_T_PVAR_INT_STATE_DECL_EXTERN_impl(name_) \
    extern int PVAR_STATE_##name_;
```

Components register static pvars using interfaces like

``` c
#define MPIR_T_PVAR_STATE_REGISTER_STATIC_impl(dtype_, name_, initval_, etype_, verb_, bind_, flags_, cat_, desc_)
```

  - For other types of pvars (i.e., dynamic allocated, or accessed by
    callbacks), Components are responsible for memory allocation and
    register them using interfaces like

<!-- end list -->

``` c
#define MPIR_T_PVAR_STATE_REGISTER_DYNAMIC_impl(dtype_, name_, addr_, count_, etype_, verb_,  bind_, flags_, get_value_, get_count_, cat_, desc_)
```

### Pvar Interfaces to Components

Depending on pvar classes, provide different sets of methods to
initialize and operate the pvar. Components should only use these
interfaces.

| PVAR class                  | How to initialize            | Methods                     |
| --------------------------- | ---------------------------- | --------------------------- |
| STATE, SIZE, PERCENTAGE     | Call SET                     | SET, GET, ADDR              |
| LEVEL                       | Call SET                     | SET, GET, INC, DEC,ADDR     |
| COUNTER, AGGREGATE          | Call INIT() to init to zero  | INIT, GET, INC, ADDR        |
| TIMER                       | Call INIT() to init to zero  | INIT, GET, START, END, ADDR |
| HIGHWATERMARK, LOWWATERMARK | Call INIT(val) to set to val | INIT, GET, UPDATE, ADDR     |

### Pvar Handles

MPI_T says "... To avoid collisions with respect to accesses to
performance variables, users of the MPI tool information interface must
first create a session. Subsequent calls that access performance
variables can then be made within the context of this session. Any call
executed in a session must not influence the results in any other
session." "Before using a performance variable, a user must first
allocate a handle ... for the variable". MPI_T also defines
continuousness of a pvar. If a pvar is continuous, it is "always active
and cannot be controlled by the user". These statements basically mean a
pvar can show different values in different sessions, depending on its
continuousness. We may need to cache the value in a pvar handle. We are
going to support these combinations:

  - Non-continuous SUM pvars

<!-- end list -->

  -
    We store in each pvar handle three values of such a pvar, namely,
    accum, offset, current. When a handle is allocated, accum is
    initialized to zero. When a pvar is started, its current value is
    cached in offset. When a pvar is stopped, we read its current value
    into current and do update like "accum += current - offset". When
    reading a stopped pvar, we return accum. When reading a running
    pvar, we return accum + (current - offset). Such design has a low
    overhead and Components can update their pvars very fast, in
    regardless of how many pvar handles are allocated.

<!-- end list -->

  - Non-continuous WATERMARK pvars

<!-- end list -->

  -
    We can not apply above approach here, since there is no offset value
    for Max or Min operations. Instead, we have to design special
    non-continuous watermark pvar types. When a handle of such a pvar is
    allocated, we need to register the handle to the pvar so that
    whenever the pvar changes, we can update values (e.g., accum) in
    their handles accordingly. When a handle is freed, we need to
    unregister it from the pvar. When pvar handles are stopped, we
    should not update values in these handles. This approach is not
    scalable. But we can optimize for the single pvar session cases.

<!-- end list -->

  - Continuous SUM pvars

<!-- end list -->

  -
    Even though a continuous pvar cannot be started or stopped by the
    user, MPI3.0 requires the starting value of such a pvar to be zero.
    It looks as if the pvar is started implicitly at its handle
    allocation time. Depending on the exact time of pvar handle
    allocation, users can observe different values of the pvar in
    different sessions. So the implementation is almost as same as that
    for non-continuous SUM pvars.

<!-- end list -->

  - Continuous WATERMARK pvars

<!-- end list -->

  -
    Similar to continuous SUM pvars, MPI3.0 requires the starting value
    of a watermark is "the current utilization level of the resource at
    the time that the starting value is set". So again, depending on the
    exact time of pvar handle allocation, users can observe different
    values of the pvar in different sessions. Thus the implementation is
    almost as same as that for non-continuous WATERMARK pvars.

<!-- end list -->

  - Continuous pvars of STATE, LEVEL, SIZE, PERCENTAGE

<!-- end list -->

  -
    These pvars does not have "memory" (i.e., no history). They reflect
    the current state of a certain resource. So all sessions will read
    the same value of such pvars. Therefore, caching in pvar handles is
    not needed and the implementation can be simpler. We can also know,
    non-continuous versions of these pvars do not make sense and
    therefore are not supported.

**Memory allocation for caches in pvar handles**: As discussed above,
sometimes we need to store cached values in pvar handles. For
single-element pvars, it is simple. We just reserve fields for them in
pvar handles. For multi-element pvars, we store in pvar handles pointers
to buffers for cached values. But we do not need to separately allocate
buffers. Since when a pvar handle is going to be allocated, its all
information is known, including its bound object if any. So we know
buffer sizes at that point. We can allocate buffers alongside the pvar
handle using one malloc. Consequently, they can be freed in one free().

### Pvar Sessions

Pvar sessions are simply a list of pvar handles. Each time we allocate a
pvar handle, the handle is linked into its session.

[Category:Design Documents](Category:Design_Documents "wikilink")
