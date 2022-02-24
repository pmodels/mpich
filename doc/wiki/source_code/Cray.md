# Cray

This page describes how to build mpich from the
[main](https://github.com/pmodels/mpich/tree/main)
branch of the MPICH git [repository](https://github.com/pmodels/mpich).

This build will not perform as well as the vendor's optimized MPI. The
libfabric 'gni' provider was LANL + Intel + Cray open source reference
implementation. It was never particularly optimized and won't perform
well on KNL. However, building the open source MPICH provides a way to
do before/after testing of optimizations. Just remember to compare MPICH
to MPICH and not MPICH to MPT.

## Cray build instructions

Building on the Cray is like other systems: you'll need fairly
up-to-date Autotools (autoconf \>= 2.67, automake \>= 1.15, libtool \>=
2.4.4). On Argonne's theta machine, you can find these in
`~robl/soft/cray/autotools`

### Configure Options

Configure MPICH with these Cray specific options:

  - `--with-device=ch4:ofi:gni` - For the Cray transport
  - `--with-pmi=cray` - To pick up Cray PMI library from modules
  - `--with-pm=no` - Hydra won't work on the cray. Instead, `aprun`
                     launches the processes.
  - `--enable-ugni-static` - Configures the UGNI provider in a way that
                             might help the KNL nodes perform better

All in one place so you can cut and paste:

```
configure --with-device=ch4:ofi:gni --with-file-system=lustre --with-pm=no --with-pmi=cray --enable-ugni-static
```

### Build notes

  - libfabric will use standard atomic types if detected, but the Intel
    compilers do not export all the types needed. You will need

<https://github.com/ofiwg/libfabric/pull/3984/> until we update
libfabric to something newer than 1.6.0

  - In fact, there are atomic types all over the place, and libfabric's
    gni provider doesn't have a fallback like the above general patch.
    For now load `PrgEnv-gnu` into your environment. That should bring
    in a fairly recent gcc (gcc-7.3 as of this writing)

  - Libfabric-1.7.1 and older will fail to find some gni header files
    and might tell you \`WARNING: GNI provider requires CLE 5.2.UP04 or
    higher. Disabling gni provider.\`. Upgrade to libfabric-1.7.2 or
    newer, or cherry-pick
    <https://github.com/pmodels/libfabric/commit/192d52ce53e8>

Since we used `PrgEnv-gnu` to build MPICH, you will need to make sure
`PrgEnv-gnu` is in your environment. For me, my submitted jobs kept
wanting to load `PrgEnv-intel` by default, so I had to insert

```
module swap PrgEnv-intel PrgEnv-gnu
```

into my job submission script.
