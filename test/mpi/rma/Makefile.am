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
    lockcontention3        \
    lockopts               \
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
    accpscw1               \
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
    manyrma2		   \
    selfrma                \
    strided_acc_onelock    \
    strided_putget_indexed \
    strided_acc_indexed    \
    strided_acc_subarray   \
    strided_get_indexed    \
    window_creation        \
    win_flavors            \
    win_shared             \
    win_shared_noncontig   \
    win_shared_noncontig_put \
    win_dynamic_acc        \
    get_acc_local          \
    compare_and_swap       \
    linked_list            \
    linked_list_fop        \
    fetch_and_op_char      \
    fetch_and_op_short     \
    fetch_and_op_int       \
    fetch_and_op_long      \
    fetch_and_op_double    \
    fetch_and_op_long_double

strided_acc_indexed_LDADD       = $(LDADD) -lm
strided_acc_onelock_LDADD       = $(LDADD) -lm
strided_acc_subarray_LDADD      = $(LDADD) -lm
strided_get_indexed_LDADD       = $(LDADD) -lm
strided_putget_indexed_LDADD    = $(LDADD) -lm

fetch_and_op_char_CFLAGS        = -DFOP_TYPE_CHAR
fetch_and_op_short_CFLAGS       = -DFOP_TYPE_SHORT
fetch_and_op_int_CFLAGS         = -DFOP_TYPE_INT
fetch_and_op_long_CFLAGS        = -DFOP_TYPE_LONG
fetch_and_op_double_CFLAGS      = -DFOP_TYPE_DOUBLE
fetch_and_op_long_double_CFLAGS = -DFOP_TYPE_LONG_DOUBLE

fetch_and_op_char_SOURCES        = fetch_and_op.c
fetch_and_op_short_SOURCES       = fetch_and_op.c
fetch_and_op_int_SOURCES         = fetch_and_op.c
fetch_and_op_long_SOURCES        = fetch_and_op.c
fetch_and_op_double_SOURCES      = fetch_and_op.c
fetch_and_op_long_double_SOURCES = fetch_and_op.c
