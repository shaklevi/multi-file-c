/* Map validation: map_check(), is_rectangular(), is_enclosed_in_walls(). */
#include "test_common.h"
#include <stdlib.h>

#define OUTSZ 4096

/* Maps are written WITHOUT a trailing newline, like the shipped levels. */
#define VALID_MAP \
	"11111\n"     \
	"1P0C1\n"     \
	"1E001\n"     \
	"11111"

/* Convenience: run map_check on `map`, assert it returned normally with `expected_result`. */
static void	expect_result(const char *map, int expected_result, const char *msg)
{
	char	out[OUTSZ];
	int		result;
	int		code = check_map(map, &result, out, sizeof(out));

	ASSERT_EQ_INT(code, RET_NORMAL);
	ASSERT_EQ_INT(result, expected_result);
	if (msg)
		ASSERT_CONTAINS(out, msg);
}

/* ---- accepted maps ---------------------------------------------------- */

TEST(map_check, accepts_valid_map)
{
	expect_result(VALID_MAP, 0, NULL);
}

TEST(map_check, accepts_readme_example_map)
{
	expect_result("11111111111\n10000P1C001\n11111011101\n"
		"10001000001\n101011111E1\n11111111111", 0, NULL);
}

TEST(map_check, rejects_map_too_small_for_all_elements)
{
	/* 3x3 has room for only one inner cell: cannot hold P, E and C. */
	expect_result("111\n1P1\n111", 1, "exit");
}

TEST(map_check, accepts_large_map_over_1kb)
{
	/* 40x40 = 1639 bytes: spans more than one 1024-byte read() chunk. */
	char	map[2048];
	int		w = 40, h = 40, x, y, n = 0;

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			char c = '0';
			if (y == 0 || y == h - 1 || x == 0 || x == w - 1) c = '1';
			if (x == 1 && y == 1) c = 'P';
			if (x == 5 && y == 2) c = 'C';
			if (x == 7 && y == 3) c = 'E';
			map[n++] = c;
		}
		if (y < h - 1)
			map[n++] = '\n';
	}
	map[n] = '\0';
	expect_result(map, 0, NULL);
}

TEST(map_check, accepts_map_with_trailing_newline)
{
	/* Normal text files end with '\n'; editors add it automatically. */
	expect_result(VALID_MAP "\n", 0, NULL);
}

/* ---- element counting -------------------------------------------------- */

TEST(map_check, rejects_zero_players)
{
	expect_result("11111\n100C1\n1E001\n11111", 1, "exactly one player");
}

TEST(map_check, rejects_two_players)
{
	expect_result("11111\n1P0C1\n1E00P\n11111", 1, "exactly one player");
}

TEST(map_check, rejects_zero_exits)
{
	expect_result("11111\n1P0C1\n10001\n11111", 1, "exactly one exit");
}

TEST(map_check, rejects_two_exits)
{
	expect_result("11111\n1P0C1\n1E0E1\n11111", 1, "exactly one exit");
}

TEST(map_check, rejects_zero_collectibles)
{
	expect_result("11111\n1P001\n1E001\n11111", 1, "at least one collectible");
}

TEST(map_check, accepts_many_collectibles)
{
	expect_result("1111111\n1P0CCC1\n1E0CCC1\n1111111", 0, NULL);
}

TEST(map_check, rejects_invalid_character_and_exits_with_failure)
{
	char	out[OUTSZ];
	int		result;
	int		code = check_map("11111\n1P0X1\n1E0C1\n11111", &result, out, sizeof(out));

	ASSERT_EQ_INT(code, EXIT_FAILURE);
	ASSERT_CONTAINS(out, "Invalid character 'X'");
}

TEST(map_check, rejects_lowercase_element_letters)
{
	char	out[OUTSZ];
	int		result;
	int		code = check_map("11111\n1p0C1\n1E001\n11111", &result, out, sizeof(out));

	ASSERT_EQ_INT(code, EXIT_FAILURE);
	ASSERT_CONTAINS(out, "Invalid character 'p'");
}

TEST(map_check, rejects_spaces_in_map)
{
	char	out[OUTSZ];
	int		result;
	int		code = check_map("11111\n1P C1\n1E001\n11111", &result, out, sizeof(out));

	ASSERT_EQ_INT(code, EXIT_FAILURE);
	ASSERT_CONTAINS(out, "Invalid character");
}

TEST(map_check, rejects_empty_file)
{
	expect_result("", 1, "exactly one player");
}

/* ---- shape ------------------------------------------------------------- */

TEST(map_check, rejects_non_rectangular_short_middle_line)
{
	expect_result("11111\n1P0C1\n1E01\n11111", 1, "not rectangular");
}

TEST(map_check, rejects_non_rectangular_long_line)
{
	expect_result("11111\n1P0C11\n1E001\n11111", 1, "not rectangular");
}

