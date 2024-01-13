#include <arpa/inet.h> // contains functions for manipulating internet addresses
#include <sys/socket.h> // provides socket-related functions and structures
#include <unistd.h> // ex. includes the close function, which is often used to close file descriptors, including those of sockets

#include <stdio.h> // standard input/output operations
#include <string.h> // string manipulation functions
#include <stdbool.h> // the boolean type and associated macros (true and false)
#include <stdlib.h> // general-purpose functions
#include <errno.h> // handling errors

#include <ncurses.h> // used for creating text-based user interfaces in a terminal

#include <pthread.h> // way to execute multiple tasks concurrently

// -----------------------------
#define PORT 8080

// struture for client
struct client_status {
    char messages[1024][1025]; // store messages
    int fd; // file descriptor
    int count; // counter of messages
};
//------------------------------
// add msg to array
void add_message(struct client_status *client, const char* message){
    strcpy(client->messages[client->count], message);
    client->count += 1;
}

// display chat with msgs
void display_chat(struct client_status *client){
    clear(); // clear gui
    for(int i=0; i<client->count; i++){ // print all messages from array of messages
        mvprintw(i, 0, "%s", client->messages[i]);
    }
    mvprintw(client->count, 0, ">> "); // for input
    refresh();
}
//------------------------------
// try connect to server
void connect_to_server(struct client_status *client, struct sockaddr_in address) {
    if ((client->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket cr. failure!");
        exit(EXIT_FAILURE);
    }

    // prepare address
    char *ip = "127.0.0.1";
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    if (inet_pton(AF_INET, ip, &address.sin_addr) <= 0) { // convert string to binary form
        perror("Invalid address!");
        exit(EXIT_FAILURE);
    }
    // try connect to server
    if (connect(client->fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Connection error!");
        exit(EXIT_FAILURE);
    }
}

// thread for recv msg from server which came from another client to server
void *recv_msg(void *args) {
    struct client_status *client = (struct client_status *)args; // client structure
    char buffer[1025]; // buffer for income msg
    int valread; // check if something came
    while (true) {
        valread = read(client->fd, buffer, 1025 - 1);
        if (valread > 0) { // if 1 msg came
            buffer[valread] = '\0';  // set end of buffer
            add_message(&(*client), buffer);

            buffer[0] = '\0'; // clear buffer

            display_chat(&(*client));
        }

        // server closed connection
        if( valread == 0 ){
            perror("SERVER CLOSE CONNECTION!");
            exit(EXIT_FAILURE);
        }
    }
}
//------------------------------
int main(void) {
    struct sockaddr_in address;

    struct client_status client;
    client.count = 0; // initialize counter for messages

    // message buffer for send to server
    char message[1025];

    // Initialize ncurses
    initscr();

    // try connect to server
    connect_to_server(&client, address);

    // create thread for recving msg from server
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, recv_msg, (void *)&client);

    while (true) { // main thread
        // display messages
        display_chat(&client);

        // get input
        getstr(message);
        message[strlen(message)] = '\0';

        // if input == "exit" break from while loop
        if (!strcmp(message, "exit")) { break; }

        // add msg to array of messages
        add_message(&client, message);

        // send msg to server and server will broadcast it
        send(client.fd, message, strlen(message), 0);
        message[0] = '\0'; // clear message buffer
    }

    // clean up and close ncurses
    endwin();
    // close connection with server
    close(client.fd);

    return 0;
}
