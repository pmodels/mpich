##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_single.mtest

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS =      \
    attrt              \
    attrend            \
    attrend2           \
    attric             \
    attrerr            \
    attrerrcomm        \
    attrerrtype        \
    attrdelete         \
    attrdeleteget      \
    attr2type          \
    attrorder          \
    attrordercomm      \
    attrordertype      \
    baseattr2          \
    baseattrcomm       \
    fkeyval            \
    fkeyvalcomm        \
    keyval_double_free \
    keyval_double_free_comm \
    keyval_double_free_type \
    keyval_double_free_win  \
    fkeyvaltype

EXTRA_DIST = testlist.in

fkeyvaltype_CPPFLAGS = $(AM_CPPFLAGS)
fkeyvaltype_SOURCES = fkeyvaltype.c
