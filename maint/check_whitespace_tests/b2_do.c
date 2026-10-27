/* expect: B2 */
int f(int x)
{
    do
    {
        x++;
    } while (x);
    return x;
}
