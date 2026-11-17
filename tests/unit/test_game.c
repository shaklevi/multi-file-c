/* Gameplay: move_player(), collectibles, exit rules, rendering, ESC. */
#include "test_common.h"
#include <stdlib.h>

#define OUTSZ 4096

/* P(1,1)  C(3,1)  E(1,2); 24 tiles = 16 walls + 5 floor + P + C + E */
#define GAME_MAP \
	"111111\n"   \
	"1P0C01\n"   \
	"1E0001\n"   \
	"111111"

/* Two collectibles: C(3,1) and C(5,1); E(1,2). */
#define TWO_ITEMS_MAP \
	"1111111\n"       \
	"1P0C0C1\n"       \
	"1E00001\n"       \
	"1111111"

/* Open room, P in the middle. */
#define ARENA_MAP \
	"1111111\n"   \
	"1000001\n"   \
	"10P0001\n"   \
	"10C0E01\n"   \
	"1111111"

/* ---- setup / rendering --------------------------------------------------- */

TEST(game, keycodes_are_distinct_and_nonzero)
{
	ASSERT_TRUE(KEY_W != 0 && KEY_A != 0 && KEY_S != 0 && KEY_D != 0 && KEY_ESC != 0);
	ASSERT_TRUE(KEY_W != KEY_A && KEY_W != KEY_S && KEY_W != KEY_D);
	ASSERT_TRUE(KEY_A != KEY_S && KEY_A != KEY_D && KEY_S != KEY_D);
	ASSERT_TRUE(KEY_ESC != KEY_W && KEY_ESC != KEY_A && KEY_ESC != KEY_S
		&& KEY_ESC != KEY_D);
}

TEST(game, initial_state_is_read_from_map)
{
	t_vars	v;

	start_game(&v, tmp_file(GAME_MAP));
	ASSERT_EQ_INT(v.state.current_x, 1 * TILE);
	ASSERT_EQ_INT(v.state.current_y, 1 * TILE);
	ASSERT_EQ_INT(v.state.start_pos_x, 1 * TILE);
	ASSERT_EQ_INT(v.state.start_pos_y, 1 * TILE);
	ASSERT_EQ_INT(v.state.initial_position_set, 1);
	ASSERT_EQ_INT(v.state.move_count, 0);
	ASSERT_EQ_INT(v.state.collectible_count, 0);
}

TEST(game, first_frame_draws_every_tile_with_the_right_texture)
{
	t_vars	v;
	int		i, walls = 0, bgs = 0, items = 0, exits = 0, players = 0;

	start_game(&v, tmp_file(GAME_MAP));
	ASSERT_EQ_INT(g_put_count, 24);
	for (i = 0; i < g_put_count; i++)
	{
		if (g_puts[i].img == IMG_WALL) walls++;
		else if (g_puts[i].img == IMG_BG) bgs++;
		else if (g_puts[i].img == IMG_ITEM) items++;
		else if (g_puts[i].img == IMG_EXIT) exits++;
		else if (g_puts[i].img == IMG_PLAYER) players++;
		else FAIL("unknown image drawn at call %d", i);
	}
	ASSERT_EQ_INT(walls, 16);
	ASSERT_EQ_INT(bgs, 5);
	ASSERT_EQ_INT(items, 1);
	ASSERT_EQ_INT(exits, 1);
	ASSERT_EQ_INT(players, 1);
	ASSERT_TRUE(last_img_at(3 * TILE, 1 * TILE) == IMG_ITEM);
	ASSERT_TRUE(last_img_at(1 * TILE, 2 * TILE) == IMG_EXIT);
	ASSERT_TRUE(last_img_at(1 * TILE, 1 * TILE) == IMG_PLAYER);
}

/* ---- movement ------------------------------------------------------------ */

TEST(game, wasd_each_move_exactly_one_tile)
{
	t_vars	v;

	start_game(&v, tmp_file(ARENA_MAP));
	ASSERT_EQ_INT(v.state.current_x, 2 * TILE);
	ASSERT_EQ_INT(v.state.current_y, 2 * TILE);
	move_player(KEY_W, &v);
	ASSERT_EQ_INT(v.state.current_x, 2 * TILE);
	ASSERT_EQ_INT(v.state.current_y, 1 * TILE);
	move_player(KEY_S, &v);
	ASSERT_EQ_INT(v.state.current_y, 2 * TILE);
	move_player(KEY_A, &v);
	ASSERT_EQ_INT(v.state.current_x, 1 * TILE);
	move_player(KEY_D, &v);
	ASSERT_EQ_INT(v.state.current_x, 2 * TILE);
	ASSERT_EQ_INT(v.state.move_count, 4);
}

