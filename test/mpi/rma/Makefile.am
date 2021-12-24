##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_single.mtest
include Makefile_mpitests.mk


## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS =          \
    allocmem               \
    test1                  \
    test2                  \
    test2_shm              \
    test3                  \
    test3_shm              \
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
    wintest_shm            \
    transpose1             \
    transpose2             \
    transpose3             \
    transpose3_shm         \
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
    test2_am_shm           \
    test3_am               \
    test3_am_shm           \
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
    nullpscw_shm	   \
    rmanull                \
    rmazero                \
    mixedsync              \
    manyrma2		   \
    manyrma2_shm           \
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
    window_allocation      \
    window_noncontig_allocation  \
    window_attach                   \
    win_flavors            \
    win_shared             \
    win_shared_noncontig   \
    win_shared_noncontig_put \
    win_shared_zerobyte    \
    win_shared_create_allocshm    \
    win_shared_create_no_allocshm \
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
    linked_list_bench_lock_shr_nocheck  \
    fetch_and_op           \
    get_accumulate         \
    flush                  \
    win_shared_put_flush_get          \
    reqops                 \
    rputget                \
    req_example            \
    req_example_shm        \
    pscw_ordering          \
    pscw_ordering_shm	   \
    mutex_bench            \
    mutex_bench_shared     \
    mutex_bench_shm        \
    mutex_bench_shm_ordered\
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
    acc_pairtype_shm       \
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
    get_flushlocal_shm     \
    win_shared_query_null  \
    accfence1              \
    accpscw1               \
    epochtest              \
    getfence1              \
    lock_contention_dt     \
    lock_dt                \
    lock_dt_flush          \
    lock_dt_flushlocal     \
    lockall_dt             \
    lockall_dt_flush       \
    lockall_dt_flushall    \
    lockall_dt_flushlocal  \
    lockall_dt_flushlocalall \
    putfence1              \
    putpscw1

noinst_PROGRAMS += aint

acc_pairtype_shm_SOURCES            = acc_pairtype.c
acc_pairtype_shm_CPPFLAGS           = -DUSE_WIN_ALLOCATE $(AM_CPPFLAGS)

strided_acc_indexed_LDADD       = $(LDADD) -lm
strided_acc_onelock_LDADD       = $(LDADD) -lm
strided_acc_subarray_LDADD      = $(LDADD) -lm
strided_get_indexed_LDADD       = $(LDADD) -lm
strided_getacc_indexed_LDADD    = $(LDADD) -lm
strided_putget_indexed_LDADD    = $(LDADD) -lm
strided_getacc_indexed_shared_LDADD = $(LDADD) -lm
strided_putget_indexed_shared_LDADD = $(LDADD) -lm

window_allocation_SOURCES            = window_creation.c
window_allocation_CPPFLAGS           = -DUSE_WIN_ALLOCATE $(AM_CPPFLAGS)

window_noncontig_allocation_SOURCES  = window_creation.c
window_noncontig_allocation_CPPFLAGS = -DUSE_WIN_ALLOCATE_NONCONTIG $(AM_CPPFLAGS)

req_example_shm_CPPFLAGS         = -DUSE_WIN_ALLOC_SHM $(AM_CPPFLAGS)
req_example_shm_SOURCES          = req_example.c

manyrma2_shm_CPPFLAGS            = -DUSE_WIN_ALLOCATE $(AM_CPPFLAGS)
wintest_shm_CPPFLAGS             = -DUSE_WIN_ALLOCATE $(AM_CPPFLAGS)
transpose3_shm_CPPFLAGS          = -DUSE_WIN_ALLOCATE $(AM_CPPFLAGS)
nullpscw_shm_CPPFLAGS            = -DUSE_WIN_ALLOCATE $(AM_CPPFLAGS)
pscw_ordering_shm_CPPFLAGS       = -DUSE_WIN_ALLOCATE $(AM_CPPFLAGS)
test2_shm_CPPFLAGS               = -DUSE_WIN_ALLOCATE $(AM_CPPFLAGS)
test2_am_shm_CPPFLAGS            = -DUSE_WIN_ALLOCATE $(AM_CPPFLAGS)
test3_shm_CPPFLAGS               = -DUSE_WIN_ALLOCATE $(AM_CPPFLAGS)
test3_am_shm_CPPFLAGS            = -DUSE_WIN_ALLOCATE $(AM_CPPFLAGS)

manyrma2_shm_SOURCES             = manyrma2.c
wintest_shm_SOURCES              = wintest.c
transpose3_shm_SOURCES           = transpose3.c
nullpscw_shm_SOURCES             = nullpscw.c
pscw_ordering_shm_SOURCES        = pscw_ordering.c
test2_shm_SOURCES                = test2.c
test2_am_shm_SOURCES             = test2_am.c
test3_shm_SOURCES                = test3.c
test3_am_shm_SOURCES             = test3_am.c

