#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "userlist.h"
#include "queue.h"

struct user *g_ul;

int create_ul() {
    InitDQ(g_ul, struct user);
    assert(g_ul);
    g_ul->username = NULL;
    g_ul->socket = -1;

    return (g_ul != 0x0);
}

int add_user(char *username, int clntfd) {
    struct user *nu = (struct user *)malloc(sizeof(struct user));

    nu->socket = clntfd;
    nu->username = (char *)malloc(strlen(username) + 1);
    strcpy(nu->username, username);

    InsertDQ(g_ul, nu);
    return (nu != 0x0);
}

struct user *find_user(char *username) {
    struct user *i;

    for (i = g_ul->next; i != g_ul; i = i->next) {
        if (strcmp(i->username, username) == 0) {
            return i;
        }
    }
    return 0x0;
}

int update_user(char *oldname, char *newname) {
    struct user *i = find_user(oldname);

    if (i && strcmp(i->username, oldname) == 0) {
        free(i->username); // free old username

        // Allocate space for the new username
        i->username = (char *)malloc(strlen(newname) + 1);
        strcpy(i->username, newname);

        return 0;
    } else {
        return -1;
    }
}

int del_user(char *username) {
    struct user *i = find_user(username);

    if (i && strcmp(i->username, username) == 0) {
        DelDQ(i);
        free(i->username);
        free(i);
        return 0;
    } else {
        return -1;
    }
}

// Return the user attached to a sd
struct user *sd_to_user(int sd) {
	struct user *i;
	for (i = g_ul->next; i != g_ul; i = i->next) {
		if (i->socket == sd) {
			return i;
		}
	}
	return 0x0;
}