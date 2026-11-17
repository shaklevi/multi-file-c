/* Shipped content: levels/ and textures/ must be valid and playable. */
#include "test_common.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define OUTSZ 8192
#define MAX_ENTRIES 64
#define KEYS_CAP 200000

/* ---- small file helpers --------------------------------------------------- */

static char	*read_file(const char *path)
{
	FILE	*f = fopen(path, "rb");
	long	n;
	char	*buf;

	if (!f)
		return (NULL);
	fseek(f, 0, SEEK_END);
	n = ftell(f);
	fseek(f, 0, SEEK_SET);
	buf = malloc((size_t)n + 1);
	if (fread(buf, 1, (size_t)n, f) != (size_t)n)
		n = 0;
	buf[n] = '\0';
	fclose(f);
	return (buf);
}

/* Fills `names` with the non-hidden entries of dir (sorted); returns count. */
static int	list_dir(const char *dir, char names[][256], int want_dirs)
{
	DIR				*d = opendir(dir);
	struct dirent	*e;
	int				n = 0, i, j;
	char			path[1024], tmp[256];
	struct stat		st;

	if (!d)
		FAIL("cannot open directory %s", dir);
	while ((e = readdir(d)) && n < MAX_ENTRIES)
	{
		if (e->d_name[0] == '.')
			continue ;
		snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
		if (stat(path, &st) != 0)
			continue ;
		if (S_ISDIR(st.st_mode) == want_dirs)
			snprintf(names[n++], 256, "%s", e->d_name);
	}
	closedir(d);
	for (i = 0; i < n; i++)
		for (j = i + 1; j < n; j++)
			if (strcmp(names[i], names[j]) > 0)
			{
				strcpy(tmp, names[i]);
				strcpy(names[i], names[j]);
				strcpy(names[j], tmp);
			}
	return (n);
}

/* ---- level solver (BFS) ------------------------------------------------------
** Mirrors the real rules: walls block, and the exit is a wall until every
** collectible has been picked up. Produces a w/a/s/d key string. */

typedef struct { char *g; int w, h; } t_level;

static int	load_level(const char *path, t_level *L)
{
	char	*txt = read_file(path);
	char	*line, *save;
	int		row = 0;

	if (!txt)
		return (-1);
	L->w = 0;
	L->h = 0;
	for (line = txt; *line; )
	{
		char	*nl = strchr(line, '\n');
		int		len = nl ? (int)(nl - line) : (int)strlen(line);

		if (len == 0 && !nl)
			break ;
		if (L->w == 0)
			L->w = len;
		if (len != L->w)
			return (-1);
		L->h++;
		line += len + (nl ? 1 : 0);
	}
	L->g = malloc((size_t)(L->w * L->h) + 1);
	save = txt;
	for (row = 0; row < L->h; row++)
	{
		memcpy(L->g + row * L->w, save, (size_t)L->w);
		save += L->w + 1;
	}
	free(txt);
	return (0);
}

static int	walk_to_nearest(t_level *L, int *px, int *py, char target, char *keys, int *nk)
{
	int		n = L->w * L->h, head = 0, tail = 0, found = -1, i;
	int		*queue = malloc(sizeof(int) * (size_t)n);
	int		*prev = malloc(sizeof(int) * (size_t)n);
	char	*mv = malloc((size_t)n);
	int		dx[4] = {0, -1, 0, 1}, dy[4] = {-1, 0, 1, 0};
	char	dk[4] = {'w', 'a', 's', 'd'};
	int		start = *py * L->w + *px;

	for (i = 0; i < n; i++)
		prev[i] = -2;
	prev[start] = -1;
	queue[tail++] = start;
	while (head < tail && found < 0)
	{
		int	cur = queue[head++], cx = cur % L->w, cy = cur / L->w;

		for (i = 0; i < 4; i++)
		{
			int	nx = cx + dx[i], ny = cy + dy[i], c;

			if (nx < 0 || ny < 0 || nx >= L->w || ny >= L->h)
				continue ;
			c = ny * L->w + nx;
			if (prev[c] != -2 || L->g[c] == '1')
				continue ;
			if (L->g[c] == 'E' && target != 'E')
				continue ;
			prev[c] = cur;
			mv[c] = dk[i];
			if (L->g[c] == target)
			{
				found = c;
				break ;
			}
			queue[tail++] = c;
		}
	}
	if (found >= 0)
	{
		int		len = 0, c;
		char	*rev = malloc((size_t)n);

		for (c = found; prev[c] != -1; c = prev[c])
			rev[len++] = mv[c];
		if (*nk + len >= KEYS_CAP - 1)
			len = 0; /* absurdly long solution: give up on this level */
		while (len > 0)
			keys[(*nk)++] = rev[--len];
		free(rev);
		*px = found % L->w;
		*py = found / L->w;
		if (target == 'C')
			L->g[found] = '0';
	}
	free(queue);
	free(prev);
	free(mv);
	return (found >= 0 ? 0 : -1);
}