mutex_bench_SOURCES              = mutex_bench.c mcs_mutex.c mcs_mutex.h
mutex_bench_shared_CPPFLAGS      = -DUSE_WIN_SHARED $(AM_CPPFLAGS)
mutex_bench_shared_SOURCES       = mutex_bench.c mcs_mutex.c mcs_mutex.h
mutex_bench_shm_CPPFLAGS         = -DUSE_WIN_ALLOC_SHM $(AM_CPPFLAGS)
mutex_bench_shm_SOURCES          = mutex_bench.c mcs_mutex.c mcs_mutex.h
mutex_bench_shm_ordered_CPPFLAGS = -DUSE_WIN_ALLOC_SHM -DUSE_CONTIGUOUS_RANK $(AM_CPPFLAGS)
mutex_bench_shm_ordered_SOURCES  = mutex_bench.c mcs_mutex.c mcs_mutex.h

linked_list_bench_lock_shr_nocheck_SOURCES  = linked_list_bench_lock_shr.c
linked_list_bench_lock_shr_nocheck_CPPFLAGS = -DUSE_MODE_NOCHECK $(AM_CPPFLAGS)

win_shared_create_allocshm_SOURCES = win_shared_create.c
win_shared_create_no_allocshm_SOURCES = win_shared_create.c
win_shared_create_allocshm_CPPFLAGS = -DUSE_INFO_ALLOC_SHM $(AM_CPPFLAGS)

get_flushlocal_shm_SOURCES        = get_flushlocal.c
get_flushlocal_shm_CPPFLAGS       = -DUSE_WIN_ALLOCATE $(AM_CPPFLAGS)

accfence1_CPPFLAGS = $(AM_CPPFLAGS)
accfence1_SOURCES = accfence1.c

accpscw1_CPPFLAGS = $(AM_CPPFLAGS)
accpscw1_SOURCES = accpscw1.c

epochtest_CPPFLAGS = $(AM_CPPFLAGS)
epochtest_SOURCES = epochtest.c

getfence1_CPPFLAGS = -DUSE_GET $(AM_CPPFLAGS)
getfence1_SOURCES = fence.c

putfence1_CPPFLAGS = -DUSE_PUT $(AM_CPPFLAGS)
putfence1_SOURCES = fence.c

lock_contention_dt_CPPFLAGS = -DMULTI_ORIGIN -DSINGLE_TARGET -DUSE_FLUSH_NONE -DUSE_FLUSH_LOCAL_NONE $(AM_CPPFLAGS)
lock_contention_dt_SOURCES = lock_x_dt.c

lock_dt_CPPFLAGS = -DSINGLE_ORIGIN -DSINGLE_TARGET -DUSE_FLUSH_NONE -DUSE_FLUSH_LOCAL_NONE $(AM_CPPFLAGS)
lock_dt_SOURCES = lock_x_dt.c

lock_dt_flush_CPPFLAGS = -DSINGLE_ORIGIN -DSINGLE_TARGET -DUSE_FLUSH -DUSE_FLUSH_LOCAL_NONE $(AM_CPPFLAGS)
lock_dt_flush_SOURCES = lock_x_dt.c

lock_dt_flushlocal_CPPFLAGS = -DSINGLE_ORIGIN -DSINGLE_TARGET -DUSE_FLUSH_NONE -DUSE_FLUSH_LOCAL $(AM_CPPFLAGS)
lock_dt_flushlocal_SOURCES = lock_x_dt.c

lockall_dt_CPPFLAGS = -DSINGLE_ORIGIN -DMULTI_TARGET -DUSE_FLUSH_NONE -DUSE_FLUSH_LOCAL_NONE $(AM_CPPFLAGS)
lockall_dt_SOURCES = lock_x_dt.c

lockall_dt_flush_CPPFLAGS = -DSINGLE_ORIGIN -DMULTI_TARGET -DUSE_FLUSH -DUSE_FLUSH_LOCAL_NONE $(AM_CPPFLAGS)
lockall_dt_flush_SOURCES = lock_x_dt.c

lockall_dt_flushall_CPPFLAGS = -DSINGLE_ORIGIN -DMULTI_TARGET -DUSE_FLUSH_ALL -DUSE_FLUSH_LOCAL_NONE $(AM_CPPFLAGS)
lockall_dt_flushall_SOURCES = lock_x_dt.c

lockall_dt_flushlocal_CPPFLAGS = -DSINGLE_ORIGIN -DMULTI_TARGET -DUSE_FLUSH_NONE -DUSE_FLUSH_LOCAL $(AM_CPPFLAGS)
lockall_dt_flushlocal_SOURCES = lock_x_dt.c

lockall_dt_flushlocalall_CPPFLAGS = -DSINGLE_ORIGIN -DMULTI_TARGET -DUSE_FLUSH_NONE -DUSE_FLUSH_LOCAL_ALL $(AM_CPPFLAGS)
lockall_dt_flushlocalall_SOURCES = lock_x_dt.c

putpscw1_CPPFLAGS = $(AM_CPPFLAGS)
putpscw1_SOURCES = putpscw1.c
