/* expect: D3 */
int f(int x)
{
    /* a
       b
     */
    x++;
    /* c
      * d
     */
    x++;
    /*
     * boxed per the manual, realigned by indent
      */
    return x;
}

#define A 1     /* a
                  * b */
