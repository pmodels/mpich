# CH4 Namespace Convention

This page discusses how namespaces should be used in CH4.

Terms in this note:
    1. `Component`: A set of data structures and/or functions that related to a
       particular functionality.

## Namespaces
- `MPID_` - This is the abstract device interface (ADI). Usually, these names
  are linked to their corresponding implementations in the `MPIDI_` namespace.

- `MPIDI_` - This is the namespace for internal functionalities of CH4.
  Functionalities that are specific for CH4 should live in here. For example,
  `WorkQ` and `rankmap` as they are independent from netmod/shmmod/AM.

  Any function that calls the corresponding netmod/shmmod function or
  arbitrates between netmod and shmmod should also live here.

- `MPIDIU_` - This is the namespace for utility functionalities. If a function,
  a data structure, or a component that provide common functionalities used by
  CH4, generic, netmod, or shmmod. Many of the functions that fit in here have
  the appearance of a helper function.

  Example: the buf pool management, symmetric heap allocator, threading
  and locking, AV table manager.

  Anything that lives in `MPIDIU_` should not be implemented by the  AM 
  protocol. Anything lives in `MPIDIU_` should not call `MPIDI_NM_`,
  `MPIDI_SHM_`, or `MPIDIG_` functions. For example, a "helper" that arbitrates
  between netmod and shmmod should always lives in `MPIDI_`.

- `MPIDIG_` - This is the namespace for the CH4 active message (AM)
  implementation which consists of components for the AM communication protocol
  for pt2pt and RMA. Note that any function that starts with the `MPIDIG_mpi_`
  prefix is the AM implementation of the corresponding MPI call.

  Since the generic functionalities provides the fallback implementation
  for any ADI that is not implemented by a netmod or shmmod, the functions in
  `MPIDIG_` can be called in netmod and shmmod. Similarly, the netmod and
  shmmod can access the data structures in the `MPIDIG_` namespace through the
  accessors (e.g. `MPIDIG_REQUEST()`, `MPIDIG_WIN()`, etc.).

- `netmod` - Each netmod should have its own namespace with the prefix
  `MPIDI_{NM name}_`. The functionalities defined in one netmod should be used
  only in that netmod.

- `shmmod` - Each shmmod should have its own namespace with the prefix
  `MPIDI_{SHM name}_`. The functionalities defined in one shmmod should be used
  only in that shmmod.
