/* expect: none */
/* QUIRK(Q30): top level, no open bracket: column 0 or 4 */
unsigned long long counter_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa[bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb]
ATTRIBUTE((unused));
unsigned long long counter_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa[bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb]
    ATTRIBUTE((unused));
int g(void)
{
    /* QUIRK(Q28): indent puts these at the '(' + 3 */
    foo(
        /* c */
        aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa, bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb);
    foo(
#if X
        aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,
#endif
        bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb);
    /* QUIRK(Q29): indent puts the first name at + 1 */
    int aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,       /* a */
        bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb,       /* b */
        cccccccccccccccccccccccccccccccccccccccc;
    int *aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,      /* a */
        *bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb;
    /* QUIRK(Q31): indent puts these at the statement column */
    x = disp + flat_file_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa->indices[st_index]
        + (ADIO_Offset) n_filetypes * filetype_extent;
    /* QUIRK(Q22): macro calls without ';' */
    MATCH("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb)
    MATCH("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", cccccccccccccccccccccccccccccccccccccccc)
    /* QUIRK(Q32): statements in a macro argument */
    TEST_OP("Put",
            MPI_Put(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa, bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb);
);
    return 0;
}
