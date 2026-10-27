/* expect: B5 */
int f(int x)
{
    if (x) x++;
    x++; x--;
    { x++; }
    switch (x) {
        case 1: x++;
            break;
    }
    return x;
}
