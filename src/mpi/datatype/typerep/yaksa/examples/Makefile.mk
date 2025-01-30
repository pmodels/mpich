##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/examples

EXTRA_PROGRAMS += $(example_list)

example_list = examples/contig         \
               examples/vector         \
               examples/hvector        \
               examples/indexed_block  \
               examples/hindexed_block \
               examples/indexed        \
               examples/hindexed       \
               examples/resized        \
               examples/subarray       \
               examples/fuf            \
               examples/iov

example_utils = examples/matrix_util.c

examples_contig_SOURCES         = examples/contig.c $(example_utils)
examples_contig_LDADD           = libyaksa.la
examples_vector_SOURCES         = examples/vector.c $(example_utils)
examples_vector_LDADD           = libyaksa.la
examples_hvector_SOURCES        = examples/hvector.c $(example_utils)
examples_hvector_LDADD          = libyaksa.la
examples_indexed_block_SOURCES  = examples/indexed_block.c $(example_utils)
examples_indexed_block_LDADD    = libyaksa.la
examples_hindexed_block_SOURCES = examples/hindexed_block.c $(example_utils)
examples_hindexed_block_LDADD   = libyaksa.la
examples_indexed_SOURCES        = examples/indexed.c $(example_utils)
examples_indexed_LDADD          = libyaksa.la
examples_hindexed_SOURCES       = examples/hindexed.c $(example_utils)
examples_hindexed_LDADD         = libyaksa.la
examples_resized_SOURCES        = examples/resized.c $(example_utils)
examples_resized_LDADD          = libyaksa.la
examples_subarray_SOURCES       = examples/subarray.c $(example_utils)
examples_subarray_LDADD         = libyaksa.la
examples_fuf_SOURCES            = examples/fuf.c $(example_utils)
examples_fuf_LDADD              = libyaksa.la
examples_iov_SOURCES            = examples/iov.c $(example_utils)
examples_iov_LDADD              = libyaksa.la
