#ifndef FIBTOOL_TESTFW_H
#define FIBTOOL_TESTFW_H

/* A tiny test framework that writes JUnit XML, so Jenkins' `junit` step
 * can display the results. */

void testfw_run(const char *suite, const char *name, void (*fn)(void));
void testfw_fail(const char *file, int line, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));
/** writes JUnit XML to xml_path (if non-NULL); returns number of failures */
int testfw_report(const char *xml_path);

#define RUN_TEST(suite, fn) testfw_run((suite), #fn, (fn))

#define ASSERT_TRUE(cond)                                                    \
    do {                                                                     \
        if (!(cond)) {                                                       \
            testfw_fail(__FILE__, __LINE__, "assertion failed: %s", #cond);  \
            return;                                                          \
        }                                                                    \
    } while (0)

#define ASSERT_EQ(expected, actual)                                          \
    do {                                                                     \
        long long e_ = (long long)(expected);                                \
        long long a_ = (long long)(actual);                                  \
        if (e_ != a_) {                                                      \
            testfw_fail(__FILE__, __LINE__, "%s: expected %lld, got %lld",   \
                        #actual, e_, a_);                                    \
            return;                                                          \
        }                                                                    \
    } while (0)

#endif /* FIBTOOL_TESTFW_H */
