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
    dataalign2                \
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
    large_count               \
    large_type                \
    large_type_sendrec        \
    large_vec                 \
    type_large	              \
    lbub                      \
    localpack                 \
    longdouble                \
    lots_of_types             \
    pairtype_pack             \
    pairtype_size_extent      \
    sendrecvt2                \
    sendrecvt4                \
    simple_commit             \
    simple_pack               \
    pack_external             \
    simple_pack_external      \
    simple_pack_external2     \
    simple_resized            \
    simple_size_extent        \
    sizedtypes                \
    slice_pack                \
    slice_pack_external       \
    struct_derived_zeros      \
    struct_empty_el           \
    struct_ezhov              \
    struct_pack               \
    structpack2               \
    struct_pack_mpi_bottom    \
    struct_unpack_mpi_bottom  \
    struct_verydeep           \
    struct_zero_count         \
    subarray                  \
    subarray_pack             \
    tfree                     \
    tmatchsize                \
    transpose_pack            \
    tresized                  \
    tresized2                 \
    triangular_pack           \
    typecommit                \
    typefree                  \
    typelb                    \
    typename                  \
    unpack                    \
    unusual_noncontigs        \
    vecblklen                 \
    zeroblks                  \
    zeroparms

if NOT_STRICTMPI
noinst_PROGRAMS += \
    indexed_misc_oldapi       \
    lbub_oldapi               \
    simple_size_extent_oldapi \
    struct_no_real_types      \
    transpose_pack_oldapi     \
    typename_oldapi
endif

# Some of the tests use a more comprehensive set of datatype tests.
# These must specify a different LDADD that includes the object file
# with these additional routines
LDADDDATA = $(top_builddir)/util/libdtypes.la
$(top_builddir)/util/libdtypes.la:
	(cd $(top_builddir)/util && $(MAKE))

sendrecvt2_LDADD = $(LDADD) $(LDADDDATA)
sendrecvt4_LDADD = $(LDADD) $(LDADDDATA)
