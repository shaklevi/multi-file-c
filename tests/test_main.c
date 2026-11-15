#include <stddef.h>
#include "testfw.h"

void run_parse_tests(void);
void run_fibonacci_tests(void);
void run_primes_tests(void);

int main(int argc, char **argv)
{
    run_parse_tests();
    run_fibonacci_tests();
    run_primes_tests();
    return testfw_report(argc > 1 ? argv[1] : NULL) == 0 ? 0 : 1;
}
