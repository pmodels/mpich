# A Name Publishing Interface for MPICH

## Introduction

MPI defines a set of routines for exchanging data with other MPI programs. The
intent of these is to allow MPI applications to exchange the data that is
needed for the `port` argument in the `MPI_Comm_connect` and `MPI_Comm_accept`
functions.  

## The MPI Name Publishing Routines

MPI provides three routines to allow MPI applications to exchange the
`port_name`'s that are used to connect two running MPI applications. These
routines are shown below in a modified C binding. `const` is used to indicate
input arguments and array notation is used instead of pointer notation for
character strings (this follows the MPICH coding recommendations).

```
MPI_Publish_name( const char service_name[], const MPI_Info info, 
                  const char port_name[] )

MPI_Unpublish_name( const char service_name[], 
                    const MPI_Info info, 
                    const char port_name[] )

MPI_Lookup_name( const char service_name[], const MPI_Info info, 
                 char port_name[MPI_MAX_PORT_NAME] )
```

In these routines, the `port_name` is the name returned by `MPI_Open_port`.  
The `service_name` is a name chosen by the applications as the name that all
related applications will use to look up the `port_name`. The `info` argument
is used to pass any necessary information to the name publishing system
(the reason for this will become clear in
[sample implementations](#a-few-possible-implementations)). 

## Why Another Interface?

An alternative to developing another interface is to extend the process
management interface (PMI).  An advantage to this is that the MPI process
already has a connection to the outside world, used by the PMI interface to
implement the PMI `put/get` interface. Thus, an extension to the PMI interface
could support the name publishing routines (an extension is needed because the
MPI name publishing routines are independent, not collective).

To see why another interface is needed, consider the situation in the following
figure. In this figure, two MPI programs are each started by a different
process manager. These two programs wish to connect. For example, one MPI job
may be run on a batch system (the PBS job in this example) and one on an
interactive visualization cluster.

```
                        Name Publisher
   Process Manager        /       \      Process Manager
       (MPD)             /         \          (PBS)
      /    |   \        /           \         /   \
  -------------------------          -----------------------
 |  /      |      \   /    |        | \      /     \        |
 | MPI     MPI     MPI     |        |   MPI       MPI       |
 | PROC    PROC    PROC    |        |   PROC      PROC      |
  -------------------------          -----------------------

Two MPI applications using name publishing with two different process manager.
```

To implement this scenario, it is necessary to separate the name publishing
interface from the process management interface. Of course, in a scenario in
which there are two MPI jobs using the same process manager, the process
manager could provide the name service. The proposed interface allows
implementations use either a separate service or the process manager

## A Proposed Interface

The interface described here provides all the services necessary for 
implementing the MPI name service routines. In order to provide a hook for the
name service implementation to initialize, there are separate initializating
and finalize calls. These can open a connection to a name service for example.
The initialization call returns a handle of type `MPID_NS_Handle` that is used
in all of the other calls. As in other MPI calls, the free routine takes a
pointer to the handle so that the handle can be set to `NULL` when the free
routine succeeds. Each of these routines returns either `MPI_SUCCESS` or a
valid MPI error code. Implementations may either use the MPICH error code
mechanism or, for external packages, use the MPI routines for adding error
codes and classes.

```
int MPID_NS_Create( const MPIR_Info *, MPID_NS_Handle * )
int MPID_NS_Publish( MPID_NS_Handle, const MPIR_Info *,
                     const char service_name[], const char port[] )
int MPID_NS_Lookup( MPID_NS_Handle, const MPIR_Info *,
                    const char service_name[], char port[] )
int MPID_NS_Unpublish( MPID_NS_Handle, const MPIR_Info *,
                       const char service_name[] )
int MPID_NS_Free( MPID_NS_Handle * )
```

These calls make use of the `MPIR_Info` pointer (a pointer to the info object
passed into the MPI name publisher routines) to provide other information that
the name service may require. Predefined `MPI_Info` keys and values for name
service are:

- `NAMEPUB_CONTACT` - Value is a string providing contact information for the 
  name service. This may be an IP address or something else (see below).
- `NAMEPUB_CREDENTIAL` - Value is a string providing credentials for accessing
  the name service. The exact format is specified by the name service. See also
  `NAMEPUB_USER`.
- `NAMEPUB_EXPIRE` - Value is an integer, represented as a string, that
  specifies an expiration time in seconds from the time that a name is
  published.
- `NAMEPUB_IP` - Value is the IP address as a string, usually in 
  `hostname:port` format. This is an alternative to `NAMEPUB_CONTACT`. 
- `NAMEPUB_IP_FROM_ENV` - Value is the name of an environment variable whose
  value has the same meaning as the `NAMEPUB_IP` info key. 
- `NAMEPUB_REFCOUNT` - Value is an integer, represented as a string, giving the
  number of times a value can be looked up before being deleted from the name
  service. Set only on `MPI_Publish_name`.
- `NAMEPUB_USER` - Value is a string giving the user name.

Two of these info parameters make slight changes to the semantics of the name
publishing routines by providing a way to cause names to expire without an
explicit call to `MPI_UNPUBLISH_NAME`. However, the benefit of these in terms
of ensuring that the name service does not become clogged with old names
outweighs the MPI requirement that `MPI_Info` parameters make no changes to the
semantics of the routines.

## A Few Possible Implementations

This section outlines three possible implementations.

### LDAP

The Lightweight Directory Access Protocol (LDAP) is a standard for providing
general name lookup services. Open source implementations of LDAP are
available at many locations. The following sketches the routines from OpenLDAP
that can be used to implement the name interface.

- `MPID_NS_Create` - Use `ldap_open` to access the ldap server
- `MPID_NS_Publish` - Use `ldap_add_s` to insert the name
- `MPID_NS_Lookup` - Use `ldap_search_s`, `ldap_first_entry`, and
  `ldap_get_values` to access the entry
- `MPID_NS_Unpublish` - Use `ldap_modify_s` to remove the name
- `MPID_NS_Free` - Use `ldap_unbind_s` and `ldap_memfree`

### File

If all MPI processes share a single filesystem, a simple implementation of the
name service can use the file system. In this design, each value is placed into
a separate file whose name include the key. This avoids any problems with
concurrent updates to a single file.

- `MPID_NS_Create` - Create a structure to hold the names of all files created.
- `MPID_NS_Publish` - Create a file using a name containing the user, contact
  string, and the key.  The text of the file is the value of the name. Save the
  file name in the name service structure.  Protection is provided by the file
  system.
- `MPID_NS_Unpublish` - Unlink (remote) the file; remove the file name from
  the name service structure.
- `MPID_NS_Lookup` - For the filename using the same rule in publish and read
  the file.
- `MPID_NS_Free` - Remove all of the created files.

Some of the features, such as `NAMEPUB_EXPIRE`, are not supported
by this interface.  The info key `NAMEPUB_CONTACT` can be used to
specify the directory into which the files will be placed.
