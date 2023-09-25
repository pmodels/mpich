# Process-Management Interface (PMI)

PMI is an interface between MPI processes and process manager. The purpose of
this interface is to provide parallel processes a standard way to communicate
with the process manager in order to figure out the parallel job environment
including how to establish communications with each other.

## PMI versions

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
If more than one PMI interfaces are enabled, user can use environment variable
MPIR_CVAR_PMI_VERSION (with value "1", or "2", or "x") to select an interface
to be used at runtime.

## Configure PMI in MPICH

MPICH needs to be linked with an appropriate pmi library. The default is to
use embedded builtin libpmi. To use an external pmi library, configure with
`--with-pmi1=[path]` or `--with-pmi2=[path]`, or `--with-pmix=[path]`, to
load externally installed `libpmi.so`, or `libpmi2.so`, or `libpmix.so`,
respectively. We expect to find headers (e.g. `pmi.h`) in `path/include` and
the library in `path/lib`.

When external pmi library is linked, builtin libpmi will be disabled. In
special cases when both an external pmi library and builtin libpmi need be
linked, one can use `--with-pmilib=[mpich|install]` to force build the builtin
libpmi. The "install" option will build the built-in libpmi as an external
`libpmi.so`.

Process manager, e.g. `hydra`, will also be disabled if external pmi library
is used. Typically hydra will not be compatible with an external pmi library
even with the same PMI interface. However, we can force build `hydra` with
`--with-pm=hydra`.

When multiple PMI interfaces are supported by the linked pmi library, if we
want to force a specific interface, we can use the `--with-pmi` option. For
example, `--with-pmi=pmi2` will force PMI-2 interface.

As a special case, to build with a Slrum environment, use
`--with-pmilib=slurm --with-slurm=[path]`. This is because Slurm installs its
header file under `path/include/slurm/`, thus requires special handling.
Slurm supports both PMI-v1 and PMI2 interfaces. We default to use PMI-v1.
use `--with-pmilib=slurm --with-slurm=[path] --with-pmi=pmi2` to choose
PMI2 instead.

In development, we are adding basic PMIx support in MPICH's libpmi. Future
versions MPICH may support PMIx without depending on libpmix.

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

   * `PMI_dead_processes`

     A list processes that have exited or are dead.
