# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile.mtest

EXTRA_DIST = testlist.in

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
    lock_dt                \
    lock_dt_flush          \
    lock_dt_flushlocal     \
    lockall_dt             \
    lockall_dt_flush       \
    lockall_dt_flushall    \
    lockall_dt_flushlocal  \
    lockall_dt_flushlocalall\
    lock_contention_dt     \
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
    win_flavors            \
    win_shared             \
    win_shared_noncontig   \
    win_shared_noncontig_put \
    win_shared_zerobyte    \
    win_shared_create_allocshm    \
    win_shared_create_no_allocshm \
    win_zero               \
    win_large_shm          \
    win_dynamic_acc        \
    get_acc_local          \
    compare_and_swap       \
    linked_list            \
    linked_list_fop        \
    linked_list_lockall    \
    linked_list_bench_lock_all  \
    linked_list_bench_lock_excl \
    linked_list_bench_lock_shr  \
    linked_list_bench_lock_shr_nocheck  \
    fetch_and_op_char      \
    fetch_and_op_short     \
    fetch_and_op_int       \
    fetch_and_op_long      \
    fetch_and_op_double    \
    fetch_and_op_long_double \
    get_accumulate_short   \
    get_accumulate_long    \
    get_accumulate_double  \
    get_accumulate_int     \
    get_accumulate_short_derived   \
    get_accumulate_long_derived    \
    get_accumulate_double_derived  \
    get_accumulate_int_derived     \
    flush                  \
    win_shared_put_flush_get          \
    reqops                 \
    req_example            \
    req_example_shm        \
    win_info               \
    pscw_ordering          \
    pscw_ordering_shm	   \
    mutex_bench            \
    mutex_bench_shared     \
    mutex_bench_shm        \
    mutex_bench_shm_ordered\
    rma-contig             \
    badrma                 \
    nb_test                \
    acc-loc                \
    fence_shm              \
    get-struct             \
    rput_local_comp        \
    racc_local_comp        \
    at_complete            \
    atomic_rmw_fop         \
    atomic_rmw_cas         \
    atomic_rmw_gacc	   \
    atomic_get             \
    acc-pairtype           \
    manyget                \
    derived-acc-flush_local\
    large-acc-flush_local  \
    large-small-acc        \
    put_flush_get  \
    acc_flush_get  \
    gacc_flush_get \
    fop_flush_get  \
    cas_flush_get  \
    win_shared_put_flush_load  \
    win_shared_acc_flush_load  \
    win_shared_gacc_flush_load \
    win_shared_fop_flush_load  \
    win_shared_cas_flush_load

if BUILD_MPIX_TESTS
noinst_PROGRAMS += aint
endif

strided_acc_indexed_LDADD       = $(LDADD) -lm
strided_acc_onelock_LDADD       = $(LDADD) -lm
strided_acc_subarray_LDADD      = $(LDADD) -lm
strided_get_indexed_LDADD       = $(LDADD) -lm
strided_getacc_indexed_LDADD    = $(LDADD) -lm
strided_putget_indexed_LDADD    = $(LDADD) -lm
strided_getacc_indexed_shared_LDADD = $(LDADD) -lm
strided_putget_indexed_shared_LDADD = $(LDADD) -lm

fetch_and_op_char_CPPFLAGS       = -DFOP_TYPE_CHAR $(AM_CPPFLAGS)
fetch_and_op_short_CPPFLAGS      = -DFOP_TYPE_SHORT $(AM_CPPFLAGS)
fetch_and_op_int_CPPFLAGS        = -DFOP_TYPE_INT $(AM_CPPFLAGS)
fetch_and_op_long_CPPFLAGS       = -DFOP_TYPE_LONG $(AM_CPPFLAGS)
fetch_and_op_double_CPPFLAGS     = -DFOP_TYPE_DOUBLE $(AM_CPPFLAGS)
fetch_and_op_long_double_CPPFLAGS= -DFOP_TYPE_LONG_DOUBLE $(AM_CPPFLAGS)

fetch_and_op_char_SOURCES        = fetch_and_op.c
fetch_and_op_short_SOURCES       = fetch_and_op.c
fetch_and_op_int_SOURCES         = fetch_and_op.c
fetch_and_op_long_SOURCES        = fetch_and_op.c
fetch_and_op_double_SOURCES      = fetch_and_op.c
fetch_and_op_long_double_SOURCES = fetch_and_op.c

