#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <assert.h>

#include "optparser.h"
#include "a3protos.h"
#include "userlist.h"
#include "roomlist.h"

buffer *list_rooms();
buffer *list_users();
buffer *list_users_in_room(struct room *room);
void send_pm(char * username, char *msg);
void send_to_room(char *username, struct room *curr_room, char *msg);

extern struct user *g_ul;
extern struct room *g_rl;

int main(int argc, char *argv[]) {
    server_arguments args;
    int sockfd, highest_fd, retval;
    struct user *i;
    fd_set rfds;

    if (argc != 3) {
        printf("Wrong number of arguments\n");
        exit(-1);
    }

    // Parse arguements
    args = server_parseopt(argc, argv);

    // Set server to listen on the port
    sockfd = sListen(args.port);
    highest_fd = sockfd;

    // Create the list of usernames and list of rooms 
    create_ul();
    create_rl();

    // Run server
    while(1) {
        // Initialize rfds
		FD_ZERO(&rfds);
        // Add the server socket to the set
        FD_SET(sockfd, &rfds);
        // Add the client sockets to the set
        for (i = g_ul->next; i != g_ul; i = i->next) {
            assert(i);
            FD_SET(i->socket, &rfds);

            if (i->socket > highest_fd) {
                highest_fd = i->socket;
            }
        }
        
        // Wait indefinitely for inc data
        retval = select(highest_fd + 1, &rfds, NULL, NULL, NULL);
        if (retval < 0) {
			perror("select() failed\n");
        }

        // If the server's sockfd is set, we know there is an inc connection
        if (FD_ISSET(sockfd, &rfds)) {

            // Accept the incoming client connection
            int clntfd = acceptClient(sockfd);  

            // Assign the client a default username
            char *new_username = assignUsername();

            // Add client's username and socket to the username list
            if (!add_user(new_username, clntfd)) { printf("add_user failed\n"); }

            // Send the client their default username
            sendData(clntfd, strlen(new_username), 0x9a, 0x0, new_username);

            free(new_username);
        }
        
        // If the client's fd is set, there is data to be read
        for (i = g_ul->next; i != g_ul; i = i->next) {
           
            if (!FD_ISSET(i->socket, &rfds)) { continue; } // If a socket isn't readable, skip it

            // Save the users current chat room (0x0 if not in a room)
            struct room *curr_room = find_users_room(i->username);

            // First receive the incoming data size and command
            uint32_t size = 0;
            uint8_t cmd = recvCommand(i->socket, &size);

            // Greetings or staying alive message. Receive and discard
            if (cmd == 0x9b || cmd == 0x13) {
                char buffer[size + 1];
                recvData(i->socket, size, buffer);
                buffer[size] = '\0';
                // printf("%s\n", buffer);
            
            // Send public message
            } else if (cmd == 0x15) {
                uint8_t roomname_size, msg_size, uk;

                // First receive roomname size
                recvData(i->socket, sizeof(roomname_size), &roomname_size);

                // If roomname size is not 0, receive roomname
                if (roomname_size) {
                    char roomname[roomname_size + 1];
                    recvData(i->socket, roomname_size, &roomname);
                    roomname[roomname_size] = '\0';
                    if (strcmp(curr_room->name, roomname) != 0) {
                        perror("rooms desynced");
                    }
                } 

                // Receive unknown byte and msg size
                recvData(i->socket, sizeof(uk), &uk);
                recvData(i->socket, sizeof(msg_size), &msg_size);

                // Receive message
                char msg[msg_size + 1];
                recvData(i->socket, msg_size, &msg);
                msg[msg_size] = '\0';
                
                // If the packet size is too large
                if (size > 0x0100) {
                    char *err = "Length limit exceeded.";
                    sendData(i->socket, strlen(err), 0x9a, 0x01, err);

                    // check if user was in a room and remove them
                    if (curr_room) {
                        remove_user_from_room(curr_room->name, i->username);
                    }
                    struct user *temp = i->prev;
                    close(i->socket);
                    del_user(i->username);
                    i = temp;          

                // If user is in a room, send msg to everyone in that room
                } else if (roomname_size) {
                    // Send the msg to each user in the current room
                    send_to_room(i->username, curr_room, msg);
                    sendData(i->socket, 0, 0x9a, 0x0, NULL);

                } else {
                    char *err = "You shout into the void and hear nothing.";
                    sendData(i->socket, strlen(err), 0x9a, 0x01, err);
                }

            // Send private message
            } else if (cmd == 0x12) {
                // Receive username
                uint8_t username_size, msg_size, uk;
                recvData(i->socket, sizeof(username_size), &username_size);
                char sender_username[username_size + 1];
                recvData(i->socket, username_size, sender_username);
                sender_username[username_size] = '\0';

                // Receive unknown byte and msg size
                recvData(i->socket, sizeof(uk), &uk);
                recvData(i->socket, sizeof(msg_size), &msg_size);

                // Receive message
                char msg[msg_size + 1];
                recvData(i->socket, msg_size, &msg);
                msg[msg_size] = '\0';

                // If the packet size is too large
                if (size > 0x0100) {
                    char *err = "Length limit exceeded.";
                    sendData(i->socket, strlen(err), 0x9a, 0x01, err);

                    // check if user was in a room and remove them
                    if (curr_room) {
                        remove_user_from_room(curr_room->name, i->username);
                    }
                    struct user *temp = i->prev;
                    close(i->socket);
                    del_user(i->username);
                    i = temp;                

                // If the username doesnt exist, send error
                } else if (!find_user(sender_username)) {
                    char *err = "Nick not present";
                    sendData(i->socket, strlen(err), 0x9a, 0x01, err);

                // Username exists. Send message
                } else {
                    send_pm(sender_username, msg);
                    sendData(i->socket, 0, 0x9a, 0x0, NULL);
                }

            // Change username
            } else if (cmd == 0x0f) {
                // First receive the size of the new username
                uint8_t data_size;
                recvData(i->socket, sizeof(data_size), &data_size);

                // Now receive the new username
                char username[data_size + 1];
                recvData(i->socket, data_size, username);
                username[data_size] = '\0';

                // If in a room, change username in room list
                if (curr_room) {
                    for (int j = 0; j < MAX_USERS; j++) {
                        if (curr_room->ul[j] && strcmp(curr_room->ul[j], i->username) == 0) { 
                            free(curr_room->ul[j]);
                            curr_room->ul[j] = (char *)malloc(strlen(username) + 1);
                            strcpy(curr_room->ul[j], username);
                            break;
                        }
                    }
                }

                // Change the username in the global username list
                free(i->username);
                i->username = (char *)malloc(strlen(username) + 1);
                strcpy(i->username, username);

                // Send ack to client
                sendData(i->socket, 0, 0x9a, 0x0, NULL);

            // List all users or just those in the current room
            } else if (cmd == 0x0c) {
                buffer *b;
                if (curr_room) {
                    b = list_users_in_room(curr_room);
                } else {
                    b = list_users();
                }
                sendData(i->socket, b->len, 0x9a, 0x0, b->data);
                freeBuf(b);

            // List chatrooms
            } else if (cmd == 0x09) {
                buffer *b = list_rooms();
                sendData(i->socket, b->len, 0x9a, 0x0, b->data);
                freeBuf(b);

            // Leave chatroom or server
            } else if (cmd == 0x06) {
                
                sendData(i->socket, 0, 0x9a, 0x0, NULL);
                if (curr_room) {
                    // Leave room
                    remove_user_from_room(curr_room->name, i->username);
                } else {
                    // Disconnect from the server
                    struct user *temp = i->prev;
                    close(i->socket);
                    del_user(i->username);
                    i = temp;
                }
    
            // Join or create room
            } else if (cmd == 0x03) {
                // First receive the size of the roomname
                uint8_t roomname_size;
                recvData(i->socket, sizeof(roomname_size), &roomname_size);

                // Next receive the roomname
                char roomname_buffer[roomname_size + 1];
                recvData(i->socket, roomname_size, roomname_buffer);
                roomname_buffer[roomname_size] = '\0';

                // Receive size of password
                uint8_t pass_size;
                recvData(i->socket, sizeof(pass_size), &pass_size);
                char pass_buffer[pass_size + 1];
                if (pass_size) {
                    recvData(i->socket, pass_size, pass_buffer);
                }
                pass_buffer[pass_size] = '\0';

                struct room *r = find_room(roomname_buffer); // Find the room
                
                // The joined room doesn't exist, create and join it
                if (!r) {
                    // If you are already in a room, leave it
                    if (curr_room) {
                        remove_user_from_room(curr_room->name, i->username);
                    }
                    create_room(roomname_buffer, pass_buffer, i->username);
                    sendData(i->socket, 0, 0x9a, 0x0, NULL);

                // User is attempting to join their own room
                } else if (curr_room && strcmp(curr_room->name, r->name) == 0) {
                    char *msg = "You attempt to bend space and time to reenter where you already are. You fail.";
                    sendData(i->socket, strlen(msg), 0x9a, 0x01, msg);

                // If the room either has a password and the passwords equal, or doesnt have a password
                } else if ((r->password && strcmp(r->password, pass_buffer) == 0) || !r->password) {
                    if (curr_room) {
                        remove_user_from_room(curr_room->name, i->username);
                    }
                    add_user_to_room(r->name, i->username);
                    sendData(i->socket, 0, 0x9a, 0x0, NULL);

                } else if (r->password && strcmp(r->password, pass_buffer) != 0) {
                    char *msg = "Invalid password. You shall not pass.";
                    sendData(i->socket, strlen(msg), 0x9a, 0x01, msg);
                }                

            // Connection dropped. Remove client and it's socket from the username list
            } else if (cmd == 0x0) {

                // check if user was in a room and remove them
                if (curr_room) {
                    remove_user_from_room(curr_room->name, i->username);
                }
                struct user *temp = i->prev;
                close(i->socket);
                del_user(i->username);
                i = temp;
            }
        }
    }

    return 0;
}

