#ifndef _RL_H_
#define _RL_H_

#include <netinet/in.h>

#define MAX_USERS 1024 
#define MAX_USERNAME_SIZE 255

struct room
{
    struct room *next; // next entry
    struct room *prev; // prev entry

    char *name;
    char *password;
    char *ul[MAX_USERS];
};

int create_rl();
int create_room(char *roomname, char *password, char *username);
int add_user_to_room(char *roomname, char *username);
int remove_user_from_room(char *roomname, char *username);
struct room *find_room(char *roomname);
int is_empty(char *roomname);
struct room *find_users_room(char *username);

#endif
