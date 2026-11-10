/* expect: none */
struct s {
    int a;
    /* device-specific fields */
#ifdef DEV_DECL
     DEV_DECL
#endif
     foo_t * p;                 /* pointer */
    int b;
};

int f(int r, int *t)
{
    int x = F(,, r, -1, t);
    int y = G(r,,);

    return x + y;
}
