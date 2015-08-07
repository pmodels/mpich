## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

# this file exists to ensure that files in the "maint" dir get distributed
# properly when we "make dist"

dist_noinst_SCRIPTS +=                \
    maint/configure.ac                \
    maint/checkbuilds.in              \
    maint/checkmake                   \
    maint/clmake.in                   \
    maint/createcoverage.in           \
    maint/decode_handle               \
    maint/extracterrmsgs              \
    maint/extractfixme.in             \
    maint/extractstates.in            \
    maint/extractstrings.in           \
    maint/f77tof90                    \
    maint/f77tof90.in                 \
    maint/findunusederrtxt            \
    maint/gcovmerge.in                \
    maint/getcoverage.in              \
    maint/gen_subcfg_m4               \
    maint/extractcvars                   \
    maint/genstates.in                \
    maint/getcoverage.in              \
    maint/local_perl/lib/YAML/Tiny.pm \
    maint/parse.sub                   \
    maint/parsetest                   \
    maint/release.pl                  \
    maint/samplebuilds                \
    maint/testbuild                   \
    maint/testpmpi

dist_noinst_DATA +=                        \
    maint/Version                          \
    maint/Version.base.m4                  \
    maint/docnotes                         \
    maint/errmsgdirs                       \
    maint/cvardirs                         \
    maint/gccimpgen.cpp                    \
    maint/local_perl/README                \
    maint/local_perl/YAML-Tiny-1.41.tar.gz \
    maint/mpi1.lst                         \
    maint/setup.jpg                        \
    maint/structalign.c                    \
    maint/version.m4

### TODO FIXME what do we do about these?
##makedefs
##sampleconf.in

