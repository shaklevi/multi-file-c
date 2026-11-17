/* get_map_info() and the tile lookup used for collision (next_px). */
#include "test_common.h"

#define MAP_6x4 \
	"111111\n"  \
	"1P0C01\n"  \
	"1E0C01\n"  \
	"111111"

TEST(map_info, reports_dimensions_and_counts)
{
	t_vars	v;

	init_vars(&v, tmp_file(MAP_6x4));
	get_map_info(&v);
	ASSERT_EQ_INT(v.map_info.width, 6);
	ASSERT_EQ_INT(v.map_info.height, 4);
	ASSERT_EQ_INT(v.map_info.all_players, 1);
	ASSERT_EQ_INT(v.map_info.all_exits, 1);
	ASSERT_EQ_INT(v.map_info.all_collectible, 2);
}

TEST(map_info, ignores_stale_values_in_struct)
{
	t_vars	v;

	init_vars(&v, tmp_file(MAP_6x4));
	v.map_info.width = 99;
	v.map_info.height = 99;
	v.map_info.all_collectible = 99;
	v.map_info.all_exits = 99;
	v.map_info.all_players = 99;
	get_map_info(&v);
	ASSERT_EQ_INT(v.map_info.width, 6);
	ASSERT_EQ_INT(v.map_info.height, 4);
	ASSERT_EQ_INT(v.map_info.all_collectible, 2);
}

TEST(map_info, height_ignores_trailing_newline)
{
	t_vars	v;

	init_vars(&v, tmp_file(MAP_6x4 "\n"));
	get_map_info(&v);
	ASSERT_EQ_INT(v.map_info.height, 4);
}

TEST(map_info, matches_for_wide_map)
{
	t_vars	v;

	init_vars(&v, tmp_file("1111111111\n1P0000C0E1\n1111111111"));
	get_map_info(&v);
	ASSERT_EQ_INT(v.map_info.width, 10);
	ASSERT_EQ_INT(v.map_info.height, 3);
}

/* ---- next_px: pixel position -> map character -------------------------- */

TEST(next_px, converts_pixels_to_tile_coordinates)
{
	ASSERT_EQ_INT(calculate_tile_coordinate(0, 50), 0);
	ASSERT_EQ_INT(calculate_tile_coordinate(49, 50), 0);
	ASSERT_EQ_INT(calculate_tile_coordinate(50, 50), 1);
	ASSERT_EQ_INT(calculate_tile_coordinate(149, 50), 2);
	ASSERT_EQ_INT(calculate_tile_coordinate(150, 50), 3);
}

TEST(next_px, returns_the_character_at_each_tile)
{
	t_vars	v;

	init_vars(&v, tmp_file(MAP_6x4));
	ASSERT_EQ_CHAR(next_px(0, 0, &v), '1');
	ASSERT_EQ_CHAR(next_px(50, 50, &v), 'P');
	ASSERT_EQ_CHAR(next_px(100, 50, &v), '0');
	ASSERT_EQ_CHAR(next_px(150, 50, &v), 'C');
	ASSERT_EQ_CHAR(next_px(50, 100, &v), 'E');
	ASSERT_EQ_CHAR(next_px(150, 100, &v), 'C');
	ASSERT_EQ_CHAR(next_px(250, 150, &v), '1');
}

TEST(next_px, uses_the_tile_that_contains_the_pixel)
{
	t_vars	v;

	init_vars(&v, tmp_file(MAP_6x4));
	ASSERT_EQ_CHAR(next_px(50 + 49, 50 + 49, &v), 'P');
	ASSERT_EQ_CHAR(next_px(150 + 1, 50, &v), 'C');
}

TEST(next_px, out_of_range_returns_nul)
{
	t_vars	v;

	init_vars(&v, tmp_file(MAP_6x4));
	ASSERT_EQ_CHAR(next_px(10 * 50, 50, &v), '\0'); /* past the right edge */
	ASSERT_EQ_CHAR(next_px(50, 10 * 50, &v), '\0'); /* below the map */
}

TEST(next_px, does_not_leak_file_descriptors)
{
	t_vars	v;
	int		i;
	int		fd;

	init_vars(&v, tmp_file(MAP_6x4));
	for (i = 0; i < 2000; i++)
		next_px(50, 50, &v); /* would hit EMFILE (exit) if fds leaked */
	fd = open("/dev/null", O_RDONLY);
	ASSERT_TRUE(fd < 20);
	close(fd);
}

const t_test	map_info_tests[] = {
	T(map_info, reports_dimensions_and_counts),
	T(map_info, ignores_stale_values_in_struct),
	KNOWN_BUG(map_info, height_ignores_trailing_newline,
		"get_map_info() counts '\\n' characters, so a map ending in a newline "
		"gets one extra row (window one tile too tall)"),
	T(map_info, matches_for_wide_map),
	T(next_px, converts_pixels_to_tile_coordinates),
	T(next_px, returns_the_character_at_each_tile),
	T(next_px, uses_the_tile_that_contains_the_pixel),
	T(next_px, out_of_range_returns_nul),
	T(next_px, does_not_leak_file_descriptors),
};
const int	map_info_count = sizeof(map_info_tests) / sizeof(map_info_tests[0]);
