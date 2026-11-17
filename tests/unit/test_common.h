#ifndef TEST_COMMON_H
# define TEST_COMMON_H

# include "../../src/header.h"
# include "framework.h"
# include "mock_mlx.h"
# include <string.h>

/* Fake image handles so tests can tell which texture was drawn. */
# define IMG_BG     ((void *)0x10)
# define IMG_PLAYER ((void *)0x11)
# define IMG_EXIT   ((void *)0x12)
# define IMG_ITEM   ((void *)0x13)
# define IMG_WALL   ((void *)0x14)
# define TILE       50

/* Functions that exist in src/ but are not declared in header.h. */
int		is_rectangular(int fd);
char	find_tile_in_map(int tile_x, int tile_y, int fd);
int		calculate_tile_coordinate(int position, int tile_size);
int		check_collectible(int next_x, int next_y, char tile, t_vars *vars);

/* Same initial state main() builds, plus fake MLX handles. */
void	init_vars(t_vars *v, const char *path);

/* Mirrors main(): map_check -> get_map_info -> draw. Fails the test if the
** map is rejected. */
void	start_game(t_vars *v, const char *path);

/*
** Runs map_check() on `content` in an isolated process.
** Returns the process exit code (RET_NORMAL if map_check returned),
** *result = map_check()'s return value (-1 if it never returned),
** out = captured output.
*/
int		check_map(const char *content, int *result, char *out, size_t cap);

/* Maps a letter to a game key: w a s d, q = ESC, anything else = an unused key. */
int		key_for(char c);
void	press_keys(t_vars *v, const char *keys);

/*
** Starts a game on the map and presses `keys` in an isolated process.
** Returns the exit code (0 = game exited via exit(0); RET_NORMAL = the game
** was still running after the last key). out = captured output.
*/
int		play_game(const char *map_content, const char *keys, char *out, size_t cap);
int		play_game_file(const char *map_path, const char *keys, char *out, size_t cap);

/* Last image drawn at pixel (x, y) since the last mock_mlx_reset(), or NULL. */
void	*last_img_at(int x, int y);

/* Test suites (each returns its own table). */
extern const t_test	map_check_tests[];
extern const int	map_check_count;
extern const t_test	map_info_tests[];
extern const int	map_info_count;
extern const t_test	game_tests[];
extern const int	game_count;
extern const t_test	asset_tests[];
extern const int	asset_count;

#endif
