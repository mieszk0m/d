#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAXLINE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Użycie: %s <multicast_addr> <port> <interfejs>\n", argv[0]);
        exit(1);
    }

    char *group = argv[1];
    int port = atoi(argv[2]);
    char *iface = argv[3];

    int sockfd;
    struct sockaddr_in addr;
    struct ip_mreq mreq;
    char buffer[MAXLINE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(sockfd, (struct sockaddr *) &addr, sizeof(addr));

    mreq.imr_multiaddr.s_addr = inet_addr(group);
    mreq.imr_interface.s_addr = inet_addr("192.168.56.101"); // lub dynamicznie
    setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    while (1) {
        int n = recvfrom(sockfd, buffer, MAXLINE, 0, NULL, 0);
        if (n < 0) {
            perror("recvfrom error");
            continue;
        }
        buffer[n] = 0;
        printf("💬 %s\n", buffer);
    }

    close(sockfd);
    return 0;
}
