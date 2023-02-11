#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "roomlist.h"
#include "userlist.h"
#include "queue.h"

struct room *g_rl;

int create_rl() {
    InitDQ(g_rl, struct room);
    assert(g_rl);
    g_rl->name = NULL;
    g_rl->password = NULL;

    return (g_rl != 0x0);
}

int create_room(char *roomname, char *password, char *username) {
    struct room *nr = (struct room *)malloc(sizeof(struct room));

    nr->name = (char *)malloc(strlen(roomname) + 1);
    strcpy(nr->name, roomname);
    
    if (password) {
        nr->password = (char *)malloc(strlen(password) + 1);
        strcpy(nr->password, password);
    }

    memset(nr->ul, 0, sizeof(nr->ul));
    nr->ul[0] = (char *)malloc(strlen(username) + 1);
    strcpy(nr->ul[0], username);

    InsertDQ(g_rl, nr);
    return (nr != 0x0);
}

int add_user_to_room(char *roomname, char *username) {
    struct room *i = find_room(roomname);

    if (!i) {
        perror("Could not find room\n");
        return -1;
    }

    for (int j = 0; j < MAX_USERS; j++) {
        if (i->ul[j] == 0) {
            i->ul[j] = (char *)malloc(strlen(username) + 1);
            strcpy(i->ul[j], username);

            return 0;
        }
    }
    return -1;
}

int remove_user_from_room(char *roomname, char *username) {
    struct room *i = find_room(roomname);

    if (!i) {
        perror("Could not find room\n");
        return -1;
    }

    for (int j = 0; j < MAX_USERS; j++) {
        if (i->ul[j] != 0 && (strcmp(i->ul[j], username) == 0)) {
            free(i->ul[j]);
            i->ul[j] = 0;

            if (is_empty(roomname)) {
                DelDQ(i);
                free(i->name);
                free(i->password);
                free(i);
            }
            return 0;
        }
    }

    return -1;
}

struct room *find_room(char *roomname) {
    struct room *i;

    for (i = g_rl->next; i != g_rl; i = i->next) {
        if (strcmp(i->name, roomname) == 0) {
            return i;
        }
    }
    return 0x0;
}

int is_empty(char *roomname) {
    struct room *i = find_room(roomname);

    if (!i) {
        perror("Could not find room\n");
        return -1;
    }

    for (int j = 0; j < MAX_USERS; j++) {
        if (i->ul[j] !=0) {
            return 0;
        }
    }
    return 1;
}

struct room *find_users_room(char *username) {
    struct room *i;

    for (i = g_rl->next; i != g_rl; i = i->next) {
         for (int j = 0; j < MAX_USERS; j++) {
            if (i->ul[j] != 0 && strcmp(i->ul[j], username) == 0) {
                return i;
            }
         }
    }

    return 0x0;
}