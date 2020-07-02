#define HAVE_ENDIAN_H 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
#include "main.h"
#include "input.h"

using namespace android;
int fd;
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

int is_screen_locked()
{
    Vector<String16> args;
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> service = sm->checkService(String16("power"));
    int err = service->dump(STDOUT_FILENO, args);
    if (err != 0)
        printf("error: get power service");

    return 0;
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

    if (argc == 1)
        DIE("no IP target");

    targetIP = inet_addr(argv[1]);

    delay = 1000000 / fps;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd == -1)
        DIE("Fail creating socket");

    memset(&target, 0, addr_size);
    target.sin_family = AF_INET;
    target.sin_addr.s_addr = targetIP;
    target.sin_port = htons(port);
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    input_setup(port);
    input_press(KEY_F17);

    ProcessState::self()->startThreadPool();
    display = SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain);

    if (SurfaceComposerClient::getDisplayInfo(display, &mainDpyInfo) != NO_ERROR)
        DIE("ERROR: unable to get display characteristics");

    scalled_width = mainDpyInfo.w / 2;
    scalled_height = mainDpyInfo.h / 2;

    if (screenshot.update(display, scalled_width, scalled_height) == NO_ERROR)
    {
        stride = screenshot.getStride();
        format = screenshot.getFormat();
        bpp = bytesPerPixel(format);
        bmpConfig = flinger2skia(format);
        bmp.setConfig(bmpConfig, scalled_width, scalled_height, stride * bpp);
        printf("Screen format %d: stride %d, scalled %dx%dx%d\n", format, stride, scalled_width, scalled_height, bpp);
    }
    else
        DIE("Fail getting screenshot\n");

    printf("Sending to %s, scalled screen %dx%d\n", inet_ntoa(target.sin_addr), scalled_width, scalled_height);

    while (1)
    {
        // Todo: detect screen sleep, dont send anything
        if (screenshot.update(display, scalled_width, scalled_height) == NO_ERROR)
        {
            bmp.setPixels((void *)screenshot.getPixels());
            SkImageEncoder::EncodeStream(&stream, bmp, SkImageEncoder::kJPEG_Type, 70);
            streamData = stream.copyToData();

            sendto(fd, streamData->data(), streamData->size(), 0, (struct sockaddr *)&target, __SOCK_SIZE__);
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

    return 0;
}