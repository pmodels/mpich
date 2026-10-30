/* expect: A7 */
void f(void *q, long n)
{
    char *p = (char *) q;
    MPI_Aint a = (MPI_Aint) n;
    long b = (long) -n;
    int c = (int) sizeof(long);
    void (*fp) (void) = (void (*)(void)) q;
    p = (MPI_Win) (void *) q;
    p = (char *)q;
    p = (MPI_Win) (void *)q;
    a = (MPI_Aint)n;
    b = (long)  -n;
    c = (int)(n + 1);
    fp = (void (*)(void))q;
    for (int *r = (int *)q; r; r = 0) {
        n++;
    }
}
