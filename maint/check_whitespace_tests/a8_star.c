/* expect: A8 */
int f(int a, int b, char **argv)
{
    char *p = argv[0];
    char **pp = argv;
    int x = a * b;
    void *v = (void *) p;
    char*q = p;
    char * r = p;
    char ** rr = argv;
    x = a* b;
    v = (void*) p;
    return x + (q == r) + (rr == pp) + (v != 0);
}
