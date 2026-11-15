#include "primes.h"
#include "testfw.h"

static void test_is_prime_small(void)
{
    ASSERT_TRUE(!is_prime(-7));
    ASSERT_TRUE(!is_prime(0));
    ASSERT_TRUE(!is_prime(1));
    ASSERT_TRUE(is_prime(2));
    ASSERT_TRUE(is_prime(97));
    ASSERT_TRUE(!is_prime(91)); /* 7 * 13 */
}

static void test_count_primes_known_values(void)
{
    ASSERT_EQ(25, count_primes(100));
    ASSERT_EQ(168, count_primes(1000));
    ASSERT_EQ(0, count_primes(1));
}

static void test_count_primes_rejects_bad_limit(void)
{
    ASSERT_EQ(-1, count_primes(-1));
    ASSERT_EQ(-1, count_primes(PRIMES_MAX_LIMIT + 1));
}

static void test_sieve_agrees_with_is_prime(void)
{
    long expected = 0;
    for (int i = 0; i <= 5000; i++)
        expected += is_prime(i);
    ASSERT_EQ(expected, count_primes(5000));
}

void run_primes_tests(void)
{
    RUN_TEST("primes", test_is_prime_small);
    RUN_TEST("primes", test_count_primes_known_values);
    RUN_TEST("primes", test_count_primes_rejects_bad_limit);
    RUN_TEST("primes", test_sieve_agrees_with_is_prime);
}