/* Returns 0 and fills keys when the level can be completed. */
static int	solve_level(const char *path, char *keys, size_t cap)
{
	t_level	L;
	int		px = -1, py = -1, i, nk = 0;

	if (load_level(path, &L) != 0)
		return (-1);
	for (i = 0; i < L.w * L.h; i++)
		if (L.g[i] == 'P')
		{
			px = i % L.w;
			py = i / L.w;
		}
	if (px < 0 || cap < KEYS_CAP)
		return (-1);
	while (memchr(L.g, 'C', (size_t)(L.w * L.h)))
		if (walk_to_nearest(&L, &px, &py, 'C', keys, &nk) != 0)
			return (-2);
	if (walk_to_nearest(&L, &px, &py, 'E', keys, &nk) != 0)
		return (-3);
	keys[nk] = '\0';
	free(L.g);
	return (0);
}

/* ---- level tests -------------------------------------------------------------- */

static int	level_paths(char paths[][1024], char names[][256])
{
	char	dir[512];
	int		n, i;

	snprintf(dir, sizeof(dir), "%s/levels", repo_root());
	n = list_dir(dir, names, 0);
	for (i = 0; i < n; i++)
		snprintf(paths[i], 1024, "%s/%s", dir, names[i]);
	return (n);
}

TEST(assets, at_least_one_level_exists)
{
	static char	paths[MAX_ENTRIES][1024], names[MAX_ENTRIES][256];

	ASSERT_TRUE(level_paths(paths, names) >= 1);
}

TEST(assets, every_level_passes_map_validation)
{
	static char	paths[MAX_ENTRIES][1024], names[MAX_ENTRIES][256];
	int			n = level_paths(paths, names), i;

	for (i = 0; i < n; i++)
	{
		char	out[OUTSZ];
		int		result, code;
		char	*content = read_file(paths[i]);

		ASSERT_TRUE(content != NULL);
		code = check_map(content, &result, out, sizeof(out));
		if (code != RET_NORMAL || result != 0)
			FAIL("level '%s' rejected by map_check (exit=%d, result=%d):\n%s",
				names[i], code, result, out);
	}
}

TEST(assets, every_level_is_solvable)
{
	static char	paths[MAX_ENTRIES][1024], names[MAX_ENTRIES][256];
	int			n = level_paths(paths, names), i;

	for (i = 0; i < n; i++)
	{
		static char	keys[KEYS_CAP];
		int		r = solve_level(paths[i], keys, sizeof(keys));

		if (r != 0)
			FAIL("level '%s' is not solvable (code %d: -2 = a collectible is "
				"unreachable, -3 = exit unreachable)", names[i], r);
	}
}

TEST(assets, every_level_can_be_played_to_the_end)
{
	static char	paths[MAX_ENTRIES][1024], names[MAX_ENTRIES][256];
	int			n = level_paths(paths, names), i;

	for (i = 0; i < n; i++)
	{
		static char	keys[KEYS_CAP];
		char	out[OUTSZ];
		int		code;

		if (solve_level(paths[i], keys, sizeof(keys)) != 0)
			FAIL("level '%s' is not solvable", names[i]);
		code = play_game_file(paths[i], keys, out, sizeof(out));
		if (code != 0)
			FAIL("level '%s': game did not finish after %zu moves (code %d)\n%s",
				names[i], strlen(keys), code, out);
		if (!strstr(out, "Move number:"))
			FAIL("level '%s': no win message printed\n%s", names[i], out);
	}
}

