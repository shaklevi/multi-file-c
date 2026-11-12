#include <stdio.h>
#include "fibonacci.h"

int main(void)
{
    char line[128];
    int limit;
    printf("Program  begins!");
    printf("Enter an integer: ");
    fgets(line, sizeof(line), stdin);

    if (!parse_int(line, &limit))
    {
        fprintf(stderr, "Invalid integer.\n");
        return 1;
    }

    printf("Parsed: %d\n", limit);

     printFibonacci(limit);

    return 0;
}
