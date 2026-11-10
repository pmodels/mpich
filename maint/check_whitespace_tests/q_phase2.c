/* expect: none */
/* the natural forms the phase-2 quirks accept besides indent's */
struct s {
    int a, b;
};

int f(int a, foo_t *p, foo_t **pp, int (*fp) (int), long extent)
{
    struct s t = {.a = 1, .b = 2};
    struct s u = { .a = 1,.b = 2 };
    void *v = (foo_t *) &a;
    int x = (a) -a;
    x = (fp)(1);
    x = (*fp)(1);
    x = !!a;
    x = a + (int64_t) 2 * extent;
    x = 1;                      /* column 33 */
    for (int *q = (int *)p; q; q = 0) {
        x++;
    }
    return x + t.a + u.a + (v != 0) + (pp != 0);
}

foo_t * g(foo_t * p);
#ifdef A
#endif                          /* column 33 */
