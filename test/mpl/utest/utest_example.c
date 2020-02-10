#include "utest.h"

void test_case_func()
{
    UT_TEST_CHECK(1 == 1);
}

UT_TEST_SET = {
    { "example", test_case_func },
    { NULL, NULL }
};
