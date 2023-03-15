#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define REMOTE_PORT 8080
#define LOCAL_PORT 8888

int main(int argc, char *argv[]) {
    int remote_socket, local_socket, remote_client_socket;
    struct sockaddr_in remote_addr, local_addr, remote_client_addr;
    int addr_len = sizeof(struct sockaddr_in);
    char buffer[1024];

    if ((remote_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    if ((local_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    memset(&remote_addr, 0, addr_len);
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(REMOTE_PORT);
    remote_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    memset(&local_addr, 0, addr_len);
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(LOCAL_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(local_socket, (struct sockaddr *)&local_addr, addr_len) < 0) {
        perror("Failed to bind socket");
        exit(EXIT_FAILURE);
    }

    if (listen(local_socket, 3) < 0) {
        perror("Failed to listen on socket");
        exit(EXIT_FAILURE);
    }

    printf("Listening for incoming connections on port %d...\n", LOCAL_PORT);

    if ((remote_client_socket = accept(local_socket, (struct sockaddr *)&remote_client_addr, (socklen_t*)&addr_len)) < 0) {
        perror("Failed to accept connection");
        exit(EXIT_FAILURE);
    }

    printf("Accepted connection from %s:%d\n", inet_ntoa(remote_client_addr.sin_addr), ntohs(remote_client_addr.sin_port));

    if (connect(remote_socket, (struct sockaddr *)&remote_addr, addr_len) < 0) {
        perror("Failed to connect to remote machine");
        exit(EXIT_FAILURE);
    }

    printf("Connected to remote machine at %s:%d\n", inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));

    while (1) {
        int num_bytes_read = read(remote_client_socket, buffer, sizeof(buffer));

        if (num_bytes_read <= 0) {
            break;
        }

        write(remote_socket, buffer, num_bytes_read);
        memset(buffer, 0, sizeof(buffer));

        num_bytes_read = read(remote_socket, buffer, sizeof(buffer));

        if (num_bytes_read <= 0) {
            break;
        }

        write(remote_client_socket, buffer, num_bytes_read);
        memset(buffer, 0, sizeof(buffer));
    }

    printf("Connection closed\n");

    close(remote_client_socket);
    close(remote_socket);
    close(local_socket);

    return 0;
}
