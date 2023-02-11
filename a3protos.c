#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "a3protos.h"
#include "userlist.h"

static const int MAXCLIENTS = 1024;

int sListen(int port) {

    // Create endpoint and return tcp file descriptor
    int servSock;
    if ((servSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        perror("socket() failed\n");
        exit(0);
    }

    // Initialize struct to store server address, port, and family
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr)); // Zero
    servAddr.sin_family = AF_INET; // Set family to IPv4
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Set address tp accept any incoming messages

     // set server port number
    in_port_t servPort = port;
    servAddr.sin_port = htons(servPort); // Ensure binary value is formatted as required by API

    // Bind to the local address
    if (bind(servSock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        perror("bind() failed\n");
        exit(-1);
    }

    // Listen
    if (listen(servSock, MAXCLIENTS) < 0) {
        printf("listen() failed\n");
        exit(-1);
    }

    return servSock;
}

int acceptClient(int sockfd) {
    struct sockaddr_in clntAddr; // Clients address
    socklen_t clntAddrLen = sizeof(clntAddr); // Length of client address structure

    // Wait for connection
    int clntfd = accept(sockfd, (struct sockaddr *) &clntAddr, &clntAddrLen);
    if (clntfd < 0) {
        printf("accept() failed\n");
        exit(-1);
    }

    // print ip address and port for funsies
    // char clntName[INET_ADDRSTRLEN];
    // if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName, sizeof(clntName)) != NULL) {
    //     printf("Handling client %s/%d\n", clntName, ntohs(clntAddr.sin_port));
    // } else {
    //     printf("Unable to get client address\n");
    // }

    return clntfd;
}

void sendData(int clntfd, uint32_t data_size, uint8_t cmd, uint8_t err, void *data) {
    buffer *b = initEmpty();

    // Append size of data
    if (err != 0xff) {
        uint32_t d_s = htonl(data_size + 1);
        appendBuf(b, sizeof(d_s), &d_s);
    } else {
        uint32_t d_s = htonl(data_size);
        appendBuf(b, sizeof(d_s), &d_s);
    }

    // // Append magic number 
    uint16_t magic = htons(1047);
    appendBuf(b, sizeof(magic), &magic);

    // Append command (Ex: 0x9a)
    appendBuf(b, sizeof(cmd), &cmd); 
    
    // Append possible error code? 0x0 
    if (err != 0xff) {
        appendBuf(b, sizeof(uint8_t), &err);
    }

    // Append data if not NULL
    if (data) {
        appendBuf(b, data_size, data);
    }

    ssize_t numBytes = send(clntfd, b->data, b->len, 0);
    if (numBytes < 0) {
        perror("send() failed\n");
        exit(-1);

    } else if (numBytes != b->len) {
        perror("send() failed, sent unexpected number of bytes\n");
        exit(-1);
    }
    freeBuf(b);
}

char *assignUsername() {
    size_t nbytes;

    for (int i = 0; i < MAXCLIENTS; i++) {
        nbytes = snprintf(NULL, 0, "rand%d", i) + 1;
        char *username = malloc(nbytes);
        snprintf(username, nbytes, "rand%d", i);

        if (!find_user(username)) {
            return username;
        }
    }
    return 0x0;
}

void recvData(int sd, size_t bytes, void *buffer) {
    ssize_t numBytes = 0;
    unsigned int TotalBytesRcvd = 0;

    while (TotalBytesRcvd < bytes) {
        numBytes = recv(sd, buffer, bytes, 0);
        if (numBytes < 0) {
            printf("recv() failed here\n");
            exit(-1);
        } else if (numBytes == 0) {
            printf("recv() failed, connection closed prematurely\n");
            exit(-1);
        }
        TotalBytesRcvd += numBytes;
    }
}

uint8_t recvCommand(int clntfd, uint32_t *size) {
    // The first 7 bytes are the data size (4b), magic number (2b), and command (1b)
    uint8_t p[7];

    // Receive the incoming data
	ssize_t numBytes = 0;
    unsigned int TotalBytesRcvd = 0;

    while (TotalBytesRcvd < 7) {
        numBytes = recv(clntfd, &p, 7, 0);
        if (numBytes < 0) {
            printf("recv() failed here\n");
            exit(-1);
                    
        // Connection dropped, return command 0x0
        } else if (numBytes == 0) {
            return 0x0;
        }
        TotalBytesRcvd += numBytes;
    }

    if (p[4] != 0x04 && p[5] != 0x17) {
        printf("WRONG MAGIC NUMBER\n");
        // do something
    }

    // first 4 bytes are the size of the inc data
    *size = ntohl(*(uint32_t *)p);
    return p[6];
}

buffer *initEmpty() {
    buffer *b = malloc(sizeof(b->__size) + sizeof(b->len) + sizeof(b->data));
    b->__size = 1024;
    b->len = 0;
    b->data = calloc(b->__size, sizeof(unsigned char));
    return b;
}

int appendBuf(buffer *b, unsigned int len, void *data) {
    b->len += len;
     if (b->len > b->__size) {
        b->__size *= 2;
        void *temp = realloc(b->data, b->__size * sizeof(unsigned char));
        if (!temp) {
            free(b->data);
            printf("realloc() failed\n");
            exit(-1);
        }
        b->data = temp;
    }
    memcpy(b->data + b->len - len, data, len);
    return b->len;
}

void freeBuf(buffer *b) {
    free(b->data);
    free(b);
}