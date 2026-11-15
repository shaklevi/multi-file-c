#include <stdio.h>

#include "fibonacci.h"

size_t fibonacci_series(int limit, long long *buf, size_t cap)
{
    long long first = 0, second = 1, next;
    size_t n = 0;

    if (limit < 0)
        return 0;

    if (buf != NULL && cap > 0)
        buf[0] = first;
    n = 1;

    /* long long: first + second cannot overflow while next <= INT_MAX */
    next = first + second;
    while (next <= limit) {
        if (buf != NULL && n < cap)
            buf[n] = next;
        n++;
        first = second;
        second = next;
        next = first + second;
    }
    return n;
}

void printFibonacci(int limit)
{
    long long terms[96];
    size_t n = fibonacci_series(limit, terms, sizeof terms / sizeof terms[0]);

    printf("Fibonacci Series up to %d:", limit);
    for (size_t i = 0; i < n; i++)
        printf("%s %lld", i ? "," : "", terms[i]);
    printf("\n");
}
