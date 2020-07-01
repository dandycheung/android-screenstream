#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include "main.h"
#include "input.h"

static int fd_input;
static struct input_event *event_key, *event_sync;
static uint8_t event_size;

void input_setup()
{
    int version;
    event_size = sizeof(struct input_event);
    event_key = (input_event *)calloc(event_size, 1);
    event_sync = (input_event *)calloc(event_size, 1);
    event_key->type = EV_KEY;

    fd_input = open("/dev/input/event4", O_RDWR);
    if (ioctl(fd_input, EVIOCGVERSION, &version))
        DIE("error: open input");
}

int input_press(int key)
{
    // UP
    event_key->code = key;
    event_key->value = 1;
    if (write(fd_input, event_key, event_size) != event_size)
    {
        return 0;
    }
    // Sync
    if (write(fd_input, event_sync, event_size) != event_size)
    {
        return 0;
    }
    // Down
    event_key->value = 0;
    if (write(fd_input, event_key, event_size) != event_size)
    {
        return 0;
    }
    // Sync
    if (write(fd_input, event_sync, event_size) != event_size)
    {
        return 0;
    }
    return 1;
}
