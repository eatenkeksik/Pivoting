#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

int main(int argc, char** argv) {
  int tun_fd = open("/dev/net/tun", O_RDWR);
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  strncpy(ifr.ifr_name, "tun0", IFNAMSIZ);
  ioctl(tun_fd, TUNSETIFF, (void *)&ifr);
  int local_sock = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in local_addr;
  local_addr.sin_family = AF_INET;
  inet_aton("10.0.0.1", &local_addr.sin_addr);
  local_addr.sin_port = htons(8888);
  bind(local_sock, (struct sockaddr *)&local_addr, sizeof(local_addr));

  int remote_sock = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in remote_addr;
  remote_addr.sin_family = AF_INET;
  inet_aton("127.0.0.1", &remote_addr.sin_addr);
  remote_addr.sin_port = htons(8080);
  connect(remote_sock, (struct sockaddr *)&remote_addr, sizeof(remote_addr));

  char buffer[1024];
  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int num_bytes = recvfrom(local_sock, buffer, sizeof(buffer), 0,
                             (struct sockaddr *)&client_addr, &client_addr_len);
    if (num_bytes < 0) {
      perror("recvfrom failed");
      continue;
    }
    send(remote_sock, buffer, num_bytes, 0);
    num_bytes = recv(remote_sock, buffer, sizeof(buffer), 0);
    if (num_bytes < 0) {
      perror("recv failed");
      continue;
    }
    sendto(local_sock, buffer, num_bytes, 0, (struct sockaddr *)&client_addr, client_addr_len);
  }

  return 0;
}
