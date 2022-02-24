# Reference Counting

This page documents the mechanisms and rationale behind reference
counting in MPICH. In particular, this document is concerned about
reference counting request objects. There is 
[another document](Process_Groups_and_Virtual_Connections.md) about
reference counting communicators, VCs, VCRTs, and PGs.

## Current Mechanisms

### Core Mechanisms

``` c
MPIU_Object_add_ref(objptr_)
MPIU_Object_release_ref(objptr_, inuse_ptr_)
MPIU_Object_set_ref(objptr_, val_)
MPIU_Object_get_ref(objptr_)

/* simple, non-atomic definition of MPIU_OBJECT_HEADER */
#define MPIU_OBJECT_HEADER           \
    int handle;                      \
    MPIU_Handle_ref_count ref_count/*semicolon intentionally omitted*/

/* other relevant macros */
MPIU_Object_add_ref_always(objptr_)
MPIU_Object_release_ref_always(objptr_, inuse_ptr_)
```

### Type-Specific Mechanisms

Most object types have macros or functions that wrap the above macros in
order to permit interception when debugging and to add type-specific
actions. However, MPIU_Object_set_ref is almost always used directly
without a wrapper. All of the above core macros clearly log the pointer,
type, storage area (BUILTIN/DIRECT/INDIRECT), and current reference
count on every refcount operation via the 
[standard MPICH debug logging routines](Debug_Event_Logging.md). 
The debug logging mechanism (with the HANDLE and REFCOUNT classes enabled) 
really helps in debugging reference counting problems.

``` c
/* requests */
MPID_Request_release(req_) /* calls _release_ref and frees the object if !inuse */
MPIDI_CH3U_Request_release(req_) /* calls MPID_Request_release if the completion counter is 0 */
MPIDI_CH3_Request_add_ref(req_) /* totally unused and not implemented */

/* TODO document other object types here */
```

## Current Semantics of Reference Counting

Requests that are created by `MPIDI_Request_create_sreq` and
`MPIDI_Request_create_rreq` are initialized to a reference count of
**2**. Those created by `MPID_Request_create` (very few are) are
initialized to **1**.

**Q:** When are references to requests added?

**Q:** When are references to requests released?

## Future Reference Counting Changes
