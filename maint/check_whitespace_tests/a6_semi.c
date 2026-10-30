/* expect: A6 */
int f(int x)
{
    g(x) ;
    h(x , 1);
    for (;;);
    return 0;
}
