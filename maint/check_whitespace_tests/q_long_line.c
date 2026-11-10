/* expect: none */
void f(void)
{
    /* QUIRK(Q37): the ' {' of a control header may end past column 100 */
    if (thread_level_provided == MPI_THREAD_SINGLE || thread_level_provided == MPI_THREAD_FUNNELE) {
        x = 1;
    }
    /* QUIRK(Q36): a statement with a comment keeps its long lines */
    MPII_Segment_manipulate(segp, first, lastp, leaf_contig_count_block, leaf_vector_count_block, NULL, /* c */
                            (void *) &params);
    foo(a, bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb, c);        /* c */
    /* no break point within 100, or none that moves the text left */
    foo("Scheduling scatter followed by recursive exchange allgather based broadcast on %d ranksXXXXXX", n, m);
    x = "Scheduling scatter followed by recursive exchange allgather based broadcast on %d ranks, root";
    foo(aaaa,
        "Scheduling scatter followed by recursive exchange allgather based broadcast on %d ranksXXXXX",
        n);
}

/* QUIRK(Q38): after a kept break before a declarator */
int ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff(int count, int thresh, struct fid_cntr
                                                                           *trigger_cntr, struct fid_cntr
                                                                           *complete_cntr,
                                                                           bool is_persistent);
