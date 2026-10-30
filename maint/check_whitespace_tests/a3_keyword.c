/* expect: A3 */
int f(int x)
{
    if(x)
        x++;
    while  (x)
        x--;
    x = sizeof (int);
    return(x);
}
