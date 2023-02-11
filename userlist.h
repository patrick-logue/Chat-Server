#ifndef _UL_H_
#define _UL_H_

#include <netinet/in.h>

struct user
{
    struct user *next; // next entry
    struct user *prev; // prev entry

    //struct sockaddr_in user_addr
    int socket;
    char *username;
};

int create_ul();
int add_user(char *username, int clntfd);
int update_user(char *oldname, char *newname);
int del_user(char *username);
struct user *find_user(char *username);
struct user *sd_to_user(int sd);

#endif
