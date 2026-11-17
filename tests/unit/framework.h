/*
** Minimal, dependency-free C test framework for maze-escape.
**
**  - every test runs in its own forked process (crash / hang / exit() safe)
**  - a test passes ONLY if it returns normally (exit code RET_NORMAL)
**  - tests marked KNOWN_BUG are expected to fail; they do not break the build
**    unless STRICT_KNOWN_BUGS=1 is set. If one starts passing it is reported
**    as XPASS so the marker can be removed.
**  - results are printed to stdout and written as JUnit XML (--junit FILE)
*/
#ifndef FRAMEWORK_H
# define FRAMEWORK_H

# include <stddef.h>

# define RET_NORMAL 200 /* exit code used by the runner for "test returned" */

typedef void	(*t_test_fn)(void);

typedef struct s_test
{
	const char	*suite;
	const char	*name;
	t_test_fn	fn;
	const char	*known_bug; /* NULL for a normal test, else the reason */
}	t_test;

# define TEST(suite, name) static void suite##__##name(void)
# define T(suite, name) {#suite, #name, suite##__##name, NULL}
# define KNOWN_BUG(suite, name, why) {#suite, #name, suite##__##name, why}

void	test_fail(const char *file, int line, const char *fmt, ...)
		__attribute__((noreturn, format(printf, 3, 4)));

# define FAIL(...) test_fail(__FILE__, __LINE__, __VA_ARGS__)
# define ASSERT_TRUE(c) \
	do { if (!(c)) FAIL("expected true: %s", #c); } while (0)
# define ASSERT_EQ_INT(actual, expected) \
	do { long a_ = (long)(actual); long e_ = (long)(expected); \
		if (a_ != e_) FAIL("%s: expected %ld, got %ld", #actual, e_, a_); \
	} while (0)
# define ASSERT_EQ_CHAR(actual, expected) \
	do { int a_ = (unsigned char)(actual); int e_ = (unsigned char)(expected); \
		if (a_ != e_) FAIL("%s: expected '%c' (%d), got '%c' (%d)", \
			#actual, e_, e_, a_, a_); } while (0)
# define ASSERT_CONTAINS(haystack, needle) \
	do { if (!strstr((haystack), (needle))) \
		FAIL("expected output to contain \"%s\"; output was:\n%s", \
			(needle), (haystack)); } while (0)
# define ASSERT_NOT_CONTAINS(haystack, needle) \
	do { if (strstr((haystack), (needle))) \
		FAIL("output must not contain \"%s\"; output was:\n%s", \
			(needle), (haystack)); } while (0)

/* ---- helpers ---------------------------------------------------------- */

/* Repository root (env MAZE_ROOT, default ".."). */
const char	*repo_root(void);

/* Write `content` to a fresh temp file, return its path (never freed). */
char		*tmp_file(const char *content);

/*
** Run fn(arg) in a child process with stdout+stderr captured into out.
** Returns the child's exit code (RET_NORMAL if fn returned normally), or
** -SIGNUM if it was killed by a signal (e.g. -11 = segfault).
*/
int			run_isolated(void (*fn)(void *), void *arg, char *out, size_t cap);

/* Runs the tests; returns 0 if the build should pass, 1 otherwise. */
int			run_all(const t_test *tests, int n, const char *filter,
				const char *junit);

#endif