TEST(assets, every_level_fits_a_normal_screen)
{
	static char	paths[MAX_ENTRIES][1024], names[MAX_ENTRIES][256];
	int			n = level_paths(paths, names), i;

	for (i = 0; i < n; i++)
	{
		t_vars	v;

		init_vars(&v, paths[i]);
		get_map_info(&v);
		if (v.map_info.width * TILE > 1920 || v.map_info.height * TILE > 1080)
			FAIL("level '%s' needs a %dx%d window, larger than 1920x1080",
				names[i], v.map_info.width * TILE, v.map_info.height * TILE);
		ASSERT_TRUE(v.map_info.all_collectible < MAX_COLLECTIBLES);
	}
}

/* ---- texture tests --------------------------------------------------------------- */

static const char	*g_required[5] = {"bg.xpm", "exit.xpm", "item.xpm", "player.xpm", "wall.xpm"};

static int	pack_dirs(char dirs[][1024], char names[][256])
{
	char	base[512];
	int		n, i;

	snprintf(base, sizeof(base), "%s/textures", repo_root());
	n = list_dir(base, names, 1);
	for (i = 0; i < n; i++)
		snprintf(dirs[i], 1024, "%s/%s", base, names[i]);
	return (n);
}

TEST(assets, at_least_one_texture_pack_exists)
{
	static char	dirs[MAX_ENTRIES][1024], names[MAX_ENTRIES][256];

	ASSERT_TRUE(pack_dirs(dirs, names) >= 1);
}

TEST(assets, every_texture_pack_has_exactly_the_five_required_files)
{
	static char	dirs[MAX_ENTRIES][1024], names[MAX_ENTRIES][256];
	int			n = pack_dirs(dirs, names), i, j;

	for (i = 0; i < n; i++)
	{
		static char	files[MAX_ENTRIES][256];
		int			count = list_dir(dirs[i], files, 0);

		if (count != 5)
			FAIL("texture pack '%s' has %d files, expected exactly 5", names[i], count);
		for (j = 0; j < 5; j++)
		{
			char	p[1100];

			snprintf(p, sizeof(p), "%s/%s", dirs[i], g_required[j]);
			if (access(p, R_OK) != 0)
				FAIL("texture pack '%s' is missing %s", names[i], g_required[j]);
		}
	}
}

/* Parses an XPM3 file; returns 0 if well-formed. Reports width/height. */
static int	check_xpm(const char *path, int *w, int *h, char *why, size_t whysz)
{
	char	*txt = read_file(path), *line, *next;
	int		in_array = 0, nstr = 0, nc = 0, cpp = 0, ok = 0;

	if (!txt)
		return (snprintf(why, whysz, "cannot read file"), -1);
	if (strncmp(txt, "/* XPM */", 9) != 0)
		return (snprintf(why, whysz, "missing '/* XPM */' signature"), -1);
	for (line = txt; line && *line; line = next)
	{
		char	*q1, *q2;
		int		len;

		next = strchr(line, '\n');
		if (next)
			*next++ = '\0';
		if (!in_array)
		{
			if (strstr(line, "static char") || strstr(line, "static const char"))
				in_array = 1;
			continue ;
		}
		while (*line == ' ' || *line == '\t')
			line++;
		if (*line != '"')
			continue ;
		q1 = line;
		q2 = strrchr(line, '"');
		if (q2 == q1)
			return (snprintf(why, whysz, "unterminated string literal"), -1);
		len = (int)(q2 - q1 - 1);
		if (nstr == 0)
		{
			if (sscanf(q1 + 1, "%d %d %d %d", w, h, &nc, &cpp) != 4
				|| *w <= 0 || *h <= 0 || nc <= 0 || cpp <= 0)
				return (snprintf(why, whysz, "bad XPM header"), -1);
		}
		else if (nstr > nc && nstr <= nc + *h && len != *w * cpp)
			return (snprintf(why, whysz, "pixel row %d has length %d, expected %d",
				nstr - nc, len, *w * cpp), -1);
		nstr++;
	}
	ok = (nstr == 1 + nc + *h);
	if (!ok)
		return (snprintf(why, whysz, "expected %d strings (1 header + %d colors + %d rows), found %d",
			1 + nc + *h, nc, *h, nstr), -1);
	return (0);
}