int mycmp(const void *a, const void *b) {
    return strcmp((char *)a, (char *)b);
}

buffer *list_rooms() {
    struct room *i;
    buffer *b = initEmpty();
    uint8_t len = 0;
    int count = 0, j = 0;

    // Count # of rooms
    for (i = g_rl->next; i != g_rl; i = i->next) {
        count++;
    }

    char room_arr[count][MAX_USERNAME_SIZE];
    memset(room_arr, 0, sizeof(room_arr));

    // Add all rooms to the room_arr
    for (i = g_rl->next; i != g_rl; i = i->next) {
        strcpy(room_arr[j], i->name);
        j++;
    }

    qsort(room_arr, count, MAX_USERNAME_SIZE, mycmp);

    for (j = 0; j < count; j++) {
        len = strlen(room_arr[j]);
        appendBuf(b, sizeof(uint8_t), &len);
        appendBuf(b, len, room_arr[j]);
    }

    return b;
}
buffer *list_users() {
    struct user *i;
    buffer *b = initEmpty();
    uint8_t len = 0;

    for (i = g_ul->next; i != g_ul; i = i->next) {
        len = strlen(i->username);
        appendBuf(b, sizeof(uint8_t), &len);
        appendBuf(b, len, i->username);
    }

    return b;
}

