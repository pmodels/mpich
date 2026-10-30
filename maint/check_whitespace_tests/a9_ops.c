/* expect: A9 */
struct s {
    int a;
};

int f(int a, int b, struct s *p, struct s t)
{
    int x = a + b;
    x += a << 2;
    x = (a == b) ? -a : !b;
    x = a && b || ~a;
    x = p->a + t.a + a++ - --b + &a != 0;
    x=a;
    x = a+b;
    x = a -b;
    x = a == b ?a : b;
    x = - a;
    x = ! b;
    x = a ++;
    x = p -> a;
    x = t. a;
    x = a&&b;
    return x;
}
