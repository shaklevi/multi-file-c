#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "testfw.h"

#define MAX_CASES 256

struct tcase {
    const char *suite, *name;
    int failed;
    char msg[256];
};

static struct tcase cases[MAX_CASES];
static size_t ncases;
static struct tcase *cur;

void testfw_run(const char *suite, const char *name, void (*fn)(void))
{
    if (ncases >= MAX_CASES) {
        fprintf(stderr, "too many test cases\n");
        return;
    }
    cur = &cases[ncases++];
    cur->suite = suite;
    cur->name = name;
    fn();
    printf("[%s] %s.%s%s%s\n", cur->failed ? "FAIL" : " OK ", suite, name,
           cur->failed ? " -> " : "", cur->failed ? cur->msg : "");
    cur = NULL;
}

void testfw_fail(const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    int off;

    if (cur == NULL || cur->failed)
        return;
    cur->failed = 1;
    off = snprintf(cur->msg, sizeof cur->msg, "%s:%d: ", file, line);
    if (off < 0 || (size_t)off >= sizeof cur->msg)
        return;
    va_start(ap, fmt);
    vsnprintf(cur->msg + off, sizeof cur->msg - (size_t)off, fmt, ap);
    va_end(ap);
}

static void xml_escape(FILE *f, const char *s)
{
    for (; *s; s++) {
        switch (*s) {
        case '&': fputs("&amp;", f); break;
        case '<': fputs("&lt;", f); break;
        case '>': fputs("&gt;", f); break;
        case '"': fputs("&quot;", f); break;
        default:  fputc(*s, f);
        }
    }
}

int testfw_report(const char *xml_path)
{
    int failures = 0;

    for (size_t i = 0; i < ncases; i++)
        failures += cases[i].failed;

    printf("\n%zu tests, %d failures\n", ncases, failures);

    if (xml_path != NULL) {
        FILE *f = fopen(xml_path, "w");
        if (f == NULL) {
            perror(xml_path);
            return failures ? failures : 1;
        }
        fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        fprintf(f, "<testsuite name=\"fibtool\" tests=\"%zu\" failures=\"%d\">\n",
                ncases, failures);
        for (size_t i = 0; i < ncases; i++) {
            fprintf(f, "  <testcase classname=\"%s\" name=\"%s\"",
                    cases[i].suite, cases[i].name);
            if (cases[i].failed) {
                fprintf(f, ">\n    <failure message=\"");
                xml_escape(f, cases[i].msg);
                fprintf(f, "\"/>\n  </testcase>\n");
            } else {
                fprintf(f, "/>\n");
            }
        }
        fprintf(f, "</testsuite>\n");
        fclose(f);
    }
    return failures;
}
