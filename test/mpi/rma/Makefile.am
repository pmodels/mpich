##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_single.mtest
include Makefile_mpitests.mk

EXTRA_DIST = testlist.in

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS =          \
    allocmem               \
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
    putfidx                \
    accfence2              \
    adlb_mimic1            \
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
    rmazero                \
    mixedsync              \
    manyrma2		   \
    manyrma3		   \
    selfrma                \
    strided_acc_onelock    \
    strided_putget_indexed \
    strided_putget_indexed_shared \
    strided_acc_indexed    \
    strided_acc_subarray   \
    strided_get_indexed    \
    strided_getacc_indexed \
    strided_getacc_indexed_shared \
    window_creation        \
    window_attach                   \
    win_flavors            \
    win_shared             \
    win_shared_noncontig   \
    win_shared_noncontig_put \
    win_shared_zerobyte    \
    win_shared_create      \
    win_shared_query       \
    win_zero               \
    win_large_shm          \
    win_dynamic_acc        \
    win_dynamic_rma_flush_get             \
    get_acc_local          \
    compare_and_swap       \
    linked_list            \
    linked_list_fop        \
    linked_list_lockall    \
    linked_list_bench_lock_all  \
    linked_list_bench_lock_excl \
    linked_list_bench_lock_shr  \
    fetch_and_op           \
    get_accumulate         \
    flush                  \
    win_shared_put_flush_get          \
    reqops                 \
    req_example            \
    pscw_ordering          \
    mutex_bench            \
    rma_contig             \
    badrma                 \
    nb_test                \
    acc_loc                \
    acc_ordering           \
    fence_shm              \
    get_struct             \
    rput_local_comp        \
    racc_local_comp        \
    at_complete            \
    atomic_rmw_fop         \
    atomic_rmw_cas         \
    atomic_rmw_gacc	   \
    atomic_get             \
    acc_pairtype           \
    manyget                \
    derived_acc_flush_local\
    large_acc_flush_local  \
    large_small_acc        \
    wrma_flush_get  \
    win_shared_rma_flush_load  \
    overlap_wins_rma  \
    rget_unlock  \
    lock_nested  \
    rget_testall \
    get_flushlocal         \
    win_shared_query_null  \
    accfence1              \
    accpscw1               \
    epochtest              \
    fence                  \
    lock_x_dt              \
    aint	           \
    putpscw1
