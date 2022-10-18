# Process-Management Interface (PMI)

PMI is an interface between MPI processes and process manager. The purpose of
this interface is to provide parallel processes a standard way to communicate
with the process manager in order to figure out the parallel job environment
including how to establish communications with each other.

## Configure PMI in MPICH

### PMI versions - `--with-pmi={pmi1,pmi2,pmix}`

There are currently three varieties of PMI interfaces. PMI1, or just PMI, is
the de-facto interface supported by most process managers and MPI
implementations. It is still the current default interface for MPICH and
hydra. PMI2 is an experiemental interface with a few enhancements including
multi-threading support and separate API for job attributes. PMIx is a new
effort to extend and push PMI beyond its original purpose of bootstrapping
parallel processes.

There is no official standard documents on PMI1 and PMI2. The best references
are the respective header files, [pmi.h](../../../src/pmi/include/pmi.h) and
[pmi2.h](../../../src/pmi/include/pmi2.h).

The Flux team maintains an excellent RFC for PMI1 (that also covers wire protocol):
[Simple Process Manager Interface v1](https://flux-framework.readthedocs.io/projects/flux-rfc/en/latest/spec_13.html).

PMIx is covered by the official [PMIx Standard](https://pmix.github.io/standard).

MPICH can be configured to use any of the PMI interfaces. By default it will
use PMI1, which is our most feature complete and stable implementation.

### PMI library - `--with-pmilib={mpich,install,slurm,cray,pmix}`

MPICH needs to be linked with an appropriate pmi library. The default option is
`--with-pmilib=mpich`, with which, we use an embedded library that is shipped
with MPICH. The option `install` will use the same library from MPICH but
will build `libpmi` separately and link to it. This is useful if swapping
libpmi at runtime is desirable. The option "slurm" will look for libpmi or
libpmi2 from slurm, whose path can be separately specified via
`--with-slurm=path` option. Similarly, the option "cray" looks for libpmi
or libpmi2 from cray. The option "pmix" looks for libpmix.

Some libraries support both PMI1 and PMI2, such as MPICH's libpmi. Some
supports either PMI1 or PMI2 based on whether it is linked to libpmi or
libpmi2. Currently, PMIx is only supported with libpmix.

In development, we are adding basic PMIx support in MPICH's libpmi. Future
versions MPICH may support PMIx without depending on libpmix.

### Legacy/Alias Options

As a convenience and legacy option, user can use `--with-pmi={slurm,cray}`. It
will pick the recommended PMI version according to vendor preference and link
with the approrate Slurm or Cray libpmi.

## PMI usages in MPICH

All PMI usages in MPICH are wrapped in `src/util/mpir_pmi.c`. MPICH is PMI
version agnostic beyond `mpir_pmi`.

## PMI-1 extensions

1. Job attributes via reserved `PMI` key space.

   PMI-1 does not have separate API for job attributes. Job attributes are
   supported using the `PMI_Get` API with preset keynames. Consequently, all
   keynames with `PMI_` prefix are reserved as read-only job attributes.
   `PMI_Put` of a key with `PMI_` prefix will return failure.

   Currently supported job attribute keys are:
   
   * `PMI_process_mapping`

     This is a list of rank-to-node mapping.
     TODO: describe format

   * `PMI_hwloc_xmlfile`

     Process launcher may expose this variable so processes can avoid probing
     hardware independently.
