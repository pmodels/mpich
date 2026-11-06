/* expect: C7 */
/* -nbbo with --honour-newlines: indent never moves a leading '&&' or '||' up, it only
 * joins the lines when they fit */
void f(void)
{
    do {
        x = 1;
    } while (connreq->stat == MPIDI_CH3I_PORT_CONNREQ_INITED
             && (int) time_gap < timeout);
    while (aaaa
           || bbbb) {
        x = 1;
    }
    return aaaa
        && bbbb;
    foo(aaaa
        && bbbb, c);
    if ((aaaa
         || bbbb) && cccc) {
        x = 1;
    }
    for (i = 0; i < n
         && j; i++) {
        x = 1;
    }
    if (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
        && bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb) {
        x = 1;
    }
    y = aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
        || bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb;
    if ((aaaa || bbbb)
        && cccc) {
        x = 1;
    }
    if (((requested_lock == MPI_LOCK_SHARED) &&
         ((existing_lock == MPID_LOCK_NONE) || (existing_lock == MPI_LOCK_SHARED)))
        ||
        ((requested_lock == MPI_LOCK_EXCLUSIVE) && (existing_lock == MPID_LOCK_NONE))) {
        x = 1;
    }
}
