/* Headless stand-in for MiniLibX: only the calls the game logic needs. */
#include "mock_mlx.h"
#include "mlx.h"
#include <string.h>
#include <unistd.h>

t_put_call	g_puts[MAX_PUTS];
int			g_put_count;
int			g_clear_count;
int			g_destroy_count;

void	mock_mlx_reset(void)
{
	g_put_count = 0;
	g_clear_count = 0;
	g_destroy_count = 0;
}

int	mlx_put_image_to_window(void *mlx, void *win, void *img, int x, int y)
{
	(void)mlx;
	(void)win;
	if (g_put_count < MAX_PUTS)
	{
		g_puts[g_put_count].img = img;
		g_puts[g_put_count].x = x;
		g_puts[g_put_count].y = y;
		g_put_count++;
	}
	return (0);
}

int	mlx_clear_window(void *mlx, void *win)
{
	(void)mlx;
	(void)win;
	g_clear_count++;
	return (0);
}

/* Written with write(2) so tests that run the game in a child process can
** see it in the captured output. */
int	mlx_destroy_window(void *mlx, void *win)
{
	static const char	msg[] = "[mock-mlx] destroy_window\n";

	(void)mlx;
	(void)win;
	g_destroy_count++;
	if (write(1, msg, sizeof(msg) - 1) < 0)
		return (-1);
	return (0);
}
