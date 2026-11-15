#include <limits.h>

#include "parse.h"
#include "testfw.h"

static void test_valid(void)
{
    int v = 0;
    ASSERT_EQ(1, parse_int("42", &v));
    ASSERT_EQ(42, v);
    ASSERT_EQ(1, parse_int("-17\n", &v));
    ASSERT_EQ(-17, v);
}

static void test_garbage(void)
{
    int v = 123;
    ASSERT_EQ(0, parse_int("", &v));
    ASSERT_EQ(0, parse_int("abc", &v));
    ASSERT_EQ(0, parse_int("12abc", &v));
    ASSERT_EQ(123, v); /* untouched on error */
}

static void test_range(void)
{
    int v = 0;
    ASSERT_EQ(1, parse_int("2147483647", &v));
    ASSERT_EQ(INT_MAX, v);
    ASSERT_EQ(0, parse_int("2147483648", &v));
    ASSERT_EQ(0, parse_int("99999999999999999999", &v));
}

void run_parse_tests(void)
{
    RUN_TEST("parse", test_valid);
    RUN_TEST("parse", test_garbage);
    RUN_TEST("parse", test_range);
}
