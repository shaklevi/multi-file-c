#include <stdio.h>
#include "int_parser.h"

int main(void)
{
    char line[128];
    int value;
    printf("Enter an integer: ");
    fgets(line, sizeof(line), stdin);

    if (!parse_int(line, &value))
    {
        fprintf(stderr, "Invalid integer.\n");
        return 1;
    }

    printf("Parsed: %d\n", value);
    return 0;
}