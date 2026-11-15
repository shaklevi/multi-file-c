#include <limits.h>

#include "fibonacci.h"
#include "testfw.h"

static void test_limit_zero(void)
{
    long long buf[4];
    ASSERT_EQ(1, fibonacci_series(0, buf, 4));
    ASSERT_EQ(0, buf[0]);
}

static void test_negative_limit_is_empty(void)
{
    ASSERT_EQ(0, fibonacci_series(-5, NULL, 0));
}

static void test_series_up_to_20(void)
{
    const long long expected[] = { 0, 1, 2, 3, 5, 8, 13 };
    long long buf[16];
    size_t n = fibonacci_series(20, buf, 16);

    ASSERT_EQ(7, n);
    for (size_t i = 0; i < n; i++)
        ASSERT_EQ(expected[i], buf[i]);
}

static void test_count_only_with_null_buffer(void)
{
    ASSERT_EQ(7, fibonacci_series(20, NULL, 0));
}

static void test_small_buffer_does_not_overflow(void)
{
    long long buf[3] = { -1, -1, -1 };
    ASSERT_EQ(7, fibonacci_series(20, buf, 3)); /* reports the full count */
    ASSERT_EQ(2, buf[2]);
}

static void test_int_max_does_not_overflow(void)
{
    /* the original int-based loop had signed overflow (UB) here */
    ASSERT_TRUE(fibonacci_series(INT_MAX, NULL, 0) > 40);
}

void run_fibonacci_tests(void)
{
    RUN_TEST("fibonacci", test_limit_zero);
    RUN_TEST("fibonacci", test_negative_limit_is_empty);
    RUN_TEST("fibonacci", test_series_up_to_20);
    RUN_TEST("fibonacci", test_count_only_with_null_buffer);
    RUN_TEST("fibonacci", test_small_buffer_does_not_overflow);
    RUN_TEST("fibonacci", test_int_max_does_not_overflow);
}
