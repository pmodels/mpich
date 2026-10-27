/* expect: A10 */
int g(int a, int b);

int f(int a, int b)
{
    int x, y;
    x = g(a, b);
    y = g(a,b);
    int c,d;
    c = d = g(x,  y);
    return c;
}