TEST(map_check, rejects_non_rectangular_short_last_line)
{
	expect_result("11111\n1P0C1\n1E001\n1111", 1, "not rectangular");
}

TEST(map_check, rejects_non_rectangular_long_last_line)
{
	expect_result("11111\n1P0C1\n1E001\n111111", 1, "not rectangular");
}

/* ---- walls ------------------------------------------------------------- */

TEST(map_check, rejects_hole_in_top_wall)
{
	expect_result("11011\n1P0C1\n1E001\n11111", 1, "not enclosed");
}

TEST(map_check, rejects_hole_in_left_wall)
{
	expect_result("11111\n0P0C1\n1E001\n11111", 1, "not enclosed");
}

TEST(map_check, rejects_hole_in_right_wall)
{
	expect_result("11111\n1P0C1\n1E000\n11111", 1, "not enclosed");
}

TEST(map_check, rejects_hole_in_bottom_wall)
{
	/* Bottom row has a gap. The player could walk out of the map. */
	expect_result("11111\n1P0C1\n1E001\n11011", 1, "not enclosed");
}

TEST(map_check, rejects_player_on_border)
{
	expect_result("11111\nPE0C1\n10001\n11111", 1, "not enclosed");
}

/* ---- I/O --------------------------------------------------------------- */

static void	missing_file_child(void *arg)
{
	t_vars	v;

	(void)arg;
	init_vars(&v, "/nonexistent/dir/no_such_map.ber");
	map_check(&v);
}

TEST(map_check, missing_file_exits_with_failure)
{
	char	out[OUTSZ];
	int		code = run_isolated(missing_file_child, NULL, out, sizeof(out));

	ASSERT_EQ_INT(code, EXIT_FAILURE);
	ASSERT_CONTAINS(out, "Error opening map file");
}

static void	directory_child(void *arg)
{
	t_vars	v;

	(void)arg;
	init_vars(&v, "/tmp");
	map_check(&v);
}

TEST(map_check, directory_as_map_does_not_crash)
{
	char	out[OUTSZ];
	int		code = run_isolated(directory_child, NULL, out, sizeof(out));

	ASSERT_TRUE(code >= 0); /* never killed by a signal */
}

TEST(map_check, is_rectangular_accepts_empty_and_single_line)
{
	int		fd;
	char	*p = tmp_file("");

	fd = open(p, O_RDONLY);
	ASSERT_EQ_INT(is_rectangular(fd), 1);
	p = tmp_file("1111");
	fd = open(p, O_RDONLY);
	ASSERT_EQ_INT(is_rectangular(fd), 1);
}

TEST(map_check, is_enclosed_accepts_all_wall_map)
{
	char	*p = tmp_file("111\n111\n111");
	int		fd = open(p, O_RDONLY);

	ASSERT_EQ_INT(is_enclosed_in_walls(fd), 1);
}

const t_test	map_check_tests[] = {
	T(map_check, accepts_valid_map),
	T(map_check, accepts_readme_example_map),
	T(map_check, rejects_map_too_small_for_all_elements),
	KNOWN_BUG(map_check, accepts_large_map_over_1kb,
		"maps larger than 1023 bytes are wrongly rejected as 'not enclosed' "
		"(is_enclosed_in_walls() treats every 1KB read() chunk as a whole map)"),
	KNOWN_BUG(map_check, accepts_map_with_trailing_newline,
		"a map file ending in '\\n' is rejected as 'not rectangular' "
		"(is_rectangular() compares the empty last line with the first)"),
	T(map_check, rejects_zero_players),
	T(map_check, rejects_two_players),
	T(map_check, rejects_zero_exits),
	T(map_check, rejects_two_exits),
	T(map_check, rejects_zero_collectibles),
	T(map_check, accepts_many_collectibles),
	T(map_check, rejects_invalid_character_and_exits_with_failure),
	T(map_check, rejects_lowercase_element_letters),
	T(map_check, rejects_spaces_in_map),
	T(map_check, rejects_empty_file),
	T(map_check, rejects_non_rectangular_short_middle_line),
	T(map_check, rejects_non_rectangular_long_line),
	T(map_check, rejects_non_rectangular_short_last_line),
	T(map_check, rejects_non_rectangular_long_last_line),
	T(map_check, rejects_hole_in_top_wall),
	T(map_check, rejects_hole_in_left_wall),
	T(map_check, rejects_hole_in_right_wall),
	KNOWN_BUG(map_check, rejects_hole_in_bottom_wall,
		"the last map row is never validated when the file has no trailing "
		"newline, so a map with an open bottom wall is accepted"),
	T(map_check, rejects_player_on_border),
	T(map_check, missing_file_exits_with_failure),
	T(map_check, directory_as_map_does_not_crash),
	T(map_check, is_rectangular_accepts_empty_and_single_line),
	T(map_check, is_enclosed_accepts_all_wall_map),
};
const int	map_check_count = sizeof(map_check_tests) / sizeof(map_check_tests[0]);
