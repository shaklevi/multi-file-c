#ifndef FIBTOOL_PRIMES_H
#define FIBTOOL_PRIMES_H

#define PRIMES_MAX_LIMIT 50000000

/** \returns 1 if n is prime, 0 otherwise */
int is_prime(int n);

/** \brief counts primes <= limit using a sieve of Eratosthenes
 *
 * \returns the count, or -1 if limit is out of range / memory is unavailable
 */
long count_primes(int limit);

/** prints all primes <= limit to stdout. \returns 0 on success, -1 on error */
int print_primes(int limit);

#endif /* FIBTOOL_PRIMES_H */
