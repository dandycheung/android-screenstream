#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

    if (argc == 1)
    {
        printf("no IP target\n");
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
        sendto(fd, buff, size, 0, (struct sockaddr *)&target, addr_size);
        sleep(3);
    }
    return 0;
    printf("Exited\n");
}