// Damiano Stoffie 166312, Anatoliy Babushka 165958, gioco multiplayer, 2015

#include "game.h"

void safe_write(int fd, void * buffer, int size)
{
    if (write(fd, buffer, size) == -1) {
        printf("An error occurred while writing file escriptor %i\n", fd);
        abort();
    }
}

void safe_read(int fd, void * buffer, int size)
{
    if (read(fd, buffer, size) == -1) {
        printf("An error occurred while reading file escriptor %i\n", fd);
        abort();
    }
}