TEST(assets, every_xpm_file_is_well_formed)
{
	static char	dirs[MAX_ENTRIES][1024], names[MAX_ENTRIES][256];
	int			n = pack_dirs(dirs, names), i, j;

	for (i = 0; i < n; i++)
		for (j = 0; j < 5; j++)
		{
			char	p[1100], why[256];
			int		w, h;

			snprintf(p, sizeof(p), "%s/%s", dirs[i], g_required[j]);
			if (check_xpm(p, &w, &h, why, sizeof(why)) != 0)
				FAIL("%s/%s: %s", names[i], g_required[j], why);
		}
}

TEST(assets, all_images_in_a_pack_have_the_same_size)
{
	static char	dirs[MAX_ENTRIES][1024], names[MAX_ENTRIES][256];
	int			n = pack_dirs(dirs, names), i, j;

	for (i = 0; i < n; i++)
	{
		int	w0 = 0, h0 = 0;

		for (j = 0; j < 5; j++)
		{
			char	p[1100], why[256];
			int		w, h;

			snprintf(p, sizeof(p), "%s/%s", dirs[i], g_required[j]);
			if (check_xpm(p, &w, &h, why, sizeof(why)) != 0)
				FAIL("%s/%s: %s", names[i], g_required[j], why);
			if (j == 0)
			{
				w0 = w;
				h0 = h;
			}
			else if (w != w0 || h != h0)
				FAIL("%s/%s is %dx%d but bg.xpm is %dx%d", names[i],
					g_required[j], w, h, w0, h0);
		}
	}
}

TEST(assets, texture_packs_match_the_50px_tile_size)
{
	/* The game draws on a fixed 50px grid. These two shipped packs are
	** smaller (they render with gaps) and are allow-listed; any NEW pack
	** must be 50x50. */
	static const char	*allowed_smaller[] = {"pacman", "space-ship"};
	static char			dirs[MAX_ENTRIES][1024], names[MAX_ENTRIES][256];
	int					n = pack_dirs(dirs, names), i, k;

	for (i = 0; i < n; i++)
	{
		char	p[1100], why[256];
		int		w, h, skip = 0;

		for (k = 0; k < 2; k++)
			if (strcmp(names[i], allowed_smaller[k]) == 0)
				skip = 1;
		if (skip)
			continue ;
		snprintf(p, sizeof(p), "%s/bg.xpm", dirs[i]);
		if (check_xpm(p, &w, &h, why, sizeof(why)) != 0)
			FAIL("%s/bg.xpm: %s", names[i], why);
		if (w != TILE || h != TILE)
			FAIL("texture pack '%s' is %dx%d, the game's tile size is %dx%d",
				names[i], w, h, TILE, TILE);
	}
}

const t_test	asset_tests[] = {
	T(assets, at_least_one_level_exists),
	T(assets, every_level_passes_map_validation),
	T(assets, every_level_is_solvable),
	T(assets, every_level_can_be_played_to_the_end),
	T(assets, every_level_fits_a_normal_screen),
	T(assets, at_least_one_texture_pack_exists),
	T(assets, every_texture_pack_has_exactly_the_five_required_files),
	T(assets, every_xpm_file_is_well_formed),
	T(assets, all_images_in_a_pack_have_the_same_size),
	T(assets, texture_packs_match_the_50px_tile_size),
};
const int	asset_count = sizeof(asset_tests) / sizeof(asset_tests[0]);
