#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include "decode.h"
#define BUFF_SIZE 102400
// sizeof(struct sockaddr_in)
#define SOCK_SIZE 16U

size_t frame_received = 0UL;
int fd;
uint32_t nip, ip;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

static int setup_connection(uint16_t *port)
{
    struct sockaddr_in addr;
    fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0)
    {
        printf("Fail listen socket");
        return 0;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(*port);
    printf("Listening on %d ", *port);
    if (bind(fd, (struct sockaddr *)&addr, SOCK_SIZE) != 0)
    {
        printf("Fail\n");
        close(fd);
        return 0;
    }
    printf("OK\n");
    return 1;
}

static void receiver_thread(Window *window)
{
    socklen_t sender_size;
    ssize_t received;
    Display *display = XOpenDisplay(NULL);
    unsigned char *buff = malloc(BUFF_SIZE);
    struct sockaddr_in *sender = calloc(SOCK_SIZE, 1);

    if (display == NULL)
    {
        fprintf(stderr, "Cannot open display in thread\n");
        exit(1);
    }
    // XEvent exppp;
    // memset(&exppp, 0, sizeof(exppp));
    // exppp.type = Expose;
    // exppp.xexpose.window = window;
    while (1)
    {
        sender_size = SOCK_SIZE;
        received = recvfrom(fd, buff, BUFF_SIZE, 0, (struct sockaddr *)sender, &sender_size);
        if (received > 0)
        {
            if (nip != sender->sin_addr.s_addr)
            {
                printf("Ignoring packet from different initial IP!.");
                continue;
                // nip = sender->sin_addr.s_addr;
                // frame_received = 0;
                // XSendEvent(display, window, False, ExposureMask, &exppp);
                //XFlush(display);
            }
            frame_received++;
            if (!decode_jpeg_run(buff, received))
            {
                fprintf(stderr, "Fail decode jpeg\n");
            }
            printf("%lu: %lu\n", frame_received, received);
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
    unsigned char *buff = malloc(BUFF_SIZE);

    while (1)
    {
        sender_size = SOCK_SIZE;
        printf("Waiting first frame.. ");
        received = recvfrom(fd, buff, BUFF_SIZE, 0, (struct sockaddr *)&sender, &sender_size);
        printf("%lu\n", received);
        if (received > 0)
        {
            nip = sender.sin_addr.s_addr;
            ip = ntohl(nip);
            decode_jpeg_init(buff, received);
            printf("Initial size %dX%d\n", cinfo.image_width, cinfo.image_height);
            break;
        }
    }
    free(buff);
    return 1;
}

int main(void)
{
    char str[100];
    XEvent e;
    pthread_t thread_id;
    uint16_t port = 1234;
    XImage *ximage;
    Display *display;
    Screen *screen;
    Window window;
    int screen_num;

    display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    if (!setup_connection(&port))
        return 1;
    initial_size();

    screen_num = DefaultScreen(display);
    screen = ScreenOfDisplay(display, screen_num);
    window = XCreateSimpleWindow(display, screen->root,
                                 100,
                                 100,
                                 cinfo.image_width,
                                 cinfo.image_height,
                                 0,
                                 screen->white_pixel,
                                 screen->white_pixel);
    XSelectInput(display, window, ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask);
    XMapWindow(display, window);

    ximage = XCreateImage(screen->display, screen->root_visual, screen->root_depth, ZPixmap, 0,
                          xdata, cinfo.image_width, cinfo.image_height, 32, 0);

    pthread_create(&thread_id, NULL, (void *)&receiver_thread, &window);

    while (1)
    {
        XNextEvent(display, &e);
        switch (e.type)
        {
        // Resize
        case Expose:
            pthread_mutex_lock(&mutex1);
            sprintf(str, "Connected: %d.%d.%d.%d", ip >> 24, ip >> 16, ip >> 8, ip);
            XStoreName(display, window, str);
            XPutImage(display, window, DefaultGC(display, 0), ximage, 0, 0, 0, 0, cinfo.output_width, cinfo.output_height);
            pthread_mutex_unlock(&mutex1);
            break;
        // Keyboard key
        case KeyPress:
            printf("KEY: %d\n", e.xkey.keycode);
            break;
        case ButtonPress:
            printf("BTN DOWN: %d\n", e.xbutton.button);
            break;
        case ButtonRelease:
            printf("BTN UP: %d\n", e.xbutton.button);
            break;
        case MotionNotify:
            printf("MV: %03d %03d\n", e.xbutton.x, e.xbutton.y);
            break;
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