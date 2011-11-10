# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile.mtest

EXTRA_DIST = testlist

## For some reason, these tests were missing from both the simplemakefile and
## the testlist.  Leaving them disabled for now.
##segtest
##zero-blklen-vector

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS =           \
    blockindexed-misc       \
    blockindexed-zero-count \
    contents                \
    contig-zero-count       \
    contigstruct            \
    darray-pack             \
    gaddress                \
    get-elements            \
    get-elements-pairtype   \
    getpartelm              \
    hindexed-zeros          \
    indexed-misc            \
    lbub                    \
    localpack               \
    lots-of-types           \
    pairtype-pack           \
    pairtype-size-extent    \
    simple-commit           \
    simple-pack             \
    simple-pack-external    \
    simple-resized          \
    simple-size-extent      \
    sizedtypes              \
    slice-pack              \
    slice-pack-external     \
    struct-derived-zeros    \
    struct-empty-el         \
    struct-ezhov            \
    struct-no-real-types    \
    struct-pack             \
    struct-verydeep         \
    struct-zero-count       \
    subarray                \
    subarray-pack           \
    tfree                   \
    tmatchsize              \
    transpose-pack          \
    tresized                \
    tresized2               \
    triangular-pack         \
    typecommit              \
    typefree                \
    typelb                  \
    typename                \
    unpack                  \
    unusual-noncontigs      \
    zeroblks                \
    zeroparms

