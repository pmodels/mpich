/* expect: none */
int f(void)
{
    /* QUIRK(Q25): indent adds a space: a* */
    x = 1;      /* a**/
    y = 2;      /* b */ /*c*/
    /* QUIRK(Q27): indent leaves this alone :(*/
    z = 3;
    return 0;
}
