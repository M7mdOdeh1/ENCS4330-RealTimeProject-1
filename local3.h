#ifndef LOCAL3_H
#define LOCAL3_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h> 
#include <wait.h>
#include <signal.h>

#define MAX_LINE_LENGTH 100
#define MAX_ITEMS 1000
#define MAX_CUSTOMERS 10000
#define MAX_CASHIERS 100


char *trim(char *str);
int randomInRange(int min_range, int max_range);



struct Item {
    char name[50];
    int inventory;
    float price;
};

struct MEMORY {
    int numItems;
    struct Item items[MAX_ITEMS]; 
    int head, tail;
};

struct String {
    char str[50];
};

struct SHOPPING_CART {
    int numItems;
    int quantityOfItems;
    struct String items[MAX_ITEMS][3];
};

struct CASHIER{
    // add a queue of shopping carts
    int behavior; 
    struct SHOPPING_CART cartsQueue[MAX_CUSTOMERS];
};

struct ALL_SHOPPING_CARTS {
    struct SHOPPING_CART carts[MAX_CUSTOMERS];
    struct CASHIER cashiers[MAX_CASHIERS];
    int head, tail;
};





union semun {
    int val;
    struct semid_ds *buf;
    ushort *array; 
};

struct sembuf acquire = {0, -1, SEM_UNDO}, 
              release = {0,  1, SEM_UNDO};

#endif