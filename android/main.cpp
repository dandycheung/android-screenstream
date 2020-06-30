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
#include <ui/PixelFormat.h>
#include <SkImageEncoder.h>
#include <SkBitmap.h>
#include <SkData.h>
#include <SkStream.h>
using namespace android;

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

int printaddr(char *buff, struct sockaddr_in *addr)
{
    uint32_t ip = ntohl(addr->sin_addr.s_addr);
    uint16_t port = ntohs(addr->sin_port);
    return sprintf(buff, "%d.%d.%d.%d:%d",
                   ip >> 24 & 0xff,
                   ip >> 16 & 0xff,
                   ip >> 8 & 0xff,
                   ip >> 0 & 0xff,
                   port);
}

int main(int argc, char const *argv[])
{
    uint32_t targetIP;
    uint16_t port = 1234U;
    struct sockaddr_in target;
    size_t size;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int fd;
    char *buff = (char *)malloc(64);
    void const *base = 0;
    uint32_t w, s, h, f;
    SkDynamicMemoryWStream stream;
    ScreenshotClient screenshot;
    SkBitmap b;
    SkData *streamData;

    ProcessState::self()->startThreadPool();

    if (argc == 1)
    {
        printf("no IP target\n");
        return 1;
    }
    sp<IBinder> display = SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain);
    if (display == NULL)
    {
        fprintf(stderr, "Can't open display\n");
        return 1;
    }
    targetIP = inet_addr(argv[1]);
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
    {
        fprintf(stderr, "Fail creating socket\n");
        return 1;
    }
    memset(&target, 0, addr_size);
    target.sin_family = AF_INET;
    target.sin_addr.s_addr = targetIP;
    target.sin_port = htons(port);
    printaddr(buff, &target);
    printf("Sending to %s\n", buff);
    strcpy(buff, "HELLO SERVER..");
    size = 16;
    while (1)
    {
        // Todo: detect screen sleep, dont send anything
        if (screenshot.update(display) == NO_ERROR)
        {
            base = screenshot.getPixels();
            w = screenshot.getWidth();
            h = screenshot.getHeight();
            s = screenshot.getStride();
            f = screenshot.getFormat();
            b.setConfig(flinger2skia(f), w, h, s * bytesPerPixel(f));
            b.setPixels((void *)base);
            SkImageEncoder::EncodeStream(&stream, b, SkImageEncoder::kJPEG_Type, 70);
            streamData = stream.copyToData();
            sendto(fd, streamData->data(), streamData->size(), 0, (struct sockaddr *)&target, addr_size);
            streamData->unref();
            stream.reset();
        }
        else
        {
            fprintf(stderr, "Error capturing screen\n");
        }
        usleep(100);
        //sleep(1);
    }
    return 0;
    printf("Exited\n");
}