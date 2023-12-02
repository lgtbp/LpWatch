#include "lvgl_inc.h"

extern int app_main(void);
extern int sdl_main_task(void *);

void lvgl_driver_init(void)
{
}
int main(int argc, char *argv[])
{
	app_main();
	sdl_main_task((void *)0);
	return 0;
}
