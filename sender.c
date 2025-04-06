#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/utsname.h>

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
    struct in_addr localInterface;
    struct utsname sysinfo;

    uname(&sysinfo);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, group, &addr.sin_addr);

    localInterface.s_addr = inet_addr("192.168.56.101"); // lub dynamicznie
    setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface));

    while (1) {
        char input[900], message[MAXLINE];
        printf("> ");
        fflush(stdout);
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        input[strcspn(input, "\n")] = 0;

        snprintf(message, sizeof(message), "[%s@%s]: %s", sysinfo.sysname, sysinfo.nodename, input);
        sendto(sockfd, message, strlen(message), 0, (struct sockaddr *) &addr, sizeof(addr));
    }

    close(sockfd);
    return 0;
}
