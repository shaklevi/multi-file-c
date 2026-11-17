#define _GNU_SOURCE
#include "framework.h"
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define TEST_TIMEOUT_SEC 20
#define OUT_CAP 16384

void	test_fail(const char *file, int line, const char *fmt, ...)
{
	va_list	ap;

	fprintf(stderr, "ASSERTION FAILED at %s:%d\n  ", file, line);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	fflush(stderr);
	_exit(1);
}

const char	*repo_root(void)
{
	const char	*r = getenv("MAZE_ROOT");

	return (r && *r ? r : "..");
}

char	*tmp_file(const char *content)
{
	static int	counter = 0;
	char		*path = malloc(128);
	FILE		*f;

	snprintf(path, 128, "/tmp/maze_test_%d_%d.ber", (int)getpid(), counter++);
	f = fopen(path, "wb");
	if (!f)
		FAIL("cannot create temp file %s: %s", path, strerror(errno));
	fwrite(content, 1, strlen(content), f);
	fclose(f);
	return (path);
}

int	run_isolated(void (*fn)(void *), void *arg, char *out, size_t cap)
{
	int		p[2];
	pid_t	pid;
	size_t	len = 0;
	ssize_t	n;
	int		status;
	char	scratch[4096];

	if (pipe(p) == -1)
		FAIL("pipe: %s", strerror(errno));
	fflush(NULL);
	pid = fork();
	if (pid == 0)
	{
		alarm(TEST_TIMEOUT_SEC);
		close(p[0]);
		dup2(p[1], 1);
		dup2(p[1], 2);
		close(p[1]);
		fn(arg);
		fflush(NULL);
		_exit(RET_NORMAL);
	}
	close(p[1]);
	while ((n = read(p[0], scratch, sizeof(scratch))) > 0)
	{
		size_t	room = len + 1 < cap ? cap - 1 - len : 0;
		size_t	take = (size_t)n < room ? (size_t)n : room;

		memcpy(out + len, scratch, take); /* keep reading even when full so */
		len += take;                      /* the child never blocks on write */
	}
	close(p[0]);
	out[len] = '\0';
	waitpid(pid, &status, 0);
	if (WIFSIGNALED(status))
		return (-WTERMSIG(status));
	return (WEXITSTATUS(status));
}

/* ---- runner ----------------------------------------------------------- */

typedef enum { R_PASS, R_FAIL, R_XFAIL, R_XPASS } t_result;

typedef struct
{
	const t_test	*test;
	t_result		result;
	char			*output;
	double			secs;
}	t_record;

static void	xml_escape(FILE *f, const char *s)
{
	for (; *s; s++)
	{
		if (*s == '<') fputs("&lt;", f);
		else if (*s == '>') fputs("&gt;", f);
		else if (*s == '&') fputs("&amp;", f);
		else if (*s == '"') fputs("&quot;", f);
		else if ((unsigned char)*s < 0x20 && *s != '\n' && *s != '\t')
			fputc('?', f);
		else fputc(*s, f);
	}
}

static void	call_test(void *arg)
{
	((t_test_fn)arg)();
}

static t_result	run_one(const t_test *t, char **output, double *secs)
{
	static char		out[OUT_CAP];
	struct timespec	t0, t1;
	int				code;

	clock_gettime(CLOCK_MONOTONIC, &t0);
	code = run_isolated(call_test, (void *)t->fn, out, sizeof(out));
	clock_gettime(CLOCK_MONOTONIC, &t1);
	*secs = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
	if (code == RET_NORMAL)
		*output = strdup(out);
	else
	{
		char	extra[256];

		if (code < 0)
			snprintf(extra, sizeof(extra), "\n[test process killed by signal %d (%s)]\n",
				-code, strsignal(-code));
		else
			snprintf(extra, sizeof(extra),
				"\n[test process exited early with code %d instead of returning]\n", code);
		*output = malloc(strlen(out) + strlen(extra) + 1);
		strcpy(*output, out);
		strcat(*output, extra);
	}
	if (t->known_bug)
		return (code == RET_NORMAL ? R_XPASS : R_XFAIL);
	return (code == RET_NORMAL ? R_PASS : R_FAIL);
}

static void	write_junit(const char *path, t_record *rec, int n, int strict)
{
	FILE	*f = fopen(path, "w");
	int		fails = 0, skips = 0, i;

	if (!f)
	{
		perror("cannot write junit file");
		return ;
	}
	for (i = 0; i < n; i++)
	{
		if (rec[i].result == R_FAIL || (strict && rec[i].result == R_XFAIL))
			fails++;
		else if (rec[i].result == R_XFAIL)
			skips++;
	}
	fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(f, "<testsuite name=\"maze-escape-unit\" tests=\"%d\" failures=\"%d\" "
		"skipped=\"%d\">\n", n, fails, skips);
	for (i = 0; i < n; i++)
	{
		const t_test	*t = rec[i].test;

		fprintf(f, "  <testcase classname=\"%s\" name=\"%s\" time=\"%.3f\">\n",
			t->suite, t->name, rec[i].secs);
		if (rec[i].result == R_FAIL || (strict && rec[i].result == R_XFAIL))
		{
			fprintf(f, "    <failure message=\"%s\"><![CDATA[", t->known_bug
				? "known bug still present (STRICT_KNOWN_BUGS=1)" : "test failed");
			fputs(rec[i].output, f);
			fprintf(f, "]]></failure>\n");
		}
		else if (rec[i].result == R_XFAIL)
		{
			fprintf(f, "    <skipped message=\"KNOWN BUG: ");
			xml_escape(f, t->known_bug);
			fprintf(f, "\"/>\n");
		}
		fprintf(f, "  </testcase>\n");
	}
	fprintf(f, "</testsuite>\n");
	fclose(f);
}

int	run_all(const t_test *tests, int n, const char *filter, const char *junit)
{
	t_record	*rec = calloc((size_t)n, sizeof(*rec));
	int			strict = getenv("STRICT_KNOWN_BUGS")
		&& strcmp(getenv("STRICT_KNOWN_BUGS"), "1") == 0;
	int			ran = 0, pass = 0, fail = 0, xfail = 0, xpass = 0, i;

	for (i = 0; i < n; i++)
	{
		char	full[256];

		snprintf(full, sizeof(full), "%s.%s", tests[i].suite, tests[i].name);
		if (filter && !strstr(full, filter))
			continue ;
		rec[ran].test = &tests[i];
		rec[ran].result = run_one(&tests[i], &rec[ran].output, &rec[ran].secs);
		switch (rec[ran].result)
		{
			case R_PASS:  pass++;  printf("[  PASS ] %s\n", full); break ;
			case R_FAIL:  fail++;  printf("[  FAIL ] %s\n%s\n", full, rec[ran].output); break ;
			case R_XFAIL: xfail++; printf("[ XFAIL ] %s  -- KNOWN BUG: %s\n", full,
				tests[i].known_bug); break ;
			case R_XPASS: xpass++; printf("[ XPASS ] %s  -- known bug seems FIXED, "
				"remove its KNOWN_BUG marker\n", full); break ;
		}
		ran++;
	}
	printf("\n==== %d tests: %d passed, %d failed, %d known-bug (expected fail), "
		"%d known-bug now passing ====\n", ran, pass, fail, xfail, xpass);
	if (strict && xfail)
		printf("STRICT_KNOWN_BUGS=1: known bugs count as failures\n");
	if (junit)
		write_junit(junit, rec, ran, strict);
	return (fail > 0 || (strict && xfail > 0));
}
