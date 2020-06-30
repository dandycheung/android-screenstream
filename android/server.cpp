#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <binder/ProcessState.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>
#include <ui/PixelFormat.h>
#include <SkImageEncoder.h>
#include <SkBitmap.h>
#include <SkData.h>
#include <SkStream.h>

#include "server.h"

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

static void handle_connection(int fd)
{
    int client_fd, bufSize = 65535;
    uint32_t w, s, h, f;
    struct sockaddr_in client;
    socklen_t clientLen;
    char *buf = (char *)malloc(bufSize), *path;
    ProcessState::self()->startThreadPool();
    void const *base = 0;
    SkDynamicMemoryWStream stream;
    ssize_t size = 0;
    ScreenshotClient screenshot;
    SkBitmap b;
    SkData *streamData;

    sp<IBinder> display = SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain);
    if (display == NULL)
    {
        fprintf(stderr, "Can't open display\n");
        return;
    }

    while (1)
    {
        clientLen = sizeof(client);
        memset(&client, 0, clientLen);
        printf("Waiting connection...\n");
        client_fd = accept(fd, (struct sockaddr *)&client, &clientLen);
        if (client_fd < 0)
        {
            printf("Fail accepting client\n");
            continue;
        }
        size = recv(client_fd, buf, bufSize, 0);
        if (size < 0)
        {
            printf("Ignore recv %d bytes\n", size);
            continue;
        }
        buf[size] = 0;
        printf("%s", buf);

        if (screenshot.update(display) == NO_ERROR)
        {
            base = screenshot.getPixels();
            w = screenshot.getWidth();
            h = screenshot.getHeight();
            s = screenshot.getStride();
            f = screenshot.getFormat();
            b.setConfig(flinger2skia(f), w, h, s * bytesPerPixel(f));
            b.setPixels((void *)base);
            SkImageEncoder::EncodeStream(&stream, b,
                                         SkImageEncoder::kWEBP_Type, SkImageEncoder::kDefaultQuality);
            streamData = stream.copyToData();
            write(client_fd, buf, size);
            write(client_fd, streamData->data(), streamData->size());
            streamData->unref();
            stream.reset();
        }
        else
        {
            fprintf(stderr, "Error capturing screen\n");
            write(client_fd, buf, size);
        }

        close(client_fd);
    }
}

int server_start()
{
    uint16_t port = 1234U;
    struct sockaddr_in serveraddr;
    socklen_t addrLen = sizeof(struct sockaddr_in);
    int fd = socket(AF_INET, SOCK_DGRAM, 0), opt_reuseaddr = 1;

    if (fd == -1)
    {
        fprintf(stderr, "Fail creating socket\n");
        return 1;
    }
    memset(&serveraddr, 0, addrLen);
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);
    printf("Listening on port %d\n", port);
    //setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt_reuseaddr, sizeof(opt_reuseaddr));

    if (bind(fd, (struct sockaddr *)&serveraddr, addrLen) == 0)
    {
        handle_connection(fd);
    }
    else
    {
        printf("Fail bind\n");
        close(fd);
        return 1;
    }
    printf("Exited\n");
}