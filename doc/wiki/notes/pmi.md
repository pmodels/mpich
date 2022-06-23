# Process Manager Interface (PMI)

## Introduction

MPI needs the services of a process manager. Many MPI implementations include
their own process managers. For greatest flexibility and usability, an MPI
program should work with any process manager, including third-party process
managers (e.g., PBS). MPICH uses a "Process Manager Interface" (PMI) to
separate the process management functions from the MPI implementation. PMI
should be scalable in design and functional in specification. MPICH executables
can be handled by different process managers without recompiling or relinking.
MPICH includes multiple implementations. We are in discussions with vendors to
support PMI directly.

## Process Group Functions

This section describes the functions that provide the basic process management
functions in PMI. These include routines to initialize and finalize PMI,
routines to get information about a parallel program (such as size and rank),
and inquire about the topology of the parallel computer (specifically, whether
it is a cluster of SMPs, and if so, which processes share the same node).

It should be easy to implement the routines in this section in most process
managers. 

- `PMI_Init`
- `PMI_Initialized`
- `PMI_Finalize`
- `PMI_Get_rank`
- `PMI_Get_size`
- `PMI_Get_universe_size`
- `PMI_Get_id`
- `PMI_Get_kvs_domain_id`
- `PMI_Get_id_length_max`
- `PMI_Get_clique_size`
- `PMI_Get_clique_ranks`
- `PMI_Barrier`
- `PMI_Abort`

## Keyval Space Functions

This section describes the functions to provide a way for processes in a
parallel program to exchange information. This information may be used, for
example, to discover the host and port of another process when TCP is used for
communication between those processes. These routines are included as part of
the PMI because they are part of process management in a parallel setting and
because a process manager is well-placed to provide these services. However,
a PMI implementation, particularly one built on top of an existing process
manager, may choose to use a separate service to support these functions. For
example, a separate process that provides a well-known host and port could be
contacted by any process requiring these keyval space services.

- `PMI_KVS_Get_my_name`
- `PMI_KVS_Get`
- `PMI_KVS_Put`
- `PMI_KVS_Commit`
- `PMI_KVS_Create`
- `PMI_KVS_Destroy`
- `PMI_KVS_Get_name_length_max`
- `PMI_KVS_Get_key_length_max`
- `PMI_KVS_Get_value_length_max`
- `PMI_KVS_Iter_first`
- `PMI_KVS_Iter_next`

## Spawn Function

This section describes the functions that are used to create new processes and
to make them visible to the other processes in a parallel job.  

- `PMI_keyval_t`
- `PMI_Spawn_multiple`
- `PMI_Parse_option`
- `PMI_Args_to_keyval`
- `PMI_Free_keyvals`
- `PMI_Get_options`

## External Interface Functions

- `PMI_Publish_name`
- `PMI_Unpublish_name`
- `PMI_Lookup_name`

## PMI Constants
- `PMI_CONSTANTS`
