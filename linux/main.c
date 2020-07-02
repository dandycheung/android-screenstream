#define XK_XKB_KEYS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <X11/Xlib.h>
#include <X11/keysymdef.h>
#include <X11/extensions/shape.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <shared-config.h>
#ifdef INPUT_DEBUG
#include <input-debug.h>
#endif

#include "decode.h"

#define DIE(str, args...)             \
    do                                \
    {                                 \
        fprintf(stderr, "%s\n", str); \
        exit(1);                      \
    } while (0)

#define BTN_STATE_UP 0
#define BTN_STATE_DOWN 1
#define BTN_STATE_REPEAT 2
#define BUFF_SIZE 102400
// sizeof(struct sockaddr_in)
#define __SOCK_SIZE__ 16U
#define TOUCH_STACK_SIZE 100U

struct window_size
{
    uint16_t w, h;
};

struct touch_point
{
    // convert to percentage
    uint16_t x, y;
};

static struct window_size wSize;
static int fd;
static pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
struct input_event *event_key, *event_sync, *event_abs;
uint8_t event_size;
struct sockaddr_in listen_addr, device_addr;
Window window;

static void update_device_info(const struct sockaddr_in *const dev)
{
    memset(&device_addr, 0, __SOCK_SIZE__);
    memcpy(&device_addr, dev, __SOCK_SIZE__);
    printf("Device Connected: %s:%d\n", inet_ntoa(device_addr.sin_addr), ntohs(device_addr.sin_port));
}

static void setup_connection(uint16_t port)
{
    fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0)
        DIE("Fail listen socket");

    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&listen_addr, __SOCK_SIZE__) != 0)
        DIE("Fail bind");

    printf("Listening on %d\n", port);
}

static void receiver_thread(Window *window)
{
    socklen_t sender_size;
    ssize_t received;
    size_t frame_received = 0UL;
    Display *display = XOpenDisplay(NULL);
    unsigned char buff[BUFF_SIZE];
    struct sockaddr_in sender;

    if (display == NULL)
        DIE("Cannot open display");

    XEvent exppp;
    memset(&exppp, 0, sizeof(exppp));
    exppp.type = Expose;
    exppp.xexpose.window = *window;

    while (1)
    {
        sender_size = __SOCK_SIZE__;
        received = recvfrom(fd, &buff, BUFF_SIZE, 0, (struct sockaddr *)&sender, &sender_size);
        if (received > 0)
        {
            if (device_addr.sin_addr.s_addr != sender.sin_addr.s_addr ||
                device_addr.sin_port != sender.sin_port)
            {
                // printf("Ignoring packet from different initial IP: %s.\n", inet_ntoa(sender->sin_addr));
                // continue;
                pthread_mutex_lock(&mutex1);
                update_device_info(&sender);
                pthread_mutex_unlock(&mutex1);
            }
            frame_received++;
            if (!decode_jpeg_run(&buff, received))
            {
                fprintf(stderr, "Fail decode jpeg\n");
            }
            XSendEvent(display, *window, False, ExposureMask, &exppp);
            XFlush(display);
            //printf("%lu: %lu\n", frame_received, received);
        }
        else
        {
            printf("ignore, %lu bytes\n", received);
        }
    }
}

int initial_size()
{
    struct sockaddr_in sender;
    socklen_t sender_size;
    ssize_t received;
    unsigned char buff[BUFF_SIZE];

    while (1)
    {
        sender_size = __SOCK_SIZE__;
        printf("Waiting first frame.. ");
        received = recvfrom(fd, buff, BUFF_SIZE, 0, (struct sockaddr *)&sender, &sender_size);
        printf("%lu\n", received);

        if (received > 0)
        {
            update_device_info(&sender);
            decode_jpeg_init(buff, received);
            wSize.w = cinfo.image_width;
            wSize.h = cinfo.image_height;
            printf("Initial size %ux%u %d bytes\n", wSize.w, wSize.h, received);
            break;
        }
    }

    return 1;
}

void setup_input()
{
    event_size = sizeof(struct input_event);
    event_key = (struct input_event *)calloc(event_size, 1);
    event_sync = (struct input_event *)calloc(event_size, 1);
    event_abs = (struct input_event *)calloc(event_size, 1);
    event_key->type = EV_KEY;
    event_abs->type = EV_ABS;
    event_sync->type = EV_SYN;
}

static void send_input(struct input_event *e)
{
    uint8_t buf[INPUT_BINARY_SIZE];

    buf[0] = e->type >> 8;
    buf[1] = e->type;

    buf[2] = e->code >> 8;
    buf[3] = e->code;

    // 2 byte is enough
    buf[4] = e->value >> 8;
    buf[5] = e->value;
#ifdef INPUT_DEBUG
    print_event(e->type, e->code, e->value);
#endif
    sendto(fd, buf, INPUT_BINARY_SIZE, 0, (struct sockaddr *)&device_addr, __SOCK_SIZE__);
}
static int map_mouse_button(int btn)
{
    switch (btn)
    {
    case 1: // left
        return BTN_TOUCH;
    case 2: // middle
        return KEY_F17;
    case 3: // right
        return KEY_BACK;
    case 4: // scrl UP
        return KEY_SCROLLUP;
    case 5: // scrl DOWN
        return KEY_SCROLLDOWN;
    default:
        return -1;
    }
}

