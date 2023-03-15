#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <sys/ioctl.h>

int main(int argc, char **argv)
{
    if(argc != 2) {
        printf("Usage: %s [interface]\n", argv[0]);
        return -1;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        perror("socket");
        return -1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, argv[1], IFNAMSIZ-1);
    if(ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl");
        close(sockfd);
        return -1;
    }
    unsigned char mac_addr[6];
    memcpy(mac_addr, ifr.ifr_hwaddr.sa_data, 6);

    struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
    char *ip_addr = inet_ntoa(addr->sin_addr);

    struct arpreq request;
    memset(&request, 0, sizeof(request));
    request.arp_pa.sa_family = AF_INET;
    ((struct sockaddr_in *)&request.arp_pa)->sin_addr.s_addr = inet_addr(ip_addr);
    request.arp_ha.sa_family = ARPHRD_ETHER;
    memcpy(request.arp_ha.sa_data, mac_addr, 6);
    request.arp_flags = ATF_COM;

    struct sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    if(sendto(sockfd, &request, sizeof(request), 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr)) < 0) {
        perror("sendto");
        close(sockfd);
        return -1;
    }

    unsigned char buffer[1024];
    ssize_t num_bytes;
    while((num_bytes = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
        struct arpreq *reply = (struct arpreq *)buffer;
        if(reply->arp_pa.sa_family != AF_INET || reply->arp_ha.sa_family != ARPHRD_ETHER) continue;
        printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x IP address: %s\n",
               reply->arp_ha.sa_data[0], reply->arp_ha.sa_data[1], reply->arp_ha.sa_data[2],
               reply->arp_ha.sa_data[3], reply->arp_ha.sa_data[4], reply->arp_ha.sa_data[5],
               inet_ntoa(((struct sockaddr_in *)&(reply->arp_pa))->sin_addr));
    }

    close(sockfd);
    return 0;
}
