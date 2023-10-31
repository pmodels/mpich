# Cray

This page describes how to build mpich from the
[main](https://github.com/pmodels/mpich/tree/main)
branch of the MPICH git [repository](https://github.com/pmodels/mpich).

## General

Building on the Cray is like other systems: you'll need fairly
up-to-date Autotools (autoconf \>= 2.67, automake \>= 1.15, libtool \>=
2.4.4). On Argonne's theta machine, you can find these in
`~robl/soft/cray/autotools`.  On Polaris, these seem to be new enough already.

## Slingshot

On Cray systems the vendor-supplied libfabric should be used to get slingshot support


### Configure options

You can leave off the 'cuda' options if you are e.g. testing only I/O or building.
The Cray environment doesn't provide convenience environment variables for the 'libfabric' path, so you'll have to update that by hand when necessary

    --with-cuda=/soft/compilers/cudatoolkit/cuda-11.4.4 --with-cuda-sm=80 --with-pmi=pmi2 --with-pmi2=${CRAY_PMI_PREFIX} --with-pm=no --with-libfabric=/opt/cray/libfabric/1.11.0.4.125

## GNI

This build will not perform as well as the vendor's optimized MPI. The
libfabric 'gni' provider was LANL + Intel + Cray open source reference
implementation. It was never particularly optimized and won't perform
well on KNL. However, building the open source MPICH provides a way to
do before/after testing of optimizations. Just remember to compare MPICH
to MPICH and not MPICH to MPT.

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

Despite requesting pmi2 at configure time, I had to explicitly request it at
run-time with `export MPIR_CVAR_PMI_VERSION=2`

Since we used `PrgEnv-gnu` to build MPICH, you will need to make sure
`PrgEnv-gnu` is in your environment. For me, my submitted jobs kept
wanting to load `PrgEnv-intel` by default, so I had to insert

```
module swap PrgEnv-intel PrgEnv-gnu
```

into my job submission script.
