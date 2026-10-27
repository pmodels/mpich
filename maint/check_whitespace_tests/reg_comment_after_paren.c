/* expect: none */
int f(char *str)
{
    while (/*(isascii(*str) &&
                 * !isspace(*str)) && */
              *str != 0)
        str++;
    return 0;
}
