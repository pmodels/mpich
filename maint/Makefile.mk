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
    maint/genparams                   \
    maint/genstates.in                \
    maint/getcoverage.in              \
    maint/local_perl/lib/YAML/Tiny.pm \
    maint/parse.sub                   \
    maint/parsetest                   \
    maint/release.pl                  \
    maint/samplebuilds                \
    maint/simplemake.in               \
    maint/testbuild                   \
    maint/update_windows_version      \
    maint/testpmpi                    \
    maint/updatefiles                 \
    maint/smlib/libadd.smlib

dist_noinst_DATA +=                        \
    maint/Version                          \
    maint/Version.base.m4                  \
    maint/docnotes                         \
    maint/errmsgdirs                       \
    maint/gccimpgen.cpp                    \
    maint/impgen.vcproj                    \
    maint/local_perl/README                \
    maint/local_perl/YAML-Tiny-1.41.tar.gz \
    maint/makegcclibs.bat                  \
    maint/makegcclibs_64.bat               \
    maint/mpi1.lst                         \
    maint/mpich2i.vdproj                   \
    maint/mpich2x64i.vdproj                \
    maint/setup.jpg                        \
    maint/structalign.c                    \
    maint/template.c                       \
    maint/version.m4                       \
    maint/winbuild.wsf                     \
    maint/simplemake.txt                   \
    maint/smlib/README

### TODO FIXME what do we do about these?
##makedefs
##sampleconf.in

