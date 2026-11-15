#include <stdio.h>
#include <stdlib.h>

#include "primes.h"

int is_prime(int n)
{
    if (n < 2)
        return 0;
    for (long long d = 2; d * d <= n; d++)
        if (n % d == 0)
            return 0;
    return 1;
}

/* returns a calloc'ed table where sieve[i] == 1 means i is composite */
static unsigned char *build_sieve(int limit)
{
    unsigned char *sieve;

    if (limit < 0 || limit > PRIMES_MAX_LIMIT)
        return NULL;

    sieve = calloc((size_t)limit + 1, 1);
    if (sieve == NULL)
        return NULL;

    for (long long i = 2; i * i <= limit; i++)
        if (!sieve[i])
            for (long long j = i * i; j <= limit; j += i)
                sieve[j] = 1;
    return sieve;
}

long count_primes(int limit)
{
    long count = 0;
    unsigned char *sieve = build_sieve(limit);

    if (sieve == NULL)
        return -1;
    for (int i = 2; i <= limit; i++)
        if (!sieve[i])
            count++;
    free(sieve);
    return count;
}

int print_primes(int limit)
{
    int first = 1;
    unsigned char *sieve = build_sieve(limit);

    if (sieve == NULL)
        return -1;

    printf("Primes up to %d:", limit);
    for (int i = 2; i <= limit; i++) {
        if (!sieve[i]) {
            printf("%s %d", first ? "" : ",", i);
            first = 0;
        }
    }
    printf("\n");
    free(sieve);
    return 0;
}