get_accumulate_short_CPPFLAGS    = -DGACC_TYPE_SHORT $(AM_CPPFLAGS)
get_accumulate_long_CPPFLAGS     = -DGACC_TYPE_LONG $(AM_CPPFLAGS)
get_accumulate_double_CPPFLAGS   = -DGACC_TYPE_DOUBLE $(AM_CPPFLAGS)
get_accumulate_int_CPPFLAGS      = -DGACC_TYPE_INT $(AM_CPPFLAGS)
get_accumulate_short_derived_CPPFLAGS  = -DGACC_TYPE_SHORT -DGACC_TYPE_DERIVED $(AM_CPPFLAGS)
get_accumulate_long_derived_CPPFLAGS   = -DGACC_TYPE_LONG -DGACC_TYPE_DERIVED $(AM_CPPFLAGS)
get_accumulate_double_derived_CPPFLAGS = -DGACC_TYPE_DOUBLE -DGACC_TYPE_DERIVED $(AM_CPPFLAGS)
get_accumulate_int_derived_CPPFLAGS    = -DGACC_TYPE_INT -DGACC_TYPE_DERIVED $(AM_CPPFLAGS)

get_accumulate_short_SOURCES     = get_accumulate.c
get_accumulate_long_SOURCES      = get_accumulate.c
get_accumulate_double_SOURCES    = get_accumulate.c
get_accumulate_int_SOURCES       = get_accumulate.c
get_accumulate_short_derived_SOURCES  = get_accumulate.c
get_accumulate_long_derived_SOURCES   = get_accumulate.c
get_accumulate_double_derived_SOURCES = get_accumulate.c
get_accumulate_int_derived_SOURCES      = get_accumulate.c

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

mutex_bench_SOURCES              = mutex_bench.c mcs-mutex.c mcs-mutex.h
mutex_bench_shared_CPPFLAGS      = -DUSE_WIN_SHARED $(AM_CPPFLAGS)
mutex_bench_shared_SOURCES       = mutex_bench.c mcs-mutex.c mcs-mutex.h
mutex_bench_shm_CPPFLAGS         = -DUSE_WIN_ALLOC_SHM $(AM_CPPFLAGS)
mutex_bench_shm_SOURCES          = mutex_bench.c mcs-mutex.c mcs-mutex.h
mutex_bench_shm_ordered_CPPFLAGS = -DUSE_WIN_ALLOC_SHM -DUSE_CONTIGUOUS_RANK $(AM_CPPFLAGS)
mutex_bench_shm_ordered_SOURCES  = mutex_bench.c mcs-mutex.c mcs-mutex.h

linked_list_bench_lock_shr_nocheck_SOURCES  = linked_list_bench_lock_shr.c
linked_list_bench_lock_shr_nocheck_CPPFLAGS = -DUSE_MODE_NOCHECK $(AM_CPPFLAGS)

win_shared_create_allocshm_SOURCES = win_shared_create.c
win_shared_create_no_allocshm_SOURCES = win_shared_create.c
win_shared_create_allocshm_CPPFLAGS = -DUSE_INFO_ALLOC_SHM $(AM_CPPFLAGS)

put_flush_get_SOURCES        = wrma_flush_get.c
acc_flush_get_SOURCES        = wrma_flush_get.c
gacc_flush_get_SOURCES       = wrma_flush_get.c
fop_flush_get_SOURCES        = wrma_flush_get.c
cas_flush_get_SOURCES        = wrma_flush_get.c

put_flush_get_CPPFLAGS  = -DTEST_PUT $(AM_CPPFLAGS)
acc_flush_get_CPPFLAGS  = -DTEST_ACC $(AM_CPPFLAGS)
gacc_flush_get_CPPFLAGS = -DTEST_GACC $(AM_CPPFLAGS)
fop_flush_get_CPPFLAGS  = -DTEST_FOP $(AM_CPPFLAGS)
cas_flush_get_CPPFLAGS  = -DTEST_CAS $(AM_CPPFLAGS)

win_shared_put_flush_load_SOURCES        = win_shared_rma_flush_load.c
win_shared_acc_flush_load_SOURCES        = win_shared_rma_flush_load.c
win_shared_gacc_flush_load_SOURCES       = win_shared_rma_flush_load.c
win_shared_fop_flush_load_SOURCES        = win_shared_rma_flush_load.c
win_shared_cas_flush_load_SOURCES        = win_shared_rma_flush_load.c

win_shared_put_flush_load_CPPFLAGS  = -DTEST_PUT $(AM_CPPFLAGS)
win_shared_acc_flush_load_CPPFLAGS  = -DTEST_ACC $(AM_CPPFLAGS)
win_shared_gacc_flush_load_CPPFLAGS = -DTEST_GACC $(AM_CPPFLAGS)
win_shared_fop_flush_load_CPPFLAGS  = -DTEST_FOP $(AM_CPPFLAGS)
win_shared_cas_flush_load_CPPFLAGS  = -DTEST_CAS $(AM_CPPFLAGS)