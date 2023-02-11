#include <stdint.h>

typedef struct buffer {
    unsigned int __size;
    unsigned int len;
    void *data;
} buffer;

int sListen(int port);
void sendData(int clntfd, uint32_t data_size, uint8_t cmd, uint8_t err, void *data);
char *assignUsername();
int acceptClient(int sockfd);
void recvData(int sd, size_t bytes, void *buffer);
uint8_t recvCommand(int clntfd, uint32_t *size);
buffer *initEmpty();
int appendBuf(buffer *b, unsigned int len, void *data);
void freeBuf(buffer *b);