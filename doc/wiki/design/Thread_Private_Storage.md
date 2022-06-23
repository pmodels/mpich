# Thread Private Storage

While the MPI standard is fairly careful to be thread-safe, and in
particular to avoid the need for thread-private state, there are a few
places where the MPICH implementation may need to storage that is
specific to each thread. These places include

- Error return from predefined MPI reduction operations
- Nesting count used to ensure that `MPI_ERRORS_RETURN` is used when
  an MPI routine is used internally (e.g., if a routine in MPICH calls
  `MPI_Send`, in case of an error, `MPI_Send` should return and not
  invoke the error handler, since we will want the calling routine to
  handle the error).
- Performance counters

These are only needed in a relatively few places, and not in most of the
performance-critical paths.

Unfortunately, access to thread-private (or thread-specific) storage is
not easy in C or C++, since the compiler knows nothing about threads. At
least one compiler, the C compiler for the NEC SX-4, provides a pragma
to mark a variable as thread-private, making it somewhat easier to make
use of thread-private data. In most cases, however, it is necessary to
call a routine provided by the thread package to access thread-private
data; this data is usually identified by an integer id. For performance
reasons, we'll want to make only one call to access the thread-private
storage within a routine (since the thread-private data will have to be
assigned to a variable that is allocated on the stack, since that is the
only thread-private data on which we can depend).

These considerations lead to the following design:

1.  There is one structure, of type `MPICH_PerThread_t` (see
    `src/include/mpiimpl.h`) that contains all thread-private data. The
    thread-private data that we will have the thread package manage is a
    pointer to this storage, which will be allocated on first use.
2.  The id for this storage is in the `MPIR_Process` structure, of type
    `MPIR_PerProcess_t`, which is also defined in
    `src/include/mpiimpl.h`. The id is in the thread_storage field.
3.  In any routine that needs access to the thread-private data, the
    following macros are used. These are macros so that they can be
    rendered into null statements when single-threaded versions of MPICH
    are built.

    - `MPIU_THREADPRIV_DECL` - Used the declare the variables needed to
      access the thread-private storage. To enable a check that the
      thread-private storage has been set, this pointer is initialized to zero.
    - `MPIU_THREADPRIV_GET` - Used to acquire the (pointer to) the
      thread-private structure. This must be called before any of the
      routines that may access the thread-private storage.
    - `MPIU_THREADPRIV_FIELD` - Use to access a field within the per-thread
      instance of `MPICH_PerThread_t`. This is used by the implementations of the
      "nest" macros, the error returns for the collective operations, and
      the performance statistics collection. This macro allows the
      single-threaded code to use a statically allocated `MPIR_Thread`
      variable while the multi-threaded code must use a pointer to the
      thread-private storage:

```
#ifdef MPICH_MULTI_THREADED
#define MPIU_THREADPRIV_FIELD(name) MPIR_Thread->name
#else
#define MPIU_THREADPRIV_FIELD(name) MPIR_Thread.name
#endif
```

      This ensures the best performance in the single threaded case by
      avoiding the extra indirection that is unnecessary in that case.
    - `MPIU_THREADPRIV_INITKEY` - This macro is used to initialize the key
      that is used to identify the thread-private storage. This macro must
      be invoked early in `MPI_Init` (or `MPI_Init_thread`) to ensure that
      the key is available to all subsequently created threads.
    - `MPIU_THREADPRIV_INIT` - This macro is used to allocate and initialize
      the thread-private storage. It is used within the definition of
      `MPIU_THREADPRIV_GET` to handle the first use of thread-private storage
      within a thread and in the `MPI_Init` routine to make sure that the storage
      is allocated for the master thread.
    - `MPE_Thread_tls_t` - The type of the key for the thread-private storage.
      It may be initialized to the value `MPE_THREAD_TLS_T_NULL`.

A special routine, `MPIR_GetPerThread`, returns as its argument a
pointer to the thread-private storage for `MPIR_Thread`.

## An Alternative Proposal for Thread-Private Storage

Thread-private data is needed for some global state (such as the error
handler to be used in a thread) or by performance counters. In POSIX and
many other thread libraries, there are routines to access thread-private
data. Some compilers allow variables to be defined as thread-private -
the compiler inserts enough code to make this thread private. This
proposal is intended to replace the form described above to permit the
use of compiler-support thread-private variables.

The GCC form for thread-private variables is:

```
__thread <type> name;
```

e.g.,

```
__thread int errno;
```
as a global (or file-scoped) variable.

Where a library is used, there are the following declarations in MPICH:

```
MPID_Thread_tls_t <keyname>
```

a **local** (stack) variable for the thread private variable.

One way to unify these is to make all thread private variables pointers.
Then we can do something like the following (this is simplified, e.g.,
there is no error handling):

```
#if HAVE__THREAD
#define MPID_Threadpriv __thread void *
#define MPID_ThreadprivGet( name, localpointer ) localpointer = name
#define MPID_ThreadprivInit( name, size ) name = (void *)calloc(size)
#else
#define MPID_Threadpriv MPID_Thread_tls_t
#define MPID_ThreadprivGet( name, localpointer ) \
    MPID_Thread_tls_get( name, &localpointer )
#define MPID_ThreadprivInit( name, size ) \
    {void * _tmp = (void *)calloc(size);\
    MPID_Thread_tls_create( MPIR_FreeTLSSpace, &name,_tmp );\
    MPID_Thread_tls_set( name, _tmp );}
#endif
```

(The MPIR_FreeTLSSpace is used to free the allocated space when the
thread exits.)

An even better approach is to integrate this approach with the use of
thread-private structs, as used in MPICH for most of the thread-private
data.
