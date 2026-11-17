#include "test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TESTS 256

/* usage: unit_tests [--junit FILE] [--filter SUBSTRING] [--list] */
int	main(int argc, char **argv)
{
	static t_test	all[MAX_TESTS];
	const char		*junit = NULL, *filter = NULL;
	int				n = 0, i, list = 0;

	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "--junit") && i + 1 < argc)
			junit = argv[++i];
		else if (!strcmp(argv[i], "--filter") && i + 1 < argc)
			filter = argv[++i];
		else if (!strcmp(argv[i], "--list"))
			list = 1;
		else
		{
			fprintf(stderr, "usage: %s [--junit FILE] [--filter STR] [--list]\n", argv[0]);
			return (2);
		}
	}
	memcpy(all + n, map_check_tests, sizeof(t_test) * (size_t)map_check_count);
	n += map_check_count;
	memcpy(all + n, map_info_tests, sizeof(t_test) * (size_t)map_info_count);
	n += map_info_count;
	memcpy(all + n, game_tests, sizeof(t_test) * (size_t)game_count);
	n += game_count;
	memcpy(all + n, asset_tests, sizeof(t_test) * (size_t)asset_count);
	n += asset_count;
	if (list)
	{
		for (i = 0; i < n; i++)
			printf("%s.%s\n", all[i].suite, all[i].name);
		return (0);
	}
	printf("maze-escape unit tests (root: %s)\n\n", repo_root());
	return (run_all(all, n, filter, junit));
}
