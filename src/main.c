#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* getopt */

#include "fibonacci.h"
#include "fibutil.h"
#include "parse.h"
#include "primes.h"

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [-f | -p] [-n LIMIT] [-v] [-V] [-h]\n"
            "  -f        print Fibonacci series up to LIMIT (default)\n"
            "  -p        print primes up to LIMIT\n"
            "  -n LIMIT  non-interactive: use LIMIT instead of prompting\n"
            "  -v        verbose logging (same as FIBTOOL_LOG=debug)\n"
            "  -V        print version and exit\n"
            "  -h        show this help\n",
            prog);
}

int main(int argc, char **argv)
{
    char line[128];
    char mode = 'f';
    const char *limit_arg = NULL;
    int limit, opt;

    flog_init_from_env();

    while ((opt = getopt(argc, argv, "fpn:vVh")) != -1) {
        switch (opt) {
        case 'f':
        case 'p': mode = (char)opt; break;
        case 'n': limit_arg = optarg; break;
        case 'v': flog_set_level(FLOG_DEBUG); break;
        case 'V': printf("fibtool %s\n", fibutil_version()); return 0;
        case 'h': usage(argv[0]); return 0;
        default:  usage(argv[0]); return 2;
        }
    }

    if (limit_arg == NULL) {
        printf("Enter an integer: ");
        fflush(stdout);
        if (fgets(line, sizeof line, stdin) == NULL) {
            flog_msg(FLOG_ERROR, "no input");
            return 1;
        }
        limit_arg = line;
    }

    if (!parse_int(limit_arg, &limit)) {
        flog_msg(FLOG_ERROR, "Invalid integer.");
        return 1;
    }
    flog_msg(FLOG_DEBUG, "parsed limit=%d mode=%c", limit, mode);

    if (mode == 'p') {
        if (print_primes(limit) != 0) {
            flog_msg(FLOG_ERROR, "limit must be between 0 and %d", PRIMES_MAX_LIMIT);
            return 1;
        }
    } else {
        printFibonacci(limit);
    }
    return 0;
}
