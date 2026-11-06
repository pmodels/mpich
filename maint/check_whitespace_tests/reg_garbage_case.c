/* expect: none */
/* a stray token before case: not C, indent leaves it, the checker gives up */
int f(int x)
{
    switch (x) {
        case 1:
            {
                x++;
                break;
            }
            n case 2: {
                x--;
                break;
            } case 3: {
                x++;
                break;
            }
    }
    return x;
}
