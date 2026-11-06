/* expect: B1 */
int f(int x)
{
    if (x) {
        x++;
    }
    else {
        x--;
    }
    if (x)
    {
        x++;
    }
    return x;
}