TEST(game, walls_block_movement_in_all_directions)
{
	t_vars	v;

	start_game(&v, tmp_file(GAME_MAP));
	press_keys(&v, "w"); /* wall above */
	press_keys(&v, "a"); /* wall to the left */
	ASSERT_EQ_INT(v.state.current_x, 1 * TILE);
	ASSERT_EQ_INT(v.state.current_y, 1 * TILE);
	ASSERT_EQ_INT(v.state.move_count, 0);
	ASSERT_EQ_INT(g_clear_count, 0); /* a blocked move must not redraw */
}

TEST(game, blocked_moves_are_not_counted)
{
	t_vars	v;

	start_game(&v, tmp_file(ARENA_MAP));
	press_keys(&v, "wwww"); /* one step up, then three into the wall */
	ASSERT_EQ_INT(v.state.current_y, 1 * TILE);
	ASSERT_EQ_INT(v.state.move_count, 1);
}

TEST(game, player_can_never_leave_the_map)
{
	t_vars	v;

	start_game(&v, tmp_file(ARENA_MAP)); /* room is x:1..5  y:1..3 */
	press_keys(&v, "wwwwwwwwww");   /* hammer the top wall */
	ASSERT_EQ_INT(v.state.current_y, 1 * TILE);
	press_keys(&v, "aaaaaaaaaa");   /* hammer the left wall */
	ASSERT_EQ_INT(v.state.current_x, 1 * TILE);
	press_keys(&v, "dddddddddd");   /* along the top row: no items there */
	ASSERT_EQ_INT(v.state.current_x, 5 * TILE);
	press_keys(&v, "ssssssssss");   /* down the right column */
	ASSERT_EQ_INT(v.state.current_y, 3 * TILE);
	ASSERT_EQ_INT(v.state.current_x, 5 * TILE);
}

TEST(game, unused_key_is_not_counted_as_a_move)
{
	t_vars	v;

	start_game(&v, tmp_file(GAME_MAP));
	press_keys(&v, "xxx");
	ASSERT_EQ_INT(v.state.move_count, 0);
	ASSERT_EQ_INT(v.state.current_x, 1 * TILE);
	ASSERT_EQ_INT(v.state.current_y, 1 * TILE);
}

TEST(game, moving_redraws_player_and_restores_background)
{
	t_vars	v;

	start_game(&v, tmp_file(GAME_MAP));
	mock_mlx_reset();
	press_keys(&v, "d");
	ASSERT_EQ_INT(g_clear_count, 1);
	ASSERT_TRUE(last_img_at(2 * TILE, 1 * TILE) == IMG_PLAYER);
	ASSERT_TRUE(last_img_at(1 * TILE, 1 * TILE) == IMG_BG); /* start tile is empty now */
}

/* ---- collectibles --------------------------------------------------------- */

TEST(game, walking_over_a_collectible_collects_it)
{
	t_vars	v;

	start_game(&v, tmp_file(GAME_MAP));
	press_keys(&v, "dd");
	ASSERT_EQ_INT(v.state.collectible_count, 1);
	ASSERT_EQ_INT(v.state.collectible_x[0], 3 * TILE);
	ASSERT_EQ_INT(v.state.collectible_y[0], 1 * TILE);
}

TEST(game, collected_item_disappears_from_the_screen)
{
	t_vars	v;

	start_game(&v, tmp_file(GAME_MAP));
	press_keys(&v, "dda"); /* step off the collectible */
	ASSERT_TRUE(last_img_at(3 * TILE, 1 * TILE) == IMG_BG);
}

TEST(game, same_collectible_is_never_counted_twice)
{
	t_vars	v;

	start_game(&v, tmp_file(GAME_MAP));
	press_keys(&v, "ddaaddaadd");
	ASSERT_EQ_INT(v.state.collectible_count, 1);
}

TEST(game, collecting_every_item_matches_map_total)
{
	t_vars	v;

	start_game(&v, tmp_file(TWO_ITEMS_MAP));
	ASSERT_EQ_INT(v.map_info.all_collectible, 2);
	press_keys(&v, "dddd");
	ASSERT_EQ_INT(v.state.collectible_count, 2);
}

/* ---- exit rules ----------------------------------------------------------- */

