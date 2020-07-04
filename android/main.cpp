#define HAVE_ENDIAN_H 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <binder/ProcessState.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>
#include <binder/IServiceManager.h>
#include <ui/PixelFormat.h>
#include <ui/DisplayInfo.h>
#include <SkImageEncoder.h>
#include <SkBitmap.h>
#include <SkData.h>
#include <SkStream.h>
#include <linux/input-event-codes.h>

#include <shared-config.h>

#include "main.h"
#include "input.h"

using namespace android;

static struct sigaction defaultSigHandlerINT;
//static struct sigaction defaultSigHandlerHUP;
static int device_state = 0;
static pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int net_fd;

static SkBitmap::Config flinger2skia(PixelFormat f)
{
    switch (f)
    {
    case PIXEL_FORMAT_RGB_565:
        return SkBitmap::kRGB_565_Config;
    default:
        return SkBitmap::kARGB_8888_Config;
    }
}

void *sleep_watcher(void *arg)
{
    int fds[] = {0, 0}, *cur_fd;
    size_t ret;
    char ch;
    fds[0] = open("/sys/power/wait_for_fb_sleep", O_RDONLY);
    fds[1] = open("/sys/power/wait_for_fb_wake", O_RDONLY);
    cur_fd = &fds[device_state];
    while (device_state != DEVICE_STATE_EXIT)
    {
        ret = read(*cur_fd, &ch, 0);
        pthread_mutex_lock(&mutex1);
        device_state = !device_state;
        cur_fd = &fds[device_state];
        pthread_mutex_unlock(&mutex1);
        printf("** Device is %s\n", device_state == DEVICE_STATE_WAKE ? "WAKED" : "SLEEPT");
    }
    close(fds[0]);
    close(fds[1]);
    printf("** Sleeping watcher thread exited\n");
    return NULL;
}
/*
 * Catch keyboard interrupt signals.  On receipt, the "stop requested"
 * flag is raised, and the original handler is restored (so that, if
 * we get stuck finishing, a second Ctrl-C will kill the process).
 */
static void signalCatcher(int signum)
{
    pthread_mutex_lock(&mutex1);
    device_state = DEVICE_STATE_EXIT;
    pthread_mutex_unlock(&mutex1);

    printf("Exiting..\n");

    switch (signum)
    {
    case SIGINT:
    case SIGHUP:
        sigaction(SIGINT, &defaultSigHandlerINT, NULL);
        //sigaction(SIGHUP, &defaultSigHandlerHUP, NULL);
        break;
    default:
        abort();
        break;
    }
}
/*
 * Configures signal handlers.  The previous handlers are saved.
 *
 * If the command is run from an interactive adb shell, we get SIGINT
 * when Ctrl-C is hit.  If we're run from the host, the local adb process
 * gets the signal, and we get a SIGHUP when the terminal disconnects.
 */
static void configureSignals()
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = signalCatcher;
    if (sigaction(SIGINT, &act, &defaultSigHandlerINT) != 0)
    {
        printf("Unable to configure SIGINT\n");
    }
    // if (sigaction(SIGHUP, &act, &defaultSigHandlerHUP) != 0)
    // {
    //     status_t err = -errno;
    //     fprintf(stderr, "Unable to configure SIGHUP handler: %s\n",
    //             strerror(errno));
    // }
}
int main(int argc, char const *argv[])
{
    uint32_t targetIP;
    uint16_t port = 1234U;
    uint8_t fps = 20;
    useconds_t delay;
    struct sockaddr_in target;
    ssize_t size, bpp;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    SkDynamicMemoryWStream stream;
    ScreenshotClient screenshot;
    SkBitmap bmp;
    SkData *streamData;
    SkBitmap::Config bmpConfig;
    DisplayInfo mainDpyInfo;
    sp<IBinder> display;
    uint32_t scalled_width, scalled_height, format, stride;
    pthread_t sleep_thread;

    if (argc == 1)
        DIE("no IP target");

    targetIP = inet_addr(argv[1]);

    delay = 1000000 / fps;

    configureSignals();
    if (pthread_create(&sleep_thread, NULL, sleep_watcher, NULL) != 0)
    {
        printf("Fail starting sleep watcher thread\n");
    }

    net_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (net_fd == -1)
        DIE("Fail creating socket");

    memset(&target, 0, addr_size);
    target.sin_family = AF_INET;
    target.sin_addr.s_addr = targetIP;
    target.sin_port = htons(port);

    ProcessState::self()->startThreadPool();
    display = SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain);

    if (SurfaceComposerClient::getDisplayInfo(display, &mainDpyInfo) != NO_ERROR)
        DIE("ERROR: unable to get display characteristics");

    input_setup();
    input_press(KEY_F17);

    scalled_width = mainDpyInfo.w / 2;
    scalled_height = mainDpyInfo.h / 2;

    if (screenshot.update(display, scalled_width, scalled_height) == NO_ERROR)
    {
        stride = screenshot.getStride();
        format = screenshot.getFormat();
        bpp = bytesPerPixel(format);
        bmpConfig = flinger2skia(format);
        bmp.setConfig(bmpConfig, scalled_width, scalled_height, stride * bpp);
        printf("Screen scalled to %dx%dx%d\n", scalled_width, scalled_height, bpp);
    }
    else
        DIE("Fail getting screenshot\n");

    printf("Sending to %s\n", inet_ntoa(target.sin_addr));
    pthread_mutex_lock(&mutex1);
    while (device_state != DEVICE_STATE_EXIT)
    {
        if (device_state == DEVICE_STATE_SLEEP)
        {
            pthread_mutex_unlock(&mutex1);
            // send empty packet, so server know our addr & port and can send input back
            // to our address even device is slept by sending FK17 (Middle Mouse).
            sendto(net_fd, NULL, 0, 0, (struct sockaddr *)&target, __SOCK_SIZE__);
            sleep(1);
            continue;
        }
        pthread_mutex_unlock(&mutex1);

        // Todo: detect screen sleep, dont send anything
        if (screenshot.update(display, scalled_width, scalled_height) == NO_ERROR)
        {
            bmp.setPixels((void *)screenshot.getPixels());
            SkImageEncoder::EncodeStream(&stream, bmp, SkImageEncoder::kJPEG_Type, 70);
            streamData = stream.copyToData();

            sendto(net_fd, streamData->data(), streamData->size(), 0, (struct sockaddr *)&target, __SOCK_SIZE__);
            //send(fd, streamData->data(), streamData->size(), 0);
            streamData->unref();
            stream.reset();
        }
        else
        {
            fprintf(stderr, "Error capturing screen\n");
        }
        usleep(delay);
    }

    printf("Main thread exit.\n");
    return 0;
}