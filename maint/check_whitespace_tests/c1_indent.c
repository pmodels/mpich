/* expect: C1 */
int f(int x)
{
     x++;
    if (x)
      x--;
    if (x) {
        x++;
      }
    return x;
}
