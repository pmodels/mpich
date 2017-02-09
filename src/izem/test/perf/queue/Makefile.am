# -*- Mode: Makefile; -*-
#
# See COPYRIGHT in top-level directory.
#

TESTS = \
	enq_deq_pairs_gl \
	enq_deq_pairs_ms \
	enq_deq_pairs_swp \
	thread_scale_mpmc_gl \
	thread_scale_mpmc_ms \
	thread_scale_mpsc_gl \
	thread_scale_mpsc_swp \
	thread_scale_mpsc_ms \
	thread_scale_spmc_gl \
	thread_scale_spmc_ms
#	enq_deq_pairs_fa
#	thread_scale_mpsc_fa

#XFAIL_TESTS = 	enq_deq_pairs_fa \
#		thread_scale_mpsc_fa

check_PROGRAMS = $(TESTS)
noinst_PROGRAMS = $(TESTS)

include $(top_srcdir)/test/Makefile.mk

enq_deq_pairs_gl_SOURCES = enq_deq_pairs.c
enq_deq_pairs_ms_SOURCES = enq_deq_pairs.c
enq_deq_pairs_swp_SOURCES = enq_deq_pairs.c
#enq_deq_pairs_fa_SOURCES = enq_deq_pairs.c
thread_scale_mpmc_gl_SOURCES = thread_scale.c
thread_scale_mpmc_ms_SOURCES = thread_scale.c
thread_scale_mpsc_gl_SOURCES = thread_scale.c
thread_scale_mpsc_swp_SOURCES = thread_scale.c
#thread_scale_mpsc_fa_SOURCES = thread_scale.c
thread_scale_mpsc_ms_SOURCES = thread_scale.c
thread_scale_spmc_gl_SOURCES = thread_scale.c
thread_scale_spmc_ms_SOURCES = thread_scale.c

enq_deq_pairs_gl_CFLAGS = -DZMTEST_USE_GLQUEUE -DZMTEST_ALLOC_QELEM -fopenmp
enq_deq_pairs_ms_CFLAGS = -DZMTEST_USE_MSQUEUE -DZMTEST_ALLOC_QELEM -fopenmp
enq_deq_pairs_swp_CFLAGS = -DZMTEST_USE_SWPQUEUE -DZMTEST_ALLOC_QELEM -fopenmp
#enq_deq_pairs_fa_CFLAGS = -DZMTEST_USE_FAQUEUE -DZMTEST_ALLOC_QELEM -fopenmp
thread_scale_mpmc_gl_CFLAGS = -DZMTEST_USE_GLQUEUE -DZMTEST_MPMC -DZMTEST_ALLOC_QELEM -fopenmp
thread_scale_mpmc_ms_CFLAGS = -DZMTEST_USE_MSQUEUE -DZMTEST_MPMC -DZMTEST_ALLOC_QELEM -fopenmp
thread_scale_mpsc_gl_CFLAGS = -DZMTEST_USE_GLQUEUE -DZMTEST_MPSC -DZMTEST_ALLOC_QELEM -fopenmp
thread_scale_mpsc_swp_CFLAGS = -DZMTEST_USE_SWPQUEUE -DZMTEST_MPSC -DZMTEST_ALLOC_QELEM -fopenmp
#thread_scale_mpsc_fa_CFLAGS = -DZMTEST_USE_FAQUEUE -DZMTEST_MPSC -DZMTEST_ALLOC_QELEM -fopenmp
thread_scale_mpsc_ms_CFLAGS = -DZMTEST_USE_MSQUEUE -DZMTEST_MPSC -DZMTEST_ALLOC_QELEM -fopenmp
thread_scale_spmc_gl_CFLAGS = -DZMTEST_USE_GLQUEUE -DZMTEST_SPMC -DZMTEST_ALLOC_QELEM -fopenmp
thread_scale_spmc_ms_CFLAGS = -DZMTEST_USE_MSQUEUE -DZMTEST_SPMC -DZMTEST_ALLOC_QELEM -fopenmp

enq_deq_pairs_gl_LDFLAGS = -fopenmp
enq_deq_pairs_ms_LDFLAGS = -fopenmp
enq_deq_pairs_swp_LDFLAGS = -fopenmp
#enq_deq_pairs_fa_LDFLAGS = -fopenmp
thread_scale_mpmc_gl_LDFLAGS = -fopenmp
thread_scale_mpmc_ms_LDFLAGS = -fopenmp
thread_scale_mpsc_gl_LDFLAGS = -fopenmp
thread_scale_mpsc_swp_LDFLAGS = -fopenmp
#thread_scale_mpsc_fa_LDFLAGS = -fopenmp
thread_scale_mpsc_ms_LDFLAGS = -fopenmp
thread_scale_spmc_gl_LDFLAGS = -fopenmp
thread_scale_spmc_ms_LDFLAGS = -fopenmp
