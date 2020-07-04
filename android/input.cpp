#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
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

#define HAS_KEY(v) (key_bits[v / 8] & (1 << (v % 8)))
#define HAS_ABS(v) (abs_bits[v / 8] & (1 << (v % 8)))

static int input_kbd = 0, input_tpd = 0, udp_input = 0;
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
        received = recv(net_fd, &buf, INPUT_BINARY_SIZE, 0);
        if (received <= 0)
            continue;
        //  we only use EV_KEY and EV_ABS this value never larger than 1 byte. Use it as flag
        // clear the flag on type
        e.type = INPUT_BINARY_NO_MASK(buf[0]) << 8;
        e.type |= buf[1];

        e.code = buf[2] << 8;
        e.code |= buf[3];

        e.value = buf[4] << 8;
        e.value |= buf[5];
        // is kbd or touch? check flag
        if (INPUT_BINARY_CHECK_KEYBOARD(buf[0]))
        {
            write(input_kbd, &e, event_size);
        }
        else
        {
            write(input_tpd, &e, event_size);
        }

#ifdef INPUT_DEBUG
        print_event(e.type, e.code, e.value);
#endif
    }
}

static int input_setup_virtual_keyboard()
{
    int i, fd;
    uinput_user_dev uidev;

    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0)
    {
        printf("can't open /dev/uinput");
        return 0;
    }

    // Tell bout this device support for keys
    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
    {
        printf("Fail ioctl /dev/uinput");
        return 0;
    }

    // Keyboard keys
    for (i = KEY_ESC; i <= KEY_F17; i++)
    {
        ioctl(fd, UI_SET_KEYBIT, i);
    }

    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "kbd-screenstream");
    uidev.id.bustype = BUS_VIRTUAL;
    uidev.id.vendor = 0;
    uidev.id.product = 0;
    uidev.id.version = 0;

    if (write(fd, &uidev, sizeof(uidev)) < 0)
    {
        printf("error: write");
        return 0;
    }

    if (ioctl(fd, UI_DEV_CREATE) < 0)
    {
        printf("error: UI_DEV_CREATE");
        return 0;
    }
    return fd;
}

static int input_setup_scan()
{
    char dev[PATH_MAX], *name, dirname[] = "/dev/input/";
    DIR *dir;
    struct dirent *de;
    struct input_id id;
    int version, ret, fd = -1;
    //uint8_t ev_bits[EV_MAX / 8 + 1];
    uint8_t abs_bits[ABS_MAX / 8 + 1];
    uint8_t key_bits[KEY_MAX / 8 + 1];

    if ((dir = opendir(dirname)) == NULL)
    {
        printf("Fail opendir %s\n", dirname);
        return 0;
    }

    strcpy(dev, dirname);
    name = dev + strlen(dev);

    while ((de = readdir(dir)))
    {
        if (de->d_name[0] == '.')
            continue;

        // we just find all
        if (input_tpd && input_kbd)
            break;

        strcpy(name, de->d_name);

        //memset(ev_bits, 0, sizeof(ev_bits));
        memset(key_bits, 0, sizeof(key_bits));
        memset(abs_bits, 0, sizeof(abs_bits));

        if (fd >= 0)
            close(fd);

        fd = open(dev, O_RDWR);

        if (fd < 0)
            continue;

        if (ioctl(fd, EVIOCGVERSION, &version))
            continue;

        // ret = ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits);

        // if (ret != sizeof(ev_bits))
        // {
        //     printf("\tCan't get ev bits %d\n", ret);
        //     continue;
        // }

        // if ((ev_bits[EV_KEY / 8] & (1 << (EV_KEY % 8))))
        // {
        //     printf("\t-> key device\n");
        //     //continue;
        // }

        // if ((ev_bits[EV_ABS / 8] & (1 << (EV_ABS % 8))))
        // {
        //     input_tpd = fd;
        //     fd = 0;
        //     printf("\t-> touch device\n");
        //     continue;
        // }

        ret = ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits);
        if (ret != sizeof(key_bits))
        {

#ifdef INPUT_DEBUG
            printf("%s: Can't get key\n", dev);
#endif
            continue;
        }
        ret = ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits);
        if (ret != sizeof(abs_bits))
        {

#ifdef INPUT_DEBUG
            printf("%s: Can't get key\n", dev);
#endif
            continue;
        }

        if (!input_tpd && HAS_KEY(BTN_TOUCH) && (HAS_ABS(ABS_X) || HAS_ABS(ABS_MT_POSITION_X)))
        {

#ifdef INPUT_DEBUG
            printf("%s: Found Touch\n", dev);
#endif
            input_tpd = fd;
            fd = 0;
            continue;
        }

        // Check for keyboard
        if (!input_kbd &&
            HAS_KEY(KEY_A) &&
            HAS_KEY(KEY_LEFTCTRL) &&
            HAS_KEY(KEY_ENTER) &&
            HAS_KEY(KEY_ESC))
        {

#ifdef INPUT_DEBUG
            printf("%s: Found Keyboard\n", dev);
#endif
            input_kbd = fd;
            fd = 0;
            continue;
        }
    }

    if (fd)
        close(fd);

    closedir(dir);
    return 1;
}

void input_setup()
{

    event_size = sizeof(input_event);
    event_key = (input_event *)calloc(event_size, 1);
    event_sync = (input_event *)calloc(event_size, 1);
    event_key->type = EV_KEY;

    input_setup_scan();

    // No keyboard? setup our virtual keyboard
    if (!input_kbd)
    {
        input_kbd = input_setup_virtual_keyboard();
        if (!input_kbd)
        {
            printf("\x1b[32m**Setup virtual keyboard fail**\x1b[0m\n");
            input_kbd = input_tpd;
        }
    }

    if (!input_tpd)
    {
        printf("\x1b[32m**Input touch fail, not touchsreen device?**\x1b[0m\n");
    }

    if (pthread_create(&input_thread_id, NULL, &input_thread, NULL) != 0)
        DIE("Fail starting input thread");
}

/* Simulating keypress */
int input_press(int key)
{
    // UP
    event_key->code = key;
    event_key->value = 1;

    if (write(input_kbd, event_key, event_size) != event_size)
        return 0;

    // Sync
    if (write(input_kbd, event_sync, event_size) != event_size)
        return 0;

    // Down
    event_key->value = 0;
    if (write(input_kbd, event_key, event_size) != event_size)
        return 0;

    // Sync
    if (write(input_kbd, event_sync, event_size) != event_size)
        return 0;

    return 1;
}
