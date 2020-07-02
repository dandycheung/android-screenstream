#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <linux/input-event-codes.h>
#include <shared-config.h>
#ifdef INPUT_DEBUG
#include <input-debug.h>
#endif
#include "main.h"
#include "input.h"

static int fd_input, udp_input, fd_uinput;
static struct input_event *event_key, *event_sync;
static uint8_t event_size;
static pthread_t input_thread_id;

static void *input_thread(void *)
{
    uint8_t buf[INPUT_BINARY_SIZE];
    struct input_event e;
    ssize_t received;
    memset(&e, 0, event_size);
    while (1)
    {
        // TODO: Lock in target ip
        received = recv(fd, &buf, INPUT_BINARY_SIZE, 0);
        if (received <= 0)
            continue;

        e.type = buf[0] << 8;
        e.type |= buf[1];

        e.code = buf[2] << 8;
        e.code |= buf[3];

        e.value = buf[4] << 8;
        e.value |= buf[5];

        // check is our virtual?
        if (e.type & 0b1000000000000000)
        {
            // remove mask
            printf("GOT KBD\n");
            e.type ^= 0b1000000000000000;
            write(fd_uinput, &e, event_size);
        }
        else
        {
            write(fd_input, &e, event_size);
        }
#ifdef INPUT_DEBUG
        print_event(e.type, e.code, e.value);
#endif
        usleep(1000);
    }
}

static int input_setup_uinput()
{
    int i;
    uinput_user_dev uidev;

    fd_uinput = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd_uinput < 0)
    {
        printf("can't open /dev/uinput");
        return 0;
    }

    // Keyboard
    if (ioctl(fd_uinput, UI_SET_EVBIT, EV_KEY) < 0)
    {
        printf("Fail ioctl /dev/uinput");
        return 0;
    }

    for (i = 0; i < 256; i++)
    {
        if (ioctl(fd_uinput, UI_SET_KEYBIT, i) < 0)
        {
            printf("Fail ioctl key /dev/uinput");
            return 0;
        }
    }
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Input Injector");
    uidev.id.bustype = BUS_VIRTUAL;
    uidev.id.vendor = 0;
    uidev.id.product = 0;
    uidev.id.version = 0;

    if (write(fd_uinput, &uidev, sizeof(uidev)) < 0)
    {
        printf("error: write");
        return 0;
    }

    if (ioctl(fd_uinput, UI_DEV_CREATE) < 0)
    {
        printf("error: UI_DEV_CREATE");
        return 0;
    }
    return 1;
}

void input_setup(uint16_t port)
{
    int version;

    event_size = sizeof(input_event);
    event_key = (input_event *)calloc(event_size, 1);
    event_sync = (input_event *)calloc(event_size, 1);
    event_key->type = EV_KEY;

    fd_input = open("/dev/input/event4", O_RDWR);
    if (ioctl(fd_input, EVIOCGVERSION, &version))
        DIE("open input");

    if (pthread_create(&input_thread_id, NULL, &input_thread, NULL) != 0)
        DIE("Fail starting input thread");

    if (!input_setup_uinput())
        printf("\x1b[32m**/dev/uinput fail. kbd not work**\x1b[0m\n");

#ifdef INPUT_DEBUG
    printf("\x1b[33m**input debug enabled**\x1b[0m\n");
#endif
}

/* Simulating keypress */
int input_press(int key)
{
    // UP
    event_key->code = key;
    event_key->value = 1;

    if (write(fd_input, event_key, event_size) != event_size)
        return 0;

    // Sync
    if (write(fd_input, event_sync, event_size) != event_size)
        return 0;

    // Down
    event_key->value = 0;
    if (write(fd_input, event_key, event_size) != event_size)
        return 0;

    // Sync
    if (write(fd_input, event_sync, event_size) != event_size)
        return 0;

    return 1;
}
