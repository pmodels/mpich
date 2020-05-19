##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_single.mtest

EXTRA_DIST = testlist.in

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS =             \
    blockindexed_misc         \
    blockindexed_zero_count   \
    concurrent_irecv          \
    contents                  \
    contig_zero_count         \
    contigstruct              \
    cxx_types                 \
    dataalign                 \
    darray_pack               \
    darray_cyclic             \
    gaddress                  \
    get_elements              \
    get_elements_pairtype     \
    getpartelm                \
    hindexed_zeros            \
    hindexed_block            \
    hindexed_block_contents   \
    hvecblklen                \
    indexed_misc              \
    indexed_misc_oldapi       \
    large_count               \
    large_type                \
    large_type_sendrec        \
    large_vec                 \
    lbub                      \
    lbub_oldapi               \
    localpack                 \
    longdouble                \
    lots_of_types             \
    pairtype_pack             \
    pairtype_size_extent      \
    sendrecvt2                \
    sendrecvt4                \
    simple_commit             \
    simple_pack               \
    simple_pack_external      \
    simple_pack_external2     \
    simple_resized            \
    simple_size_extent        \
    simple_size_extent_oldapi \
    sizedtypes                \
    slice_pack                \
    slice_pack_external       \
    struct_derived_zeros      \
    struct_empty_el           \
    struct_ezhov              \
    struct_no_real_types      \
    struct_pack               \
    structpack2               \
    struct_verydeep           \
    struct_zero_count         \
    subarray                  \
    subarray_pack             \
    tfree                     \
    tmatchsize                \
    transpose_pack            \
    transpose_pack_oldapi     \
    tresized                  \
    tresized2                 \
    triangular_pack           \
    typecommit                \
    typefree                  \
    typelb                    \
    typename                  \
    typename_oldapi           \
    unpack                    \
    unusual_noncontigs        \
    vecblklen                 \
    zeroblks                  \
    zeroparms

# Some of the tests use a more comprehensive set of datatype tests.
# These must specify a different LDADD that includes the object file
# with these additional routines
LDADDDATA = $(top_builddir)/util/dtypes.$(OBJEXT)

sendrecvt2_LDADD = $(LDADD) $(LDADDDATA)
sendrecvt4_LDADD = $(LDADD) $(LDADDDATA)
