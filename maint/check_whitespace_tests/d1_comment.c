/* expect: D1 */
int f(int x)
{
      /* misindented */
    x++;
    if (x)
      /* between if and body */
        x++;
/* column 0 is fine */
    return x;
}

enum {
  /* in an enum */
    E1
};
