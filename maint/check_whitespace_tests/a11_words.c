/* expect: A11 */
static unsigned long x;
static  int y;
unsigned  long z;

int f(void)
{
    const char *s = "a" "b";
    return  y + (int) z + (s != 0) + (int) x;
}
