#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

const int BUF_SIZE = 1024;

void error_handling(const char *message);

// Receive two parameters, argv [1] as port number
int main(int argc, char *argv[]) {
    int server_socket;
    int client_sock;

    char message[BUF_SIZE];
    ssize_t str_len;
    int i;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_addr_size;

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    Ser_socket = socket (PF_INET, SOCK_STREAM, 0); //Create IPv4 TCP socket
    if (server_socket == -1) {
        error_handling("socket() error");
    }

    // Initialization of address information
    memset(&server_addr, 0, sizeof(server_addr));
    Server_addr.sin_family = AF_INET; // IPV4 address family
    Server_addr.sin_addr.s_addr = htonl (INADDR_ANY); // Assign the IP address of the server using INADDR_ANY
    Server_addr.sin_port = htons (atoi (argv [1]); the // port number is set by the first parameter

    // Assignment of address information
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(sockaddr)) == -1) {
        error_handling("bind() error");
    }

    // Listen for connection requests with a maximum number of simultaneous connections of 5
    if (listen(server_socket, 5) == -1) {
        error_handling("listen() error");
    }

    client_addr_size = sizeof(client_addr);
    for (i = 0; i < 5; ++i) {
        // Accept client connection requests
        client_sock = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_size);
        if (client_sock == -1) {
            error_handling("accept() error");
        } else {
            printf("Connect client %d\n", i + 1);
        }

        // Read data from the client
        while ((str_len = read(client_sock, message, BUF_SIZE)) != 0) {
            // Transfer data to client
            write(client_sock, message, (size_t)str_len);
            message[str_len] = '';
            printf("client %d: message %s", i + 1, message);
        }
    }
    // Close the connection
    close(client_sock);

    printf("echo server\n");
    return 0;
}