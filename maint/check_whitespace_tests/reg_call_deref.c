/* expect: A5 */
/* calls whose argument starts with a dereference are not function-pointer declarators */
int f(int *p, int *n)
{
    MPIR_Assert (*n > 0);
    g (*p, 1);
    h (*p);
    return 0;
}
