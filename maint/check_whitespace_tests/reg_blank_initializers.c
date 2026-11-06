/* expect: none */
int a[2] = { 1, 2 };
int b[2] = { 3, 4 };

int c;
int g(void)
{
    return 0;
}
int d[1] = { 0 };

#ifdef X
int e[1] = { 0 };
#else
int e[1] = { 1 };
#endif
