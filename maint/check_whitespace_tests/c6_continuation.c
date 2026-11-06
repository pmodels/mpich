/* expect: C6 */
int foo_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(int aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,
                                                 int bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb);
int g(void)
{
    x = foo_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,
                                                     bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb);
    x = foo_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa(aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,
            bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb);
    y = aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa +
        bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb + cccccccccccccccccccccccccccccccccccccccc;
    y = aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa +
      bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb + cccccccccccccccccccccccccccccccccccccccc;
    if (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa &&
        (bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb_dddddddddddddddddddd ||
         cccccccccccccccccccccccccccccccccccccccc)) {
        z = 1;
    }
    if (aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa &&
        (bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb_dddddddddddddddddddd ||
        cccccccccccccccccccccccccccccccccccccccc)) {
        z = 1;
    }
    return aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa[bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb +
                                                    cccccccccccccccccccccccccccccccccccccccc];
}
