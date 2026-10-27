/* expect: C3 */
int f(int x)
{
    x++;
fn_exit:
    return x;
    fn_fail:
    goto fn_exit;
}