buffer *list_users_in_room(struct room *room) {
    buffer *b = initEmpty();
    uint8_t len = 0;

    for (int j = 0; j < MAX_USERS; j++) {
        if (!room->ul[j]) { continue; } // Skip empty positions in ul
        len = strlen(room->ul[j]);
        appendBuf(b, sizeof(uint8_t), &len);
        appendBuf(b, len, room->ul[j]);
    }
    return b;
}

void send_pm(char * username, char *msg) {
    // Construct message
    buffer *msg_to_send = initEmpty();
    struct user *user = find_user(username);

    if (!user) { perror("Could not find user\n"); }

    // Append username size, username, and zero byte
    uint8_t size = strlen(user->username);
    appendBuf(msg_to_send, sizeof(size), &size);
    appendBuf(msg_to_send, size, user->username);
    size = 0x0;
    appendBuf(msg_to_send, sizeof(size), &size);

    // Append msg size and msg
    size = strlen(msg);
    appendBuf(msg_to_send, sizeof(size), &size);
    appendBuf(msg_to_send, size, msg);

    sendData(user->socket, msg_to_send->len, 0x12, -1, msg_to_send->data);
    freeBuf(msg_to_send);
}

void send_to_room(char *username, struct room *curr_room, char *msg) {        
    // Construct message
    buffer *msg_to_send = initEmpty();
            
    // First append roomname size and roomname
    uint8_t size = strlen(curr_room->name);
    appendBuf(msg_to_send, sizeof(size), &size);
    appendBuf(msg_to_send, size, curr_room->name);

    // Next append username size, username, and zero byte
    size = strlen(username);
    appendBuf(msg_to_send, sizeof(size), &size);
    appendBuf(msg_to_send, size, username);
    size = 0x0;
    appendBuf(msg_to_send, sizeof(size), &size);

    // Finally append the msg size and msg itself
    size = strlen(msg);
    appendBuf(msg_to_send, sizeof(size), &size);
    appendBuf(msg_to_send, size, msg);
            
    for (int j = 0; j < MAX_USERS; j++) {
        if (curr_room->ul[j] != 0 && strcmp(curr_room->ul[j], username) != 0) {
            // User to send msg to
            struct user *peer = find_user(curr_room->ul[j]);
            if (!peer) { perror("Could not find user to send data to\n"); }

            sendData(peer->socket, msg_to_send->len, 0x15, -1, msg_to_send->data);
        }
    }
    freeBuf(msg_to_send);
}