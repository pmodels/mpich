/* expect: C2 */
int f(int x)
{
    switch (x) {
    case 1:
        x++;
        break;
        default:
          x--;
    }
    return x;
}
