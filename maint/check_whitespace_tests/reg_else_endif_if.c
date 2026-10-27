/* expect: none */
/* else, then a directive, then if: the if stays at the else column */
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
