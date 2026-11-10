/* expect: none */
int f(int x)
{
    switch (x) {
        case 1:{
                x++;
                break;
            }
        case 2:
            {
                x--;
                break;
            }
    }
    do {
        x++;
    }
    while (0);
  fn_exit:
    return x;
}
