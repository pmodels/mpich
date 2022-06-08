# Overriding Collective Functions

MPICH has a facility to allow different layers to override collective
functions of a communicator. This page describes the collective override
mechanism.

The communicator struct has a field called `coll_fns`, of type
`MPID_Collops *` (defined in `src/include/mpiimpl.h`), that points to a
table of collective functions. Originally, this pointer could be `NULL`
indicating that the default implementations of all collective functions
should be used. Similarly, a `NULL` function pointer in the table would
indicate that that default implementation of that specific function
should be used. However the current design requires that the `coll_fns`
pointer point to a valid table and that all *nonblocking* collective
function pointers be non-`NULL`. The blocking collective functions can
still be set to `NULL` to and the default implementations of those
functions will be used.\*

This collectives table should be overridden when the communicator is
created in a communicator creation hook that was registered using the
following function:

```
int MPIDI_CH3U_Comm_register_create_hook(int (*hook_fn)(struct MPID_Comm *, void *), void *param);
```

The communicator creation hook takes two parameters: a pointer to the
communicator that is being created and a `void *` parameter that was
passed to `MPIDI_CH3U_Comm_register_create_hook` when the hook was
registered.

Within the communicator creation hook, the collectives table is
overridden by setting the `coll_fns` pointer in the communicator to a
pointer to new table. Unless all collective functions are being
overridden, it's best to copy the previous table into the new table,
then replace the desired function pointers. Along with function pointers
for the collective functions, the table has a reference count field and
a field to store a pointer to the function table being overridden, so
that the previous table can be restored when the communicator is
destroyed.

A function can be registered to be called when the communicator is
destroyed:

```
int MPIDI_CH3U_Comm_register_destroy_hook(int (*hook_fn)(struct MPID_Comm *, void *), void *param);
```

It is important to call the communicator creation hooks for upper layers
before calling the hooks for lower layers. The communicator destruction
hooks should be called in revers order, i.e., the hooks for lower layers
should be called before calling the hooks for the upper layers. This
allows the lower layers to override collective function tables of upper
layers. The communicator destruction function should be used to restore
the `coll_fns` to the previous function pointer, so that when a
destruction hook of a layer is called, `coll_fns` will point to the same
table that was set in that layer's creation hook.

Note that different communicators can use different collective function
tables, so using statically allocated tables is unlikely to work. In the
worst case, a new function table would have to be allocated for each
communicator. We noticed that, assuming the same functions are being
overridden by a layer, the same function table can be assigned to both
communicators if the `coll_fns` pointer assigned by the upper layer on
both communicators is the same. In `ch3i_comm.c` we keep a list of
function tables we've allocated and search the list in the creation hook
for a table that has the same `prev_coll_fns` as the `coll_fns` of the
created communicator. If we find one, we increment the reference count
on that table and assign it to that communicator. Otherwise, we allocate
a new function table, assign it to the communicator and add it to the
table. In the destruction hook, we reset the `coll_fns` pointer to the
`prev_coll_fns` in the table, then decrement the count and free the
table if the reference count reaches zero.

-----

\* When implementing nonblocking collectives, we decided that it the
code was made considerably cleaner by requiring a valid pointer than
checking the pointer before calling the function. However, we did not
want to change the existing code for blocking collectives. Eventually
we'll make this consistent.
