#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
/**
 * TCP Uses 2 types of sockets, the connection socket and the listen socket.
 * The Goal is to separate the connection phase from the data exchange phase.
 * */

template <typename IntegerType>
IntegerType bitsToInt(IntegerType& result, const unsigned char* bits, bool little_endian = true)
{
	result = 0;
	if (little_endian)
		for (int n = sizeof(result); n >= 0; n--)
			result = (result << 8) + bits[n];
	else
		for (unsigned n = 0; n < sizeof(result); n++)
			result = (result << 8) + bits[n];
	return result;
}

int main(int argc, char *argv[]) {
	std::cout << argv[1]<< " " << argc << std::endl;
	// port to start the server on
	if (argc != 2) { printf("usage : ./{PROGRAMNAME} {SERVER_PORT}\n"); return 0; }

	int SERVER_PORT = atoi(argv[1]);
	std::cout << "port : " << SERVER_PORT << std::endl;
	

	// socket address used for the server
	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;

	// htons: host to network short: transforms a value in host byte
	// ordering format to a short value in network byte ordering format
	server_address.sin_port = htons(SERVER_PORT);

	// htonl: host to network long: same as htons but to long
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);

	// create a TCP socket, creation returns -1 on failure
	int listen_sock;
	if ((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		printf("could not create listen socket\n");
		return 1;
	}

	// bind it to listen to the incoming connections on the created server
	// address, will return -1 on error
	if ((bind(listen_sock, (struct sockaddr *)&server_address,
	          sizeof(server_address))) < 0) {
		printf("could not bind socket\n");
		return 1;
	}

	int wait_size = 16;  // maximum number of waiting clients, after which
	                     // dropping begins
	if (listen(listen_sock, wait_size) < 0) {
		printf("could not open socket for listening\n");
		return 1;
	}

	// socket address used to store client address
	struct sockaddr_in client_address;
	int client_address_len = 0;

	// run indefinitely
	while (true) {
		// open a new socket to transmit data per connection
		int sock;
		if ((sock =
		         accept(listen_sock, (struct sockaddr *)&client_address,
		                &client_address_len)) < 0) {
			printf("could not open a socket to accept data\n");
			return 1;
		}

		int n = 0;
		int len = 0, maxlen = 66+112*112*3;
		/*char buffer[maxlen];*/
		char* buffer = new char[maxlen];
		

		std::cout << "client connected with ip address: " << inet_ntoa(client_address.sin_addr) << std::endl;

		std::cout << "wait : recv" << std::endl;
        // keep running as long as the client keeps the connection open
		while (1) {

			int total_image_size;
			char *pbuffer = buffer;
			unsigned char bytes[4];
			int received = 0;
			int nb = 0;

			n = recv(sock, bytes, 4, 0);
			bitsToInt(total_image_size, bytes, false);

			while (received < total_image_size)
			{
				int n = total_image_size - received >= maxlen ? maxlen : total_image_size - received;
				nb = recv(sock, pbuffer, maxlen - received, received);
				received += nb;
				pbuffer += nb;
			}

            if((n = recv(sock, buffer, maxlen, 0)) > 0)
            {
				std::cout << "received: " << buffer << std::endl;
            }
		}

		std::cout << "connection cancel.\n" << std::endl;
		close(sock);
		delete(buffer);
	}

	close(listen_sock);
	return 0;
}