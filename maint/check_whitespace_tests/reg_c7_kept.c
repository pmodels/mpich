/* expect: none */
void f(void)
{
    switch (x) {
        case MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_barrier_release_gather_aaa:
            mpi_errno =
                MPIDI_POSIX_mpi_barrier_release_gather(comm, coll_attr);
            break;
    }
    fprintf(stderr,
            "Intra-node collectives about to allocate more shared memory than the set limit \
            the specified limit. Fallback \
            to other algorithms.\n");
    if (x) {
        uint64_t ss_bits =
            MPIDI_OFI_init_sendtag(MPL_atomic_relaxed_load_int
                                   (&MPIDI_OFI_REQUEST(request, util_id)),
                                   MPIR_Comm_rank(comm), request->status.MPI_TAG);
    }
    x = !
        ((aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa) &&
         bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb);
    ranks_per_package =
        (int
         **) MPL_malloc(max_entries_per_level[package_level_xxx] * sizeof(int *), MPL_MEM_OTHER);
}
