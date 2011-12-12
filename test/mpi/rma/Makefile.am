# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile.mtest

EXTRA_DIST = testlist

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS =          \
    allocmem               \
    ircpi                  \
    test1                  \
    test2                  \
    test3                  \
    test4                  \
    test5                  \
    lockcontention         \
    lockcontention2        \
    contention_put         \
    contention_putget      \
    put_base               \
    put_bottom             \
    locknull               \
    wintest                \
    transpose1             \
    transpose2             \
    transpose3             \
    transpose4             \
    transpose5             \
    transpose6             \
    transpose7             \
    fetchandadd            \
    fetchandadd_tree       \
    winname                \
    putfence1              \
    putfidx                \
    epochtest              \
    getfence1              \
    accfence1              \
    accfence2              \
    adlb_mimic1            \
    putpscw1               \
    getgroup               \
    contig_displ           \
    test1_am               \
    test2_am               \
    test3_am               \
    test4_am               \
    test5_am               \
    fetchandadd_am         \
    fetchandadd_tree_am    \
    accfence2_am           \
    test1_dt               \
    attrorderwin           \
    wincall                \
    fkeyvalwin             \
    baseattrwin            \
    nullpscw               \
    rmanull                \
    mixedsync              \
    selfrma                \
    strided_acc_onelock    \
    strided_putget_indexed \
    strided_acc_indexed    \
    strided_acc_subarray   \
    strided_get_indexed    \
    window_creation

strided_acc_indexed_LDADD       = -lm
strided_acc_onelock_LDADD       = -lm
strided_acc_subarray_LDADD      = -lm
strided_get_indexed_LDADD       = -lm
strided_putget_indexed_LDADD    = -lm