void send_touch_position(int32_t x, int32_t y)
{
    event_abs->code = ABS_MT_TOUCH_MAJOR;
    event_abs->value = 0x2a;
    send_input(event_abs);
    // event_abs->code = ABS_MT_TRACKING_ID;
    // event_abs->value = 0;
    // send_input(event_abs);
    event_abs->code = ABS_MT_POSITION_X;
    event_abs->value = x * 2;
    send_input(event_abs);
    event_abs->code = ABS_MT_POSITION_Y;
    event_abs->value = y * 2;
    send_input(event_abs);
    event_sync->code = SYN_MT_REPORT;
    send_input(event_sync);
    event_sync->code = SYN_REPORT;
    send_input(event_sync);
}
void send_keypress(uint16_t key)
{

    event_key->code = key;
    event_key->value = 1;
    send_input(event_key);

    event_sync->code = SYN_REPORT;
    send_input(event_sync);

    event_key->code = key;
    event_key->value = 0;
    send_input(event_key);

    event_sync->code = SYN_REPORT;
    send_input(event_sync);
}

void setup_window(Display **display, XImage **ximage)
{
    char str[100];
    int screen_num;
    Screen *screen;
    XSizeHints w_hints;

    *display = XOpenDisplay(NULL);

    if (*display == NULL)
        DIE("Cannot open *display");

    screen_num = DefaultScreen(*display);
    screen = ScreenOfDisplay(*display, screen_num);
    w_hints.flags = PPosition;
    w_hints.x = 1200 - wSize.w;
    w_hints.y = 50;

    printf("window screen: %d %s %d\n", screen->width, DisplayString(*display), ScreenCount(*display));

    window = XCreateSimpleWindow(*display, screen->root,
                                 w_hints.x,
                                 w_hints.y,
                                 wSize.w,
                                 wSize.h,
                                 0,
                                 screen->white_pixel,
                                 screen->white_pixel);

    XSetNormalHints(*display, window, &w_hints);
    XSelectInput(
        *display,
        window,
        ExposureMask |
            KeyPressMask |
            KeyReleaseMask |
            ButtonPressMask |
            ButtonReleaseMask |
            Button1MotionMask);

    sprintf(str, "Connected: %s:%d\n", inet_ntoa(device_addr.sin_addr), ntohs(device_addr.sin_port));
    XStoreName(*display, window, str);

    *ximage = XCreateImage(screen->display, screen->root_visual, screen->root_depth, ZPixmap, 0,
                           xdata, wSize.w, wSize.h, 32, 0);

    XMapWindow(*display, window);
}

int main(void)
{
    XEvent e;
    pthread_t thread_id;
    uint16_t port = 1234;
    XImage *ximage;
    Display *display;
    KeySym keysym;
    int btn;

    memset(&listen_addr, 0, __SOCK_SIZE__);
    memset(&device_addr, 0, __SOCK_SIZE__);

    setup_connection(port);

    initial_size();

    setup_input();

    setup_window(&display, &ximage);

    send_keypress(KEY_F17);

    pthread_create(&thread_id, NULL, (void *)&receiver_thread, &window);

    while (1)
    {

        XNextEvent(display, &e);

        switch (e.type)
        {

        // Resize
        case Expose:
            pthread_mutex_lock(&mutex1);
            XPutImage(display, window, DefaultGC(display, 0), ximage, 0, 0, 0, 0, cinfo.output_width, cinfo.output_height);
            pthread_mutex_unlock(&mutex1);
            break;

        // Keyboard key
        case KeyRelease:
        case KeyPress:
            keysym = XLookupKeysym((XKeyEvent *)&e, 0);
            printf("KEY: %d  %lu %s %s\n", e.xkey.keycode, keysym, XKeysymToString(keysym), e.type == KeyPress ? "DOWN" : "UP");
            event_key->type = EV_KEY | 0b1000000000000000;
            event_key->code = keysym;
            event_key->value = e.type == KeyPress ? BTN_STATE_DOWN : BTN_STATE_UP;
            send_input(event_key);
            event_sync->code = SYN_REPORT;
            send_input(event_sync);
            // restore
            event_key->type = EV_KEY;
            break;

        // Mouse Button
        case ButtonPress:
        case ButtonRelease:
            btn = map_mouse_button(e.xbutton.button);
            if (btn < 0)
            {
                printf("Unmapped button: %d\n", e.xbutton.button);
                continue;
            }
            printf("X-Press/Release: %s\n", e.type == ButtonPress ? "DOWN" : "UP");
            event_key->code = btn;
            event_key->value = e.type == ButtonPress ? BTN_STATE_DOWN : BTN_STATE_UP;
            send_input(event_key);
            if (btn == BTN_TOUCH)
            {
                // Send location too
                send_touch_position(e.xbutton.x, e.xbutton.y);
            }
            else
            {
                event_sync->code = SYN_REPORT;
                send_input(event_sync);
            }
            break;

        case MotionNotify:
            if (e.xbutton.x < 0 || e.xbutton.y < 0 || e.xbutton.x > wSize.w || e.xbutton.y > wSize.h)
            {
                printf("Overlap ignored\n");
                continue;
            }
            //printf("MV: %03d %03d\n", e.xbutton.x, e.xbutton.y);
            send_touch_position(e.xbutton.x, e.xbutton.y);
            break;
        default:
            printf("E.type: %d\n", e.type);
            break;
        }
    }

    pthread_join(thread_id, NULL);
    XCloseDisplay(display);
    return 0;
}