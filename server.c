#include <netinet/in.h> // primarily used for defining data structures that are related to networking
#include <arpa/inet.h> // contains functions for manipulating internet addresses
#include <sys/socket.h> // provides socket-related functions and structures
#include <unistd.h> // ex. includes the close function, which is often used to close file descriptors, including those of sockets

#include <stdio.h> // standard input/output operations
#include <string.h> // string manipulation functions
#include <stdbool.h> // the boolean type and associated macros (true and false)
#include <stdlib.h> // general-purpose functions
#include <errno.h> // handling errors

#include <sys/time.h> // It is commonly used for tasks such as benchmarking, profiling, and managing timeouts in network programming

// -----------------------------
#define PORT 8080
#define MAX_CLIENTS 3
// -----------------------------
// try start listen on specific ip address -> in ur comm. ip4 and ip of device
void start_listen(int *server_fd, struct sockaddr_in address){
	// create socket FD
	if( (*server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0 ){
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	// bind server to address
	if( bind(*server_fd, (struct sockaddr *)&address, sizeof(address)) < 0 ){
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	printf("Listening on port %d \n", PORT);
	// start listening for connection
	if( listen(*server_fd, 4) < 0 ){
		perror("listen");
		exit(EXIT_FAILURE);
	}
}
// -----------------------------
int main(void){
	int server_fd; // file descriptor
	int addrlen;  // length of address
	int sd;  // socket descriptor
	int activity; // checking activity on descriptors
	int max_sd;  // keeps track max fd value
	int new_socket; 
	int client_socket[MAX_CLIENTS];
	ssize_t valread;

	struct sockaddr_in address;

	char buffer[1025]; // for incoming msgs
	
	fd_set readfds; // initialize set for fds

	// initialise all clients_socket[] to 0
	for(int i=0; i<MAX_CLIENTS; i++){
		client_socket[i] = 0;
	}

	start_listen(&server_fd, address);

	addrlen = sizeof(address);   
	printf("Waiting for connections ...\n"); 

	while(true){
		FD_ZERO(&readfds); // clear fd set

		FD_SET(server_fd, &readfds); // add listening socket to set
		max_sd = server_fd;
		for(int i=0; i<MAX_CLIENTS; i++){ // add each valid socket to array
			sd = client_socket[i];

			if(sd > 0){
				FD_SET(sd, &readfds);
			}
			if(sd > max_sd){
				max_sd = sd;
			}
		}

		// monitor multiple fds
		activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
		if( (activity < 0) && (errno!=EINTR) ){
			printf("select error");
		}
		
		// someone connected
		if( (FD_ISSET(server_fd, &readfds)) ){
			if( (new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0){
				perror("accept");
				exit(EXIT_FAILURE);
			}
			// show message who connected
			printf("New connection , socket fd is %d , ip is : %s, port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
			// add client to array
			for(int i=0; i<30; i++){
				if( client_socket[i] == 0 ){
					client_socket[i] = new_socket;
					break;
				}
			}	
		}
		
		// broadcast and handle disconnection
		for(int i=0; i<MAX_CLIENTS; i++){
			sd = client_socket[i];
			
			if( (FD_ISSET(sd, &readfds)) ){
				// is some client disconnected
				if ((valread = read( sd , buffer, 1024)) == 0){
					getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
					printf("socket disconnected, ip: %s, port: %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
					close(sd);
					client_socket[i] = 0;
				} else {
					// if some message came broadcast it to all clients
					buffer[valread] = '\0';
					for(int i=0; i<MAX_CLIENTS; i++){
						int cfd = client_socket[i];
						if( cfd != sd ){
							send(cfd, buffer, strlen(buffer), 0);
						}
					}
					buffer[0] = '\0';
				}
			}
		}
	}

	return 0;
}

