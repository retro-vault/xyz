#include <fcntl.h>
#include <gpx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <yos.h>

static volatile unsigned console_characters;

static void count_console_character(char character)
{
    (void)character;
    ++console_characters;
}

int main(void)
{
    static const char payload[] = "Hello from an XCC YOS process!\n";
    char *copy;
    yos_t *yos = yos_get_api();
    gpx_api_t *gpx;
    int fd;

    if (!yos || yos->version() < YOS_VERSION)
        return 1;

    /* Standard output is silent until a process installs its own sink. */
    yos_set_putchar_hook(count_console_character);
    puts("This is delivered to the hook, not directly to the display.");

    copy = (char *)malloc(sizeof payload);
    if (!copy)
        return 2;

    fd = open("HELLO.TXT", O_RDWR | O_CREAT | O_TRUNC);
    if (fd < 0) {
        free(copy);
        return 3;
    }
    if (write(fd, payload, sizeof payload) != sizeof payload
        || lseek(fd, 0L, SEEK_SET) < 0
        || read(fd, copy, sizeof copy) != sizeof copy
        || close(fd) < 0
        || memcmp(copy, payload, sizeof payload) != 0) {
        close(fd);
        free(copy);
        return 4;
    }
    free(copy);

    gpx = (gpx_api_t *)query_service(GPX_SERVICE_NAME);
    if (gpx) {
        gpx_t *screen = gpx->create(GPXM_DEFAULT);
        if (screen)
            gpx->draw_pixel(screen, 128, 96, CO_FORE, BM_CPY, NULL);
    }

    return console_characters ? 0 : 5;
}
