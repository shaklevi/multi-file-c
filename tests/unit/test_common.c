#include "test_common.h"
#include <stdio.h>
#include <stdlib.h>

void	init_vars(t_vars *v, const char *path)
{
	memset(v, 0, sizeof(*v));
	v->path = (char *)path;
	v->state.tile_size = TILE;
	v->graphics.mlx = (void *)0x1;
	v->graphics.win = (void *)0x2;
	v->graphics.bg_img = IMG_BG;
	v->graphics.player_img = IMG_PLAYER;
	v->graphics.exit_img = IMG_EXIT;
	v->graphics.item_img = IMG_ITEM;
	v->graphics.wall_img = IMG_WALL;
}

void	start_game(t_vars *v, const char *path)
{
	init_vars(v, path);
	if (map_check(v) != 0)
		FAIL("map_check rejected %s", path);
	get_map_info(v);
	v->state.tile_size = TILE;
	mock_mlx_reset();
	read_and_display_map(v);
}

typedef struct { const char *path; } t_check_arg;

static void	check_child(void *arg)
{
	t_vars	v;
	int		r;
	char	buf[64];
	int		n;

	init_vars(&v, ((t_check_arg *)arg)->path);
	r = map_check(&v);
	n = snprintf(buf, sizeof(buf), "RESULT=%d\n", r);
	if (write(1, buf, (size_t)n) < 0)
		_exit(1);
}

int	check_map(const char *content, int *result, char *out, size_t cap)
{
	t_check_arg	a;
	int			code;
	char		*p;

	a.path = tmp_file(content);
	code = run_isolated(check_child, &a, out, cap);
	*result = -1;
	p = strstr(out, "RESULT=");
	if (p)
		*result = atoi(p + 7);
	unlink(a.path);
	return (code);
}

void	*last_img_at(int x, int y)
{
	int	i;

	for (i = g_put_count - 1; i >= 0; i--)
		if (g_puts[i].x == x && g_puts[i].y == y)
			return (g_puts[i].img);
	return (NULL);
}

int	key_for(char c)
{
	if (c == 'w') return (KEY_W);
	if (c == 'a') return (KEY_A);
	if (c == 's') return (KEY_S);
	if (c == 'd') return (KEY_D);
	if (c == 'q') return (KEY_ESC);
	return (120); /* 'x': a key the game does not use */
}

void	press_keys(t_vars *v, const char *keys)
{
	for (; *keys; keys++)
		move_player(key_for(*keys), v);
}

typedef struct { const char *map; const char *keys; } t_play;

static void	play_child(void *arg)
{
	t_play	*p = arg;
	t_vars	v;

	start_game(&v, p->map);
	press_keys(&v, p->keys);
}

int	play_game_file(const char *map_path, const char *keys, char *out, size_t cap)
{
	t_play	p;

	p.map = map_path;
	p.keys = keys;
	return (run_isolated(play_child, &p, out, cap));
}

int	play_game(const char *map_content, const char *keys, char *out, size_t cap)
{
	return (play_game_file(tmp_file(map_content), keys, out, cap));
}
