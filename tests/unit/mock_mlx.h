#ifndef MOCK_MLX_H
# define MOCK_MLX_H

/* Records what the game asks MiniLibX to draw, so tests can inspect it. */

# define MAX_PUTS 8192

typedef struct { void *img; int x; int y; } t_put_call;

extern t_put_call	g_puts[MAX_PUTS];
extern int			g_put_count;
extern int			g_clear_count;
extern int			g_destroy_count;

void	mock_mlx_reset(void);

#endif
