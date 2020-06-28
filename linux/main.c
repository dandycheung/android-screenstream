#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define BUFF_SIZE 256

size_t frame_received = 0UL;
static struct sockaddr_in *sender;
Display *display;
Window window;
int screen;
char str[100];
int len;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

int printaddr(char *tmp, struct sockaddr_in *addr)
{
    uint32_t ip = ntohl(addr->sin_addr.s_addr);
    uint16_t port = ntohs(addr->sin_port);
    return sprintf(tmp, "%d.%d.%d.%d:%d",
                   ip >> 24 & 0xff,
                   ip >> 16 & 0xff,
                   ip >> 8 & 0xff,
                   ip >> 0 & 0xff,
                   port);
}

static void draw_state()
{
    len = sprintf(str, "%d ", frame_received);
    printaddr(str + len, sender);
    XStoreName(display, window, str);
}

void receiver_thread(uint16_t *port)
{
    char buff[BUFF_SIZE];
    struct sockaddr_in addr;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    socklen_t received, sender_size, addr_size = sizeof(struct sockaddr_in);

    if (fd < 0)
    {
        printf("Fail listen socket");
        return;
    }

    memset(&addr, 0, addr_size);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(*port);
    printf("Listening on %d ", *port);
    if (bind(fd, (struct sockaddr *)&addr, addr_size) != 0)
    {
        printf("Fail\n");
        close(fd);
        return;
    }
    printf("OK\n");
    XEvent exppp;
    memset(&exppp, 0, sizeof(exppp));
    exppp.type = Expose;
    exppp.xexpose.window = window;
    while (1)
    {
        sender_size = addr_size;
        received = recvfrom(fd, buff, BUFF_SIZE, 0, (struct sockaddr *)sender, &sender_size);
        if (received > 0)
        {
            frame_received++;
            buff[received] = 0;
            printf("%d: %s\n", frame_received, buff);
            //draw_state();
            XSendEvent(display, window, False, ExposureMask, &exppp);
            XFlush(display);
        }
        else
        {
            printf("Fail, %d bytes\n", received);
        }
    }
}

int main(void)
{

    XEvent e;
    pthread_t thread_id;
    uint16_t port = 1234;
    sender = malloc(sizeof(struct sockaddr));

    display = XOpenDisplay(NULL);

    if (display == NULL)
    {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }
    screen = DefaultScreen(display);
    window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, 360, 640, 0,
                                 WhitePixel(display, screen), WhitePixel(display, screen));
    XSelectInput(display, window, ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask);
    XMapWindow(display, window);

    pthread_create(&thread_id, NULL, (void *)&receiver_thread, &port);

    while (1)
    {
        XNextEvent(display, &e);
        switch (e.type)
        {
        // Resize
        case Expose:
            pthread_mutex_lock(&mutex1);
            draw_state();
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