TEST(game, exit_is_blocked_until_the_item_is_collected)
{
	t_vars	v;

	start_game(&v, tmp_file(GAME_MAP));
	press_keys(&v, "s"); /* E is directly below the player */
	ASSERT_EQ_INT(v.state.current_y, 1 * TILE);
	ASSERT_EQ_INT(v.state.move_count, 0);
	ASSERT_EQ_INT(g_destroy_count, 0);
}

TEST(game, collecting_all_items_then_reaching_exit_wins)
{
	char	out[OUTSZ];
	int		code = play_game(GAME_MAP, "ddaas", out, sizeof(out));

	ASSERT_EQ_INT(code, 0);
	ASSERT_CONTAINS(out, "Move number:");
	ASSERT_CONTAINS(out, "destroy_window");
}

TEST(game, exit_stays_closed_when_only_some_items_are_collected)
{
	char	out[OUTSZ];
	/* collect the first item only, go back and try the exit */
	int		code = play_game(TWO_ITEMS_MAP, "ddaaas", out, sizeof(out));

	ASSERT_EQ_INT(code, RET_NORMAL); /* still playing */
	ASSERT_NOT_CONTAINS(out, "Move number:");
}

TEST(game, collecting_both_items_opens_the_exit)
{
	char	out[OUTSZ];
	int		code = play_game(TWO_ITEMS_MAP, "ddddaaaas", out, sizeof(out));

	ASSERT_EQ_INT(code, 0);
	ASSERT_CONTAINS(out, "Move number:");
}

TEST(game, reported_move_count_includes_the_winning_move)
{
	char	out[OUTSZ];
	/* d d a a s = 5 moves, the last one steps onto the exit */
	int		code = play_game(GAME_MAP, "ddaas", out, sizeof(out));

	ASSERT_EQ_INT(code, 0);
	ASSERT_CONTAINS(out, "Move number: 5");
}

TEST(game, blocked_attempts_do_not_inflate_final_move_count)
{
	char	out[OUTSZ];
	/* same route plus 3 bumps into walls and 1 early exit attempt */
	int		code = play_game(GAME_MAP, "wwsddaas", out, sizeof(out));

	ASSERT_EQ_INT(code, 0);
	ASSERT_CONTAINS(out, "Move number: 5");
}

TEST(game, escape_closes_the_window_and_exits_cleanly)
{
	char	out[OUTSZ];
	int		code = play_game(GAME_MAP, "q", out, sizeof(out));

	ASSERT_EQ_INT(code, 0);
	ASSERT_CONTAINS(out, "destroy_window");
	ASSERT_NOT_CONTAINS(out, "Move number:");
}

TEST(game, close_window_destroys_window_and_exits_zero)
{
	char	out[OUTSZ];
	int		code = play_game(GAME_MAP, "dq", out, sizeof(out));

	ASSERT_EQ_INT(code, 0);
	ASSERT_CONTAINS(out, "destroy_window");
}

const t_test	game_tests[] = {
	T(game, keycodes_are_distinct_and_nonzero),
	T(game, initial_state_is_read_from_map),
	T(game, first_frame_draws_every_tile_with_the_right_texture),
	T(game, wasd_each_move_exactly_one_tile),
	T(game, walls_block_movement_in_all_directions),
	T(game, blocked_moves_are_not_counted),
	T(game, player_can_never_leave_the_map),
	KNOWN_BUG(game, unused_key_is_not_counted_as_a_move,
		"pressing any key other than W/A/S/D/ESC is counted as a move "
		"(update_position() leaves the target unchanged, which looks like a free tile)"),
	T(game, moving_redraws_player_and_restores_background),
	T(game, walking_over_a_collectible_collects_it),
	T(game, collected_item_disappears_from_the_screen),
	T(game, same_collectible_is_never_counted_twice),
	T(game, collecting_every_item_matches_map_total),
	T(game, exit_is_blocked_until_the_item_is_collected),
	T(game, collecting_all_items_then_reaching_exit_wins),
	T(game, exit_stays_closed_when_only_some_items_are_collected),
	T(game, collecting_both_items_opens_the_exit),
	KNOWN_BUG(game, reported_move_count_includes_the_winning_move,
		"'Move number' is printed before the final step onto the exit is counted "
		"(off by one)"),
	KNOWN_BUG(game, blocked_attempts_do_not_inflate_final_move_count,
		"same off-by-one in the final move count"),
	T(game, escape_closes_the_window_and_exits_cleanly),
	T(game, close_window_destroys_window_and_exits_zero),
};
const int	game_count = sizeof(game_tests) / sizeof(game_tests[0]);
