/* expect: D4 */
struct s {
    int a;                      /* at column 33 */
    int b;  /* at the next tab stop */
    int c; /* one space */
};

int f(int x)
{
    x = 1;      /* next tab stop */
    x = 2;                      /* column 33 */
    x = 3; /* one space */
    x = 4;          /* two tab stops */
    if (x) {    /* after a brace */
        x = 5;
    } /* before else */
    else {
        x = 6;
    }
    return x;
}
