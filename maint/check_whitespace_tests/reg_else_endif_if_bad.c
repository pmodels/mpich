/* expect: C1 */
int f(int a, int c)
{
#ifdef X
    if (a) {
        a++;
    } else
#endif
        if (c) {
            c++;
        }
    return 0;
}
