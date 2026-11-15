#ifndef FIBTOOL_FIBONACCI_H
#define FIBTOOL_FIBONACCI_H

#include <stddef.h>

/** \brief computes the series 0, 1, 2, 3, 5, ... for all terms <= limit
 *
 * \param limit largest value to include
 * \param buf   optional output buffer (may be NULL to just count)
 * \param cap   capacity of buf, in elements
 *
 * \returns the number of terms in the series (may exceed cap)
 */
size_t fibonacci_series(int limit, long long *buf, size_t cap);

/** prints the series to stdout, followed by a newline */
void printFibonacci(int limit);

#endif /* FIBTOOL_FIBONACCI_H */
