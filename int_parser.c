#include <stdlib.h> // strtol
#include <errno.h>
#include <limits.h> // INT_MAX/MIN

int parse_int(const char *s, int *out)
{
    char *end;
    long val;

    errno = 0; /* clear before the call */

    val = strtol(s, &end, 10);

    if (end == s)
    {
        return 0; /* no digits read */
    }

    if (*end != '\0' && *end != '\n')
    {
        return 0; /* trailing garbage */
    }

    if (errno == ERANGE)
    {
        return 0; /* overflow/underflow */
    }

    if (val < INT_MIN || val > INT_MAX)
    {
        return 0; /* outside int range */
    }

    *out = (int)val;

    return 1;
